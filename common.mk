################################################################################
# \file common.mk
# \version 1.0
#
# \brief
# Settings shared across all projects.
#
################################################################################
# \copyright
# (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG.  SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

MTB_TYPE=PROJECT

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make modlibs' from command line), which will also update Eclipse IDE launch
# configurations. If TARGET is manually edited, ensure TARGET_<BSP>.mtb with a
# valid URL exists in the application, run 'make getlibs' to fetch BSP contents
# and update or regenerate launch configurations for your IDE.
TARGET=KIT_PSC3M8_EVK

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC provided with ModusToolbox IDE
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
# 
# If CONFIG is manually edited, ensure to update or regenerate launch configurations 
# for your IDE.
CONFIG=Debug


# Set Python Path
PYTHON=python


# Image signing key type.
# Allowed values:
#   Post-quantum : XMSS_SHA2_10_256, LMS_SHA256_M32_H10, ML-DSA-44 , ML-DSA-65, ML-DSA-87
#   Classic ECC  : ECDSA-256, ECDSA-384, ECDSA-521
IMAGE_SIGNING_KEY_TYPE=ML-DSA-87

# Path to Image signing private key (must match IMAGE_SIGNING_KEY_TYPE).
# Example private keys provided in the keys folder:
#   XMSS_SHA2_10_256   : ../keys/xmss_img_sign_key_priv.der
#   LMS_SHA256_M32_H10 : ../keys/lms_img_sign_key_priv.der
#   ML-DSA-44          : ../keys/ml_dsa_44_img_sign_key_priv.der
#   ML-DSA-65          : ../keys/ml_dsa_65_img_sign_key_priv.der
#   ML-DSA-87          : ../keys/ml_dsa_87_img_sign_key_priv.der
#   ECDSA-256          : ../keys/ecdsa_p256_img_sign_key_priv.der
#   ECDSA-384          : ../keys/ecdsa_p384_img_sign_key_priv.der
#   ECDSA-521          : ../keys/ecdsa_p521_img_sign_key_priv.der
IMAGE_SIGNING_KEY=../keys/ml_dsa_87_img_sign_key_priv.der


# Set Image type as BOOT or UPDATE
#  * BOOT   : Images will generated for Primary slots (Suitable for directly Programming the image through MiniProg)
#  * UPDATE : Images will generated for Secondary slots (To be used for DFU over serial interface)
IMG_TYPE=BOOT


# Image encryption (Edge Protect Bootloader feature).
# The Edge Protect Bootloader can decrypt the staged firmware while promoting it
# from the secondary (upgrade) slot to the primary slot. ONLY UPDATE images are
# encrypted; BOOT images programmed directly to the primary slot stay in plain
# text, so this setting is ignored when IMG_TYPE=BOOT.
#
# The signing scheme constrains the encryption method:
#   * KDF-CMAC works with ANY IMAGE_SIGNING_KEY_TYPE.
#   * EC256 works with ML-DSA-44, ML-DSA-65, ML-DSA-87 / LMS / XMSS / ECDSA-256 only. It CANNOT be
#     combined with ECDSA-384 or ECDSA-521: edgeprotecttools requires an EC signing
#     key to be paired with an encryption key of the SAME type, and only EC
#     P-256 image encryption exists (no P-384/P-521). EC256 + ECDSA-384/521
#     fails the post-build sign step with
#     "Signing and encryption must use the same type of key" -> use KDF-CMAC.
# Image validation (signing) must remain enabled.
#
# IMAGE_ENCRYPTION:
#   * 0 : Encryption disabled (update images are signed only)
#   * 1 : Encryption enabled (update images are signed and encrypted)
IMAGE_ENCRYPTION=0

# Encryption method (used only when IMAGE_ENCRYPTION=1). Allowed values:
#   * EC256    : Software ECIES-P256 + AES-128-CTR. The bootloader holds the
#                EC P-256 private key; the image is encrypted with the matching
#                public key (IMAGE_ENCRYPTION_KEY below).
#   * KDF-CMAC : Hardware key derivation (Crypto Suite). The image is encrypted
#                with a 16-byte AES-128 master key that must also be provisioned
#                into the device SFLASH_USER_ROW (see README).
IMAGE_ENCRYPTION_TYPE=EC256

# Path to the encryption key used to encrypt the update image (used only when
# IMAGE_ENCRYPTION=1). Must match IMAGE_ENCRYPTION_TYPE:
#   * EC256    : EC P-256 PUBLIC key (pairs with the bootloader's private key)
#                Example: ../keys/enc_ec256_pub.pem
#   * KDF-CMAC : 16-byte AES-128 master key (.bin), identical to the key
#                provisioned in the device SFLASH
#                Example: ../keys/aes128_enc_master_key.bin
IMAGE_ENCRYPTION_KEY=../keys/enc_ec256_pub.pem


# Set Image Version and Build Number
ifeq ($(IMG_TYPE),BOOT)
IMG_VER_MAJOR=1		# 0 - 255
IMG_VER_MINOR=0		# 0 - 255
IMG_REVISION=0		# 0 - 65535
IMG_BUILD_NO=0		# 0 - 65535
else
IMG_VER_MAJOR=2		# 0 - 255
IMG_VER_MINOR=0		# 0 - 255
IMG_REVISION=0		# 0 - 65535
IMG_BUILD_NO=1		# 0 - 65535
endif #$(IMG_TYPE)


# MCU boot header size
MCUBOOT_HEADER_SIZE=0x400

# PPCA Core LED toggle interval setting
ifeq ($(IMG_TYPE),BOOT)
PPCA_LED_TOGGLE_INTERVAL_MS=1000
else
PPCA_LED_TOGGLE_INTERVAL_MS=500
endif #$(IMG_TYPE)


# Config file for postbuild sign and merge operations.
# NOTE : Check the JSON file for the command parameters
# Encryption applies to UPDATE images only; the encrypted templates add the
# edgeprotecttools encryption parameters per signing method.
ifeq ($(IMG_TYPE),BOOT)
COMBINE_SIGN_JSON?=configs/boot_image.json
else
ifeq ($(IMAGE_ENCRYPTION),1)
ifeq ($(IMAGE_ENCRYPTION_TYPE),KDF-CMAC)
COMBINE_SIGN_JSON?=configs/update_image_enc_kdf_cmac.json
else
# EC256 image encryption only supports P-256 / non-EC signing keys. edgeprotecttools
# requires an EC signing key to be paired with an encryption key of the
# SAME type, and only EC P-256 image encryption exists. Reject the unsupported
# EC256 + ECDSA-384/ECDSA-521 combination here with a clear message instead of
# failing later in the post-build sign step.
ifneq ($(filter ECDSA-384 ECDSA-521,$(IMAGE_SIGNING_KEY_TYPE)),)
$(error Unsupported config: IMAGE_ENCRYPTION_TYPE=EC256 cannot be combined with IMAGE_SIGNING_KEY_TYPE=$(IMAGE_SIGNING_KEY_TYPE). EC256 image encryption is P-256 only and edgeprotecttools requires a matching signing key type. Use IMAGE_ENCRYPTION_TYPE=KDF-CMAC to encrypt images signed with ECDSA-384/ECDSA-521, or sign with ECDSA-256/ML-DSA-44, ML-DSA-65, ML-DSA-87/LMS/XMSS to keep EC256)
endif #ECDSA-384/521 + EC256 guard
COMBINE_SIGN_JSON?=configs/update_image_enc_ec256.json
endif #$(IMAGE_ENCRYPTION_TYPE)
else
COMBINE_SIGN_JSON?=configs/update_image.json
endif #$(IMAGE_ENCRYPTION)
endif #$(IMG_TYPE)

include ../common_app.mk
