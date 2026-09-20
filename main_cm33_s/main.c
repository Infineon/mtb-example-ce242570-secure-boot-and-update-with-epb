/*****************************************************************************
 * File Name        : main.c
 *
 * Description      : This is the source code for Main CM33 secure application
 *
 * Related Document : See README.md
 *
 *******************************************************************************
 * (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
 *******************************************************************************/

/******************************************************************************
 * Header Files
 *****************************************************************************/

#include "cy_pdl.h"
#include "cybsp.h"
#include "retarget_io_init.h"

#include "partition_ARMCM33.h"
#include "partition_psc3.h"

#include "transport_i2c.h"
#include "cy_dfu.h"
#include "cy_dfu_logging.h"
#include "mtb_hal_i2c.h"
#include "cy_scb_i2c.h"
#include "cy_sysint.h"
#include "cybsp.h"

/*******************************************************************************
 * Macros
 *******************************************************************************/

/* Timeout for Cy_DFU_Continue(), in milliseconds */
#define DFU_SESSION_TIMEOUT_MS (20u)

/* DFU idle timeout: 300 seconds */
#define DFU_IDLE_TIMEOUT_MS (300000u)

/* DFU command timeout: 5 seconds */
#define DFU_COMMAND_TIMEOUT_MS (5000u)

/* LED Toggle Interval: 1 second */
#define LED_TOGGLE_INTERVAL_MS (1000u)

/*******************************************************************************
 * Defines
 *******************************************************************************/

/** Image version.  All fields are in little endian. */
typedef struct
{
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint16_t iv_build_num;
    uint16_t iv_magic;
} image_version_t;

/** Image header.  All fields are in little endian byte order. */
typedef struct
{
    uint32_t ih_magic;
    uint32_t ih_load_addr;
    uint16_t ih_hdr_size;         /* Size of image header (bytes). */
    uint16_t ih_protect_tlv_size; /* Size of protected TLV area (bytes). */
    uint32_t ih_img_size;         /* Does not include header. */
    uint32_t ih_flags;            /* IMAGE_F_[...]. */
    image_version_t ih_ver;
    uint32_t _pad1;
} image_header_t;

/*******************************************************************************
 * Global Variables
 *******************************************************************************/

/* I2C transport HAL object  */
static mtb_hal_i2c_t dfuI2cHalObj;
static cy_stc_scb_i2c_context_t dfuI2cContext;

/* Image Headers */
static const image_header_t *pImgHdrMainCm33s = (image_header_t *)CYMEM_CM33_0_S_m33s_nvm_S_START;
static const image_header_t *pImgHdrPPCA0 = (image_header_t *)CYMEM_CM33_0_S_ppca0_nvm_S_START;
static const image_header_t *pImgHdrPPCA1 = (image_header_t *)CYMEM_CM33_0_S_ppca1_nvm_S_START;

/*******************************************************************************
 * Function Prototype
 *******************************************************************************/

static void dfuI2cIsr(void);
static void dfuI2cTransportCallback(cy_en_dfu_transport_i2c_action_t action);
static void dfu_i2c_transport_init(void);
static char *dfu_status_in_str(cy_en_dfu_status_t dfu_status);

/*******************************************************************************
 * Function Name: dfuI2cIsr
 ********************************************************************************
 * Summary:
 *  I2C interrupt callback
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 *******************************************************************************/
static void dfuI2cIsr(void)
{
    mtb_hal_i2c_process_interrupt(&dfuI2cHalObj);
}

/*******************************************************************************
 * Function Name: dfuI2CTransportCallback
 ********************************************************************************
 * Summary:
 *  Callback to enable or disable DFU I2C transport
 *
 * Parameters:
 *  action : Callback trigger
 *
 * Return:
 *  void
 *
 *******************************************************************************/
static void dfuI2cTransportCallback(cy_en_dfu_transport_i2c_action_t action)
{
    if (action == CY_DFU_TRANSPORT_I2C_INIT)
    {
        Cy_SCB_I2C_Enable(DFU_I2C_HW);
    }
    else if (action == CY_DFU_TRANSPORT_I2C_DEINIT)
    {
        Cy_SCB_I2C_Disable(DFU_I2C_HW, &dfuI2cContext);
    }
}

/*******************************************************************************
 * Function Name: dfu_i2c_transport_init
 ********************************************************************************
 * Summary:
 *  Configure DFU I2C transport to receive data from DFU Host Tool
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 *******************************************************************************/
static void dfu_i2c_transport_init()
{
    cy_en_scb_i2c_status_t pdlI2cStatus;
    cy_en_sysint_status_t pdlSysIntStatus;
    cy_rslt_t halStatus;

    pdlI2cStatus = Cy_SCB_I2C_Init(DFU_I2C_HW, &DFU_I2C_config, &dfuI2cContext);
    if (CY_SCB_I2C_SUCCESS != pdlI2cStatus)
    {
        CY_DFU_LOG_ERR("Error during I2C PDL initialization. Status: %X", (unsigned int)pdlI2cStatus);
    }
    else
    {
        halStatus = mtb_hal_i2c_setup(&dfuI2cHalObj, &DFU_I2C_hal_config, &dfuI2cContext, NULL);
        if (CY_RSLT_SUCCESS != halStatus)
        {
            CY_DFU_LOG_ERR("Error during I2C HAL initialization. Status: %X", (unsigned int)halStatus);
        }
        else
        {
            cy_stc_sysint_t i2cIsrCfg =
            {
                .intrSrc = DFU_I2C_IRQ,
                .intrPriority = 3U
            };
            pdlSysIntStatus = Cy_SysInt_Init(&i2cIsrCfg, dfuI2cIsr);
            if (CY_SYSINT_SUCCESS != pdlSysIntStatus)
            {
                CY_DFU_LOG_ERR("Error during I2C Interrupt initialization. Status: %X", (unsigned int)pdlSysIntStatus);
            }
            else
            {
                NVIC_EnableIRQ((IRQn_Type)i2cIsrCfg.intrSrc);
                CY_DFU_LOG_INF("I2C transport is initialized");
            }
        }
    }
    cy_stc_dfu_transport_i2c_cfg_t i2cTransportCfg =
    {
        .i2c = &dfuI2cHalObj,
        .callback = dfuI2cTransportCallback,
    };
    Cy_DFU_TransportI2cConfig(&i2cTransportCfg);
}

/*******************************************************************************
 * Function Name: dfu_status_in_str
 ********************************************************************************
 * Summary:
 *  This is the function to convert DFU status in elaborative text
 *
 * Parameters:
 *  dfu_status
 *
 * Return:
 *  string pointer
 *
 *******************************************************************************/
static char *dfu_status_in_str(cy_en_dfu_status_t dfu_status)
{
    switch (dfu_status)
    {
        case CY_DFU_SUCCESS:
            return "Success";

        case CY_DFU_ERROR_VERIFY:
            return "Packet verification failed";

        case CY_DFU_ERROR_LENGTH:
            return "The length of the packet is outside of the expected range";

        case CY_DFU_ERROR_DATA:
            return "The data in the received packet is invalid";

        case CY_DFU_ERROR_CMD:
            return "The command is not recognized";

        case CY_DFU_ERROR_CHECKSUM:
            return "The checksum does not match the expected value ";

        case CY_DFU_ERROR_ADDRESS:
            return "Wrong address";

        case CY_DFU_ERROR_TIMEOUT:
            return "The command timed out";

        case CY_DFU_ERROR_BAD_PARAM:
            return "One or more of input parameters are invalid";

        case CY_DFU_ERROR_UNKNOWN:
            return "Unkown error";

        default:
            return "Unkown error";
    }
}

/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 *  This is the main function for Main CM33s CPU. It does...
 *    1. Device/Peripheral Initialization
 *    2. Starts PPCA Cores
 *    3. Toggles LED 1
 *    4. Receives update image through DFU MW and stages it in the secondary slots
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 *******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    uint32_t count = 0;
    cy_en_dfu_status_t dfu_status = CY_DFU_ERROR_UNKNOWN;
    uint32_t dfu_state = CY_DFU_STATE_NONE;
    bool dfu_started = false;

    /* Buffer to store DFU commands. */
    CY_ALIGN(4)
    static uint8_t dfu_buffer[CY_DFU_SIZEOF_DATA_BUFFER];
    /* Buffer for DFU data packets for transport API. */
    CY_ALIGN(4)
    static uint8_t dfu_packet[CY_DFU_SIZEOF_CMD_BUFFER];

    /* DFU params, used to configure DFU. */
    cy_stc_dfu_params_t dfu_params =
    {
        .timeout = DFU_SESSION_TIMEOUT_MS,
        .dataBuffer = &dfu_buffer[0],
        .packetBuffer = &dfu_packet[0],
    };

    /* Initialize the device and board peripherals */
    /* NOTE: In case PPCA cores are running from previous boot, attention should
     * be given to the peripherals being used by PPCA cores. We should either skip
     * or perform safe initialization of such peripherals. This is use case specific
     * hence not demonstrated in this code example */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* enable interrupts */
    __enable_irq();

    /* Initialize retarget-io middleware */
    init_retarget_io();

    /* Transmit header to the terminal */
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("******************************************************************\r\n\n");
    printf("        PSOC Control C3M8: Secure Boot and Update with EPB\r\n\n");
    printf("              Image Version ( Main CM33S ): %u.%u.%u+%u\r\n", pImgHdrMainCm33s->ih_ver.iv_major,
           pImgHdrMainCm33s->ih_ver.iv_minor, pImgHdrMainCm33s->ih_ver.iv_revision, pImgHdrMainCm33s->ih_ver.iv_build_num);
    printf("              Image Version ( PPCA CM330 ): %u.%u.%u+%u\r\n", pImgHdrPPCA0->ih_ver.iv_major,
           pImgHdrPPCA0->ih_ver.iv_minor, pImgHdrPPCA0->ih_ver.iv_revision, pImgHdrPPCA0->ih_ver.iv_build_num);
    printf("              Image Version ( PPCA CM331 ): %u.%u.%u+%u\r\n", pImgHdrPPCA1->ih_ver.iv_major,
           pImgHdrPPCA1->ih_ver.iv_minor, pImgHdrPPCA1->ih_ver.iv_revision, pImgHdrPPCA1->ih_ver.iv_build_num);
    printf("\n******************************************************************\r\n");

    /* Start PPCA Cores */
    Cy_System_Init_CPU0((void *)(CYMEM_CM33_0_S_ppca0_nvm_C_S_START + pImgHdrPPCA0->ih_hdr_size), pImgHdrPPCA0->ih_img_size);
    Cy_System_Init_CPU1((void *)(CYMEM_CM33_0_S_ppca1_nvm_C_S_START + pImgHdrPPCA1->ih_hdr_size), pImgHdrPPCA1->ih_img_size);

    /* Initialize DFU MW */
    dfu_status = Cy_DFU_Init(&dfu_state, &dfu_params);
    if (CY_DFU_SUCCESS != dfu_status)
    {
        printf("DFU MW init failed \r\n");
        CY_ASSERT(0);
    }

    /* Initialize DFU communication. */
    printf("\r\n Starting DFU Transport \r\n\n");
    dfu_i2c_transport_init();
    Cy_DFU_TransportStart(CY_DFU_I2C);

    for (;;)
    {
        dfu_status = Cy_DFU_Continue(&dfu_state, &dfu_params);
        count++;
        if (CY_DFU_STATE_FINISHED == dfu_state)
        {
            printf("    [DFU] Image download complete. Issueing Device reset !!!\r\n");
            
            while (false == (Cy_SCB_UART_IsTxComplete(DEBUG_UART_HW)));

            NVIC_SystemReset();
        }
        else if (CY_DFU_STATE_FAILED == dfu_state)
        {
            printf("    [DFU] Image download failed, %s \r\n", dfu_status_in_str(dfu_status));

            /* An error occurred. Handle it here.
             * This code just restarts the DFU */
            count = 0u;
            dfu_started = false;
            Cy_DFU_Init(&dfu_state, &dfu_params);
            Cy_DFU_TransportReset();
        }
        else if (dfu_state == CY_DFU_STATE_UPDATING)
        {
            if (dfu_status == CY_DFU_SUCCESS)
            {
                if (dfu_started == false)
                {
                    printf("    [DFU] Received DFU request, starting image download\r\n");
                    dfu_started = true;
                }
                count = 0u;
            }
            else if (dfu_status == CY_DFU_ERROR_TIMEOUT)
            {
                if (count >= (DFU_COMMAND_TIMEOUT_MS / DFU_SESSION_TIMEOUT_MS))
                {
                    /* No command has been received since last 5 seconds. Restart DFU */
                    printf("    [DFU] Image download failed, %s\r\n", dfu_status_in_str(dfu_status));
                    count = 0u;
                    dfu_started = false;
                    Cy_DFU_Init(&dfu_state, &dfu_params);
                    Cy_DFU_TransportReset();
                }
            }
            else
            {
                /* Handle other errors */
                printf("    [DFU] Image download failed, %s\r\n", dfu_status_in_str(dfu_status));

                /* Delay because Transport still may be sending error response to a host. */
                Cy_SysLib_Delay(DFU_SESSION_TIMEOUT_MS);

                /* Restart DFU. */
                count = 0u;
                dfu_started = false;
                Cy_DFU_Init(&dfu_state, &dfu_params);
                Cy_DFU_TransportReset();
            }
        }
        else
        {
            /* dfu_state == CY_DFU_STATE_NONE */
            if (count >= (DFU_IDLE_TIMEOUT_MS / DFU_SESSION_TIMEOUT_MS))
            {
                /* No DFU request received in 300 seconds, lets start over.
                 * Final application can change it to either assert, reboot,
                 * enter low power mode etc, based on usecase requirements. */
                count = 0;
            }

            Cy_DFU_Init(&dfu_state, &dfu_params);
        }

        /* Blink once per second */
        if ((count % (LED_TOGGLE_INTERVAL_MS / DFU_SESSION_TIMEOUT_MS)) == 0u)
        {
            /* Invert the USER LED state */
            Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
        }

        Cy_SysLib_Delay(1);
    }
}
