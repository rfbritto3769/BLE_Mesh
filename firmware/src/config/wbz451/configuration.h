/*******************************************************************************
  System Configuration Header

  File Name:
    configuration.h

  Summary:
    Build-time configuration header for the system defined by this project.

  Description:
    An MPLAB Project may have multiple configurations.  This file defines the
    build-time options for a single configuration.

  Remarks:
    This configuration header must not define any prototypes or data
    definitions (or include any files that do).  It only provides macro
    definitions for build-time configuration options

*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
/*  This section Includes other configuration headers necessary to completely
    define this configuration.
*/

#include "user.h"
#include "device.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: System Configuration
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: System Service Configuration
// *****************************************************************************
// *****************************************************************************

#define SYS_DEBUG_ENABLE
#define SYS_DEBUG_GLOBAL_ERROR_LEVEL       SYS_ERROR_INFO
#define SYS_DEBUG_BUFFER_DMA_READY
#define SYS_DEBUG_USE_CONSOLE


#define SYS_CONSOLE_DEVICE_MAX_INSTANCES   			(1U)
#define SYS_CONSOLE_UART_MAX_INSTANCES 	   			(1U)
#define SYS_CONSOLE_USB_CDC_MAX_INSTANCES 	   		(0U)
#define SYS_CONSOLE_PRINT_BUFFER_SIZE        		(200U)


#define SYS_CONSOLE_INDEX_0                       0






// *****************************************************************************
// *****************************************************************************
// Section: Driver Configuration
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Middleware & Other Library Configuration
// *****************************************************************************
// *****************************************************************************

/*** wolfCrypt Library Configuration ***/
#define MICROCHIP_PIC32
#define MICROCHIP_MPLAB_HARMONY
#define MICROCHIP_MPLAB_HARMONY_3
#define HAVE_MCAPI
#define SIZEOF_LONG_LONG 8
#define WOLFSSL_USER_IO
#define NO_WRITEV
#define NO_FILESYSTEM
#define USE_FAST_MATH
#define NO_PWDBASED
#define HAVE_MCAPI
#define WOLF_CRYPTO_CB  // provide call-back support
#define WOLFCRYPT_ONLY
#define WOLFSSL_HAVE_MCHP_HW_CRYPTO
// ---------- CRYPTO HARDWARE MANIFEST START ----------
#define WOLFSSL_HAVE_MCHP_HW_CRYPTO_ECC_HW_PUKCC
#define WOLFSSL_HAVE_MCHP_HW_CRYPTO_AES_HW_U2238
#define WOLFSSL_HAVE_MCHP_HW_CRYPTO_SHA_HW_11105
#define WOLFSSL_HAVE_MCHP_HW_CRYPTO_SHA_HW_U2010
#define WOLFSSL_HAVE_MCHP_HW_CRYPTO_TRNG_HW_U2242
#define WOLFSSL_HAVE_MCHP_HW_CRYPTO_RSA_HW_PUKCC
// ---------- CRYPTO HARDWARE MANIFEST END ----------
#undef WOLFSSL_HAVE_MIN
#undef WOLFSSL_HAVE_MAX
// ---------- FUNCTIONAL CONFIGURATION START ----------
#define WOLFSSL_AES_SMALL_TABLES
#define NO_MD4
#define NO_MD5
#define NO_SHA // specifically, no SHA1 (legacy name)
#define NO_SHA256
#define NO_SHA224
#define NO_HMAC
#define NO_DES3
#define WOLFSSL_AES_128
#define NO_AES_192 // not supported by HW accelerator
#define NO_AES_256 // not supported by HW accelerator
#define WOLFSSL_AES_DIRECT
#define HAVE_AES_DECRYPT
#define WOLFSSL_HAVE_MCHP_HW_AES_DIRECT
#define HAVE_AES_CBC
#define WOLFSSL_HAVE_MCHP_HW_AES_CBC
#define NO_RC4
#define NO_HC128
#define NO_RABBIT
#define NO_DH
#define NO_DSA
#define NO_RSA
#define NO_DEV_RANDOM
#define WC_NO_RNG
#define WC_NO_HASHDRBG
#define WC_NO_HARDEN
#define SINGLE_THREADED
#define NO_ASN
#define NO_SIG_WRAPPER
#define NO_ERROR_STRINGS
#define WOLFSSL_MAX_ERROR_SZ 38 // Fix Mandatory Misra 21.18 caused by removing error strings with defining NO_ERROR_STRINGS
#define NO_WOLFSSL_MEMORY
// ---------- FUNCTIONAL CONFIGURATION END ----------



// *****************************************************************************
// *****************************************************************************
// Section: Application Configuration
// *****************************************************************************
// *****************************************************************************
#define CONFIG_BLE_GAP_DEV_NAME_VALUE                    {"DIMMER_01"}
// GAP Service option
#define CONFIG_BLE_GAP_SVC_DEV_NAME_WRITE                false             /* Enable Device Name Write Property */
#define CONFIG_BLE_GAP_SVC_APPEARANCE                    0x0                 /* Appearance */
#define CONFIG_BLE_GAP_SVC_PERI_PRE_CP                   false                /* Enable Peripheral Preferred Connection Parameters */

//Legacy Advertising set
#define CONFIG_BLE_GAP_ADV_DATA                      {0x02, 0x01, 0x06, 0x0A, 0x09, 0x44, 0x49, 0x4D, 0x4D, 0x45, 0x52, 0x5F, 0x30, 0x31}
#define CONFIG_BLE_GAP_ADV_DATA_ORIG_LEN             14
#define CONFIG_BLE_GAP_SCAN_RSP_DATA                 {0x11, 0x07, 0x49, 0x53, 0x53, 0x43, 0xFE, 0x7D, 0x4A, 0xE5, 0x8F, 0xA9, 0x9F, 0xAF, 0xD2, 0x05, 0xE4, 0x55}
#define CONFIG_BLE_GAP_SCAN_RSP_DATA_ORIG_LEN        18
#define CONFIG_BLE_GAP_ADV_TX_PWR                    9 /* Advertising TX Power */
#define CONFIG_BLE_GAP_ADV_INTERVAL_MIN              32 /* Advertising Interval Min */
#define CONFIG_BLE_GAP_ADV_INTERVAL_MAX              32 /* Advertising Interval Max */
#define CONFIG_BLE_GAP_ADV_TYPE                      BLE_GAP_ADV_TYPE_ADV_IND /* Advertising Type */
#define CONFIG_BLE_GAP_ADV_CHANNEL_MAP               BLE_GAP_ADV_CHANNEL_ALL /* Advertising Channel Map */
#define CONFIG_BLE_GAP_ADV_FILT_POLICY               BLE_GAP_ADV_FILTER_DEFAULT /* Advertising Filter Policy */

// Configure scan parameters
#define CONFIG_BLE_GAP_SCAN_TYPE                 BLE_GAP_SCAN_TYPE_ACTIVE_SCAN      /* Scan Type - active to get names */
#define CONFIG_BLE_GAP_SCAN_INTERVAL             160      /* Scan Interval */
#define CONFIG_BLE_GAP_SCAN_WINDOW               32      /* Scan Window */
#define CONFIG_BLE_GAP_SCAN_FILT_POLICY          BLE_GAP_SCAN_FP_ACCEPT_ALL       /* Scan Filter Policy */
#define CONFIG_BLE_GAP_SCAN_DIS_CHANNEL_MAP      0      /* Disable specific channel during scanning */

#define CONFIG_BLE_GAP_CONN_TX_PWR               15 /* Connection TX Power */

// Configure SMP parameters
#define CONFIG_BLE_SMP_IOCAP_TYPE   BLE_SMP_IO_NOINPUTNOOUTPUT  /* IO Capability */
#define CONFIG_BLE_SMP_OPTION       (0 |BLE_SMP_OPTION_BONDING |BLE_SMP_OPTION_SECURE_CONNECTION) /* Authentication Setting */

// Configure BLE_DM middleware parameters
#define CONFIG_BLE_DM_SEC_AUTO_ACCEPT      true /* Auto Accept Security Request */
#define CONFIG_BLE_DM_AUTO_REPLY_UPD_REQ   true      /* Auto Accept Connection Parameter Update Request */
#define CONFIG_BLE_DM_MIN_CONN_INTERVAL    6    /* Minimum Connection Interval */
#define CONFIG_BLE_DM_MAX_CONN_INTERVAL    3200   /* Maximum Connection Interval */
#define CONFIG_BLE_DM_MIN_CONN_LATENCY     0    /* Minimum Connection Latency */
#define CONFIG_BLE_DM_MAX_CONN_LATENCY     499    /* Maximum Connection Latency */


// Configure BLE_DD middleware parameters
#define CONFIG_BLE_GCM_DD_WAIT_FOR_SEC          false /* Wait for security */
#define CONFIG_BLE_GCM_DD_INIT_DISC_IN_CNTRL    true /* Init discovery with central role */
#define CONFIG_BLE_GCM_DD_INIT_DISC_IN_PER      false /* Init discovery with peripheral role */
#define CONFIG_BLE_GCM_DD_DIS_CONN_DISC         false

// Configuration of Service TRS
#define CONFIG_BLE_SVC_TRS_UUID_MCHP_PROPRIETARY_SERVICE_16        0x55,0xE4,0x05,0xD2,0xAF,0x9F,0xA9,0x8F,0xE5,0x4A,0x7D,0xFE,0x43,0x53,0x53,0x49    /* Service UUID */
#define CONFIG_BLE_SVC_TRS_UUID_MCHP_TRANS_TX_16                   0x16,0x96,0x24,0x47,0xC6,0x23,0x61,0xBA,0xD9,0x4B,0x4D,0x1E,0x43,0x53,0x53,0x49    /* TX Characteristic UUID */
#define CONFIG_BLE_SVC_TRS_UUID_MCHP_TRANS_RX_16                   0xB3,0x9B,0x72,0x34,0xBE,0xEC,0xD4,0xA8,0xF4,0x43,0x41,0x88,0x43,0x53,0x53,0x49    /* RX Characteristic UUID */
#define CONFIG_BLE_SVC_TRS_UUID_MCHP_TRANS_CTRL_16                 0x7E,0x3B,0x07,0xFF,0x1C,0x51,0x49,0x2F,0xB3,0x39,0x8A,0x4C,0x43,0x53,0x53,0x49    /* CP Characteristic UUID */

#define CONFIG_APP_BLE_MAXIMUM_NUMBER_OF_LINKS               6



#define CONFIG_APP_BLE_USERDATA_APPEND_INDEX      14U
#define CONFIG_APP_BLE_SVCDATA_LENGTH_INDEX       0U
#define CONFIG_APP_BLE_SVCDATA_APPEND_INDEX       14U
#define CONFIG_APP_BLE_RSP_USERDATA_APPEND_INDEX  18U
#define CONFIG_APP_BLE_RSP_SVCDATA_LENGTH_INDEX   0U
#define CONFIG_APP_BLE_RSP_SVCDATA_APPEND_INDEX   0U

#define PIC32BZ2



//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif // CONFIGURATION_H
/*******************************************************************************
 End of File
*/
