[Click here](../README.md) to view the code example README.

# Project setup

This page collects the one-time setup for each demonstration described in the [Operation](../README.md#operation) section. Complete only the setup for the demonstration(s) you intend to run — each is independent.

- [1. Secure boot and update of the application](#1-secure-boot-and-update-of-the-application) — image signing key and bootloader image validation (required for the base flow).
- [2. Encrypted update](#2-encrypted-update) — encrypt the update image (optional).
- [3. Secure boot of the EdgeProtect Bootloader](#3-secure-boot-of-the-edgeprotect-bootloader) — sign and provision the bootloader (optional).

## 1. Secure boot and update of the application

Set up the image signing key and configure the EdgeProtect Bootloader to validate application images. Perform this before running [Operation](../README.md#operation) for the first time.

### 1.1 Generate the image signing key (optional)

Sample key pairs for all supported signature schemes are provided in the *keys/* folder, so you can skip this subsection and go straight to [1.2 Import the image signing key](#12-import-the-image-signing-key). To generate your own key pair (recommended for production), follow the steps below.

   > **Note:** The provided sample keys are for development/testing purposes only. Generate your own key pair using edgeprotecttools or similar tools for production use.

#### Prerequisite for key generation

Infineon's Edge Protect Tools is a set of command line tools used to perform the functions needed for key signing, key generation, OEM certificate creation, device provisioning, and so on. These tools are executed through a shell tool. **Edge Protect Tools** executable is made available in the location *C:\Users\<username>\Infineon\Tools\ModusToolbox-Edge-Protect-Security-Suite-a.b.c\tools\edgeprotecttools* directory.

Add the executable path to the system environment path variable of the host PC.

To use Edge Protect Tools CLI, it is recommended to use "modus-shell", installed along with ModusToolbox&trade; located in the *ModusToolbox/tools_x.y* directory.

#### Generate the key

Choose one signing scheme and run the corresponding command to generate the key pair:

   - ML-DSA (ML-DSA-44 / ML-DSA-65 / ML-DSA-87)

      Choose the key type that matches the desired scheme (`ml-dsa-44` for ML-DSA-44, `ml-dsa-65` for ML-DSA-65, `ml-dsa-87` for ML-DSA-87):

      ```
      edgeprotecttools create-key --key-type ml-dsa-87 --format der --output keys/oem_img_sign_key_priv.der keys/oem_img_sign_key_pub.der
      ```

   - LMS (LMS_SHA256_M32_H10 with LMOTS_SHA256_N32_W8)

      ```
      edgeprotecttools create-key-lms --lms-type lms-sha256-m32-h10 --lmots-type lmots-sha256-n32-W8 --output keys/oem_img_sign_key_priv.der keys/oem_img_sign_key_pub.der
      ```

   - XMSS (XMSS_SHA2_10_256)

      ```
      edgeprotecttools create-key-xmss --xmss-type XMSS-SHA2_10_256 --output keys/oem_img_sign_key_priv.der keys/oem_img_sign_key_pub.der
      ```

   - ECDSA (ECDSA-256 / ECDSA-384 / ECDSA-521)

      Choose the curve that matches the desired key type (`ecdsa-p256` for ECDSA-256, `ecdsa-p384` for ECDSA-384, `ecdsa-p521` for ECDSA-521):

      ```
      edgeprotecttools create-key --key-type ecdsa-p256 --format der --output keys/oem_img_sign_key_priv.der keys/oem_img_sign_key_pub.der
      ```

The command produces two files:
   - oem_img_sign_key_priv.der (private key; used to sign images)
   - oem_img_sign_key_pub.der (public key; used to verify signatures)

   > **Note:** Keep the private key secure.

### 1.2 Import the image signing key

1. Open *\<app-directory>/Makefile*

2. Set **IMAGE_SIGNING_KEY_TYPE** variable to the signing scheme chosen (ML-DSA-44 / ML-DSA-65 / ML-DSA-87 / LMS_SHA256_M32_H10 / XMSS_SHA2_10_256 / ECDSA-256 / ECDSA-384 / ECDSA-521)
   ```
   IMAGE_SIGNING_KEY_TYPE=ML-DSA-87
   ```

3. Set **IMAGE_SIGNING_KEY** variable to the path of the private key for the chosen scheme and save the file.

   - If you generated your own key in [1.1](#11-generate-the-image-signing-key-optional), use that file (`../keys/oem_img_sign_key_priv.der`).
   - If you are using the provided sample keys, point to the matching file in the *keys/* folder. Each scheme has a `<prefix>_img_sign_key_priv.der` (private) and `<prefix>_img_sign_key_pub.der` (public), where `<prefix>` is one of `ml_dsa_44`, `ml_dsa_65`, `ml_dsa_87`, `lms`, `xmss`, `ecdsa_p256`, `ecdsa_p384`, or `ecdsa_p521`.

   For example, to use the provided ML-DSA-87 sample key:

   ```
   IMAGE_SIGNING_KEY=../keys/ml_dsa_87_img_sign_key_priv.der
   ```

### 1.3 Configure the EdgeProtect Bootloader for image validation

Configure the EdgeProtect Bootloader project (added to your workspace in [Operation](../README.md#operation) step 3) to validate the application images it boots and updates.

1. Configure the memory map

   1. Open *\<Workspace>/\<EdgeProtect_Bootloader>/common.mk*

   2. Set **MEMORY_MAP** variable to the path of the memory map JSON file *overwrite_multi3_flash.json* made available with this code example in *memory_maps* folder and save the file

      ```
      MEMORY_MAP = <Workspace>/<app-directory>/memory_map/overwrite_multi3_flash.json.
      ```

2. Configure EdgeProtect Bootloader features

   1. Open *\<Workspace>/\<EdgeProtect_Bootloader>/platforms/PSC3_P8/feature_config.json*

   2. Set the image validation scheme

       - `security_setup` > `validation_key_type` > `value` to  `LMS_SHA256_M32_H10` for LMS, `XMSS_SHA2_10_256` for XMSS,  `ML-DSA-44`/`ML-DSA-65`/`ML-DSA-87` for ML-DSA, or `ECDSA-256`/`ECDSA-384`/`ECDSA-521` for ECDSA

      > **Note:** The `validation_key_type` set here must match the `IMAGE_SIGNING_KEY_TYPE` used to sign the application images.

       - If the signing scheme is ML-DSA (`ML-DSA-44`/`ML-DSA-65`/`ML-DSA-87`), set `security_setup` > `ml_dsa_sig_hash` > `value` to `SHA256`.


   3. Set the image validation public key (use the public key that pairs with the private key used to sign the application images)

      - `security_setup` > `validation_key` > `value` to the path of the public key, relative to the bootloader project.
        - If you generated your own key in [1.1](#11-generate-the-image-signing-key-optional), use `../../<app-directory>/keys/oem_img_sign_key_pub.der`.
        - If you are using the provided sample keys, use the `<prefix>_img_sign_key_pub.der` that matches the private key selected in [1.2](#12-import-the-image-signing-key) (for example, `../../<app-directory>/keys/ml_dsa_87_img_sign_key_pub.der` for ML-DSA-87).

        > **Note:** Replace `<app-directory>` with the actual folder name of this code example in your workspace.

   4. Enable image validation during boot

      - `security_setup` > `validate_boot` > `value` to `true`

   5. Enable image validation during update

      - `security_setup` > `validate_upgrade` > `value` to `true`

   6. Save and close the file

   ```
    "security_setup": {
        "validation_key_type": {
            "description": "Type of the image validation key. Possible values: 'ECDSA-256', 'ECDSA-384', 'ECDSA-521' 'LMS_SHA256_M32_H10', 'XMSS_SHA2_10_256', 'ML-DSA-44', 'ML-DSA-65', 'ML-DSA-87'",
            "value": "ML-DSA-87"
        },
        "ml_dsa_sig_hash": {
            "description": "Hash function used for ML-DSA signature generation. Possible values: 'SHA256', 'SHA384', 'SHA512'",
            "value": "SHA256"
        },
        "validation_key": {
            "description": "Path to the image validation key. Example key path: ../keys/ecdsa-p256-pub.pem",
            "value": "../../<app-directory>/keys/ml_dsa_87_img_sign_key_pub.der"
        },
        "validate_boot": {
            "description": "Image validation during boot",
            "value": true
        },
        "validate_upgrade": {
            "description": "Image validation during upgrade",
            "value": true
        },
        
   ```

Once this setup is complete, return to [Operation](../README.md#operation) and continue from step 5.

## 2. Encrypted update

Encrypt the `UPDATE` image so it is delivered as ciphertext and decrypted by the bootloader while promoting it to the primary slot. Encryption is **off by default** and applies only to `UPDATE` images; `BOOT` images are executed in place and remain in plain text.

> **Note:** `EC256` requires **no device provisioning** — the private key is embedded in the bootloader. `KDF-CMAC` requires [ownership transfer](ownership_transfer.md) followed by provisioning the master key (see [Provision the KDF-CMAC key](#25-provision-the-kdf-cmac-key-kdf-cmac-only)).

### 2.1 Supported methods and constraints

- **Only `UPDATE` images are encrypted.** `IMG_TYPE=BOOT` images are executed in place and remain in plain text; the encryption switch is ignored for them.
- **Signing must stay enabled.** Encryption is layered on top of signing — the bootloader authenticates first, then decrypts. `validate_boot`/`validate_upgrade` must remain `true`.
- Two methods are available:

   **Table 3. Supported encryption methods**

   | Method (`IMAGE_ENCRYPTION_TYPE`) | Algorithm | Key used to encrypt | Where the decryption key lives |
   | :------------- | :------------------------------- | :-------------------------------- | :--------------------------------- |
   | `EC256`    | ECIES-P256 + AES-128-CTR (software) | EC P-256 **public** key (`.pem`)  | EC P-256 **private** key in the bootloader |
   | `KDF-CMAC` | Hardware KDF (Crypto Suite) + AES-128-CTR | 16-byte AES-128 **master** key (`.bin`) | Same master key provisioned via `raw_data_pc012` in `policy_oem_provisioning.json` |

- **`EC256` cannot be combined with `ECDSA-384` or `ECDSA-521` signing** — `edgeprotecttools` requires the signing and encryption keys to be the same EC type, and only P-256 image encryption exists. Use `KDF-CMAC` if signing with `ECDSA-384`/`ECDSA-521`.

   **Table 4. Signing scheme vs. encryption method compatibility**

   | `IMAGE_SIGNING_KEY_TYPE` | `EC256` encryption | `KDF-CMAC` encryption |
   | :-------------------------------------------- | :----------------: | :-------------------: |
   | ML-DSA-44 / ML-DSA-65 / ML-DSA-87 / LMS_SHA256_M32_H10 / XMSS_SHA2_10_256 | ✅ | ✅ |
   | ECDSA-256 | ✅ (sign and encrypt keys are both P-256) | ✅ |
   | ECDSA-384 | ❌ (no P-384 image encryption exists) | ✅ |
   | ECDSA-521 | ❌ (no P-521 image encryption exists) | ✅ |

### 2.2 Generate encryption keys (optional)

Sample encryption keys are provided in the *keys/* folder, so you can skip this subsection and use them directly. To generate your own keys (recommended for production), run the command matching your selected method from the `keys` folder:

- **EC256** – generate an EC P-256 key pair (public key encrypts, bootloader holds the private key). Sample keys `enc_ec256_priv.pem`/`enc_ec256_pub.pem` are already provided:

   ```
   edgeprotecttools create-key --key-type ECDSA-P256 --output enc_ec256_priv.pem enc_ec256_pub.pem
   ```

- **KDF-CMAC** – generate a 16-byte AES-128 master key (no sample master key is provided, so generate one):

   ```
   edgeprotecttools create-key --key-type AES128 --output aes128_enc_master_key.bin
   ```

### 2.3 Enable encryption in this code example

1. Open *\<Workspace>/\<app-directory>/Makefile*

2. Set the encryption variables:

   ```
   # Enable encryption (applies to UPDATE images only)
   IMAGE_ENCRYPTION=1

   # Select the method: EC256 or KDF-CMAC
   IMAGE_ENCRYPTION_TYPE=EC256

   # Key matching the method:
   #   EC256    -> EC P-256 public key (.pem)
   #   KDF-CMAC -> 16-byte AES-128 master key (.bin)
   IMAGE_ENCRYPTION_KEY=../keys/enc_ec256_pub.pem
   ```

   > **Note:** To disable encryption, set `IMAGE_ENCRYPTION=0`. No other change is required — the build automatically selects the plain (signed-only) update template.

### 2.4 Configure the EdgeProtect bootloader for decryption

1. Open *\<Workspace>/\<EdgeProtect_Bootloader>/platforms/PSC3_P8/feature_config.json*

2. In `security_setup`, enable encryption and select the matching method:

   - `image_encryption` > `value` to `true`
   - `encryption_type` > `value` to `EC256` or `KDF-CMAC` (must match `IMAGE_ENCRYPTION_TYPE`)
   - For `EC256`: set `encryption_key` > `value` to the EC P-256 **private** key that pairs with the public key used above (for example `../../<app-directory>/keys/enc_ec256_priv.pem`)
   - For `KDF-CMAC`: leave `encryption_key` empty. The KDF-CMAC master key is provisioned separately through `policy/policy_oem_provisioning.json` using `raw_data_pc012`, as described in the next subsection.

   ```
   "image_encryption": {
      "description": "Activate/deactivate image encryption",
      "value": true
   },
   "encryption_type": {
      "description": "Encryption method: 'KDF-CMAC' (PSC3_P8 hardware key derivation) or 'EC256' (standard elliptic curve)",
      "value": "EC256"
   },
   "encryption_key": {
      "description": "Key for EC256 encryption. Leave empty for KDF-CMAC (uses master key from SFLASH)",
      "value": "../../<app-directory>/keys/enc_ec256_priv.pem"
   },
   ```

   > **Note:** `image_encryption`, `encryption_type`, and the key must be consistent between the bootloader and this code example, otherwise the bootloader fails to decrypt the staged image and rejects the update.

3. Rebuild and reprogram the EdgeProtect bootloader so the new configuration takes effect.

### 2.5 Provision the KDF-CMAC key (KDF-CMAC only)

If `IMAGE_ENCRYPTION_TYPE=KDF-CMAC`, provision the same AES-128 master key used to encrypt the update image into the device through the OEM provisioning policy.

> **Note:** Provisioning requires device ownership. Complete [ownership transfer](ownership_transfer.md) first if you have not already.

1. Open *\<app-directory>/policy/policy_oem_provisioning.json*

2. Set `raw_data_pc012` > `value` to the generated KDF-CMAC key file:

   ```
   "raw_data_pc012": {
     "description": "Path to a binary file containing custom data accessible in PC0, PC1, and PC2. Up to 84 bytes",
     "value": "../keys/aes128_enc_master_key.bin"
   }
   ```

3. Reprovision the device so the key is programmed into the device:

   ```
   edgeprotecttools -t psoc_c3x8 provision-device -p policy/policy_oem_provisioning.json --ifx-oem-cert keys/oem_cert_development.bin --key keys/oem_dev_priv_key.pem
   ```

4. Ensure the same key file is used in this code example:

   ```
   IMAGE_ENCRYPTION_TYPE=KDF-CMAC
   IMAGE_ENCRYPTION_KEY=../keys/aes128_enc_master_key.bin
   ```

   > **Note:** The same `aes128_enc_master_key.bin` must be used for both `IMAGE_ENCRYPTION_KEY` in this code example and `raw_data_pc012` during provisioning; otherwise, the bootloader cannot decrypt the staged update image.

Once this setup is complete, return to [Operation](../README.md#operation) and repeat steps 6–8 to build and download the now signed-and-encrypted update image.

## 3. Secure boot of the EdgeProtect Bootloader

By default, the device's BootROM launches the EdgeProtect Bootloader (EPB) without authenticating it. This setup enables **secure boot** so that the BootROM authenticates the EPB image itself before transferring control to it. It is independent of the application-image authentication that the EPB performs ([Section 1](#1-secure-boot-and-update-of-the-application)), and can be enabled on its own.

Enabling secure boot of the EPB requires three things, all tied to the OEM key established during [ownership transfer](ownership_transfer.md):

- The EPB image must be **signed** with the OEM private key (`oem_dev_priv_key.pem`).
- The corresponding OEM public key must be **provisioned** into the device (this happens during [ownership transfer](ownership_transfer.md)).
- The OEM policy must select `SECURE_APP` and describe the **EPB region** in `boot_app_layout`.

> **Note:** This demonstration requires device ownership. If you have not already completed [ownership transfer](ownership_transfer.md) (for example, if you skipped the encrypted update demonstration or used `EC256` instead of `KDF-CMAC`), complete it first.

1. Sign the EdgeProtect Bootloader image. In the *EdgeProtect Bootloader* project, build and program the bootloader in signed mode using the OEM private key created during ownership transfer:

    ```
    make program BOOT_MODE=signed OEM_KEY_FILE=../../<app-directory>/keys/oem_dev_priv_key.pem BOOT_RECORD_VALUE=B_Bootloader
    ```

    > **Note:** For more details on building the bootloader in signed mode, see the "Configuring the bootloader for Secure Lifecycle Stage (LCS)" section of the EdgeProtect Bootloader README, and the provisioning guide (AN243370).

2. In the OEM policy (*policy/policy_oem_provisioning.json*), set `device_policy` > `boot` > `boot_cfg_id` > `value` to `SECURE_APP`:

    ```
    "boot": {
      "boot_cfg_id": {
        "description": "A behavior for BOOT_APP_LAYOUT (BOOT_SIMPLE_APP applicable to NORMAL_PROVISIONED only)",
        "applicable_conf": "SIMPLE_APP, SECURE_APP, DUAL_BANK_SIMPLE_APP, DUAL_BANK_SECURE_APP, PROT_FW",
        "value": "SECURE_APP"
      },
    ```

3. Update `device_policy` > `boot` > `boot_app_layout` so it describes the **EdgeProtect Bootloader region** — the image the BootROM authenticates and launches. The EPB occupies address `0x32000000` with size `0x1B000`, as defined by `bootloader` > `bootloader_area` in *memory_map/overwrite_multi3_flash.json*.

    ```
      "boot_app_layout": {
        "description": "The memory layout for the applications defined by BOOT_CFG_ID. 0x32000000 - 0x33FFFFFF for secure addresses; 0x22000000 - 0x23FFFFFF for non-secure addresses",
        "value": [
          {
            "address": "0x32000000",
            "size": "0x1B000"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          }
        ]
      },
    ```

4. Once the policy is updated, provision the device with the updated policy:

    ```
    edgeprotecttools -t psoc_c3x8 provision-device -p policy/policy_oem_provisioning.json --ifx-oem-cert keys/oem_cert_development.bin --key keys/oem_dev_priv_key.pem
    ```

Once this setup is complete, return to [Operation](../README.md#operation) and repeat steps 5–8, programming the signed EdgeProtect Bootloader (from step 1) in place of the standard bootloader. On each boot, the BootROM authenticates the EPB image and transfers control to it only if authentication succeeds.

### 3.1 Restore the device

Once you have enabled secure boot of the EdgeProtect Bootloader, the device expects a signed EPB image and will not boot an unsigned one. Follow these steps to return the device to a general-purpose state so it can run other (non-secure-boot) code examples. Revert the changes to the policy file (*policy_oem_provisioning.json*), reprovision the device.

1. In the OEM policy, set the following fields:

   - `device_policy` > `boot` > `boot_cfg_id` > `value` to `SIMPLE_APP`

    ```
    "boot": {
      "boot_cfg_id": {
        "description": "A behavior for BOOT_APP_LAYOUT (BOOT_SIMPLE_APP applicable to NORMAL_PROVISIONED only)",
        "applicable_conf": "SIMPLE_APP, SECURE_APP, DUAL_BANK_SIMPLE_APP, DUAL_BANK_SECURE_APP, PROT_FW",
        "value": "SIMPLE_APP"
      },
    ```

2. In the OEM policy, revert `device_policy` > `boot` > `boot_app_layout` back to its default (optional):

    ```
      "boot_app_layout": {
        "description": "The memory layout for the applications defined by BOOT_CFG_ID. 0x32000000 - 0x33FFFFFF for secure addresses; 0x22000000 - 0x23FFFFFF for non-secure addresses",
        "value": [
          {
            "address": "0x32000000",
            "size": "0x40000"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          },
          {
            "address": "0x00000000",
            "size": "0x00"
          }
        ]
      },
    ```

3. Once the policy changes are reverted, provision the device.

    ```
    edgeprotecttools -t psoc_c3x8 provision-device -p policy/policy_oem_provisioning.json --ifx-oem-cert keys/oem_cert_development.bin --key keys/oem_dev_priv_key.pem
    ```
