/*******************************************************************************
  Application Utility Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_utility.c

  Summary:
    This file contains the utility functions for BLE project.

  Description:
    This file contains the utility functions for BLE project.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries.
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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "definitions.h"
#include "app_utility.h"
#include "ble_gap.h"
#include "mba_error_defs.h"

// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: File Scope or Global Data 
// *****************************************************************************
// *****************************************************************************
static uint8_t appBleAdvData[BLE_GAP_ADV_MAX_LENGTH] = CONFIG_BLE_GAP_ADV_DATA;
static uint8_t appBleAdvSvcDataLen = 0U;
static uint8_t appBleAdvUserDataLen = 0U;

static uint8_t appBleScanRspData[BLE_GAP_ADV_MAX_LENGTH] = CONFIG_BLE_GAP_SCAN_RSP_DATA;
static uint8_t appBleScanRspSvcDataLen = 0U;
static uint8_t appBleScanRspUserDataLen = 0U;


// *****************************************************************************
// *****************************************************************************
// Section: Local Functions 
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Interface Functions 
// *****************************************************************************
// *****************************************************************************
uint8_t APP_UTILITY_UpdateBleAdvServiceData(uint8_t* p_svcData,uint8_t svcDataLen)
{
    BLE_GAP_AdvDataParams_T bleAdvData;
    uint8_t error  = APP_UTILITY_SUCCESS;
    (void)memset(bleAdvData.advData,0x00,BLE_GAP_ADV_MAX_LENGTH);
    bleAdvData.advLen = CONFIG_BLE_GAP_ADV_DATA_ORIG_LEN + svcDataLen + appBleAdvUserDataLen;

    if(((BLE_GAP_ADV_MAX_LENGTH+1U) < bleAdvData.advLen) || (CONFIG_APP_BLE_SVCDATA_APPEND_INDEX == 0U) || (CONFIG_BLE_GAP_ADV_DATA_ORIG_LEN == 0U))
    {
        error = APP_UTILITY_INVALID_LEN;
    }
    else 
    {
        (void)memcpy(&bleAdvData.advData[0],&appBleAdvData[0], CONFIG_APP_BLE_SVCDATA_APPEND_INDEX);
        (void)memcpy(&bleAdvData.advData[CONFIG_APP_BLE_SVCDATA_APPEND_INDEX],p_svcData, svcDataLen);
        (void)memcpy(&bleAdvData.advData[CONFIG_APP_BLE_SVCDATA_APPEND_INDEX + svcDataLen], &appBleAdvData[CONFIG_APP_BLE_SVCDATA_APPEND_INDEX + appBleAdvSvcDataLen],((CONFIG_BLE_GAP_ADV_DATA_ORIG_LEN + appBleAdvUserDataLen) - CONFIG_APP_BLE_SVCDATA_APPEND_INDEX));

        bleAdvData.advData[CONFIG_APP_BLE_SVCDATA_LENGTH_INDEX] +=(svcDataLen - appBleAdvSvcDataLen);

        if(MBA_RES_SUCCESS == BLE_GAP_SetAdvData(&bleAdvData))
        {
            (void)memcpy(appBleAdvData, bleAdvData.advData, bleAdvData.advLen);
            appBleAdvSvcDataLen = svcDataLen;
        }
        else
        {
            error = APP_UTILITY_FAIL;
        }
    }

    return error;
}

uint8_t APP_UTILITY_UpdateBleAdvUserData(uint8_t* p_userData,uint8_t userDataLen)
{
    BLE_GAP_AdvDataParams_T bleAdvData;
    uint8_t error  = APP_UTILITY_SUCCESS;
    (void)memset(bleAdvData.advData,0,BLE_GAP_ADV_MAX_LENGTH);
    bleAdvData.advLen = CONFIG_BLE_GAP_ADV_DATA_ORIG_LEN + appBleAdvSvcDataLen + userDataLen;
    
    if((BLE_GAP_ADV_MAX_LENGTH+1U) < bleAdvData.advLen)
    {
        error = APP_UTILITY_INVALID_LEN;
    }
    else 
    {   
        (void)memcpy(&bleAdvData.advData[0],&appBleAdvData[0], CONFIG_APP_BLE_USERDATA_APPEND_INDEX + appBleAdvSvcDataLen);
        (void)memcpy(&bleAdvData.advData[CONFIG_APP_BLE_USERDATA_APPEND_INDEX  + appBleAdvSvcDataLen],p_userData, userDataLen);

         if(MBA_RES_SUCCESS == BLE_GAP_SetAdvData(&bleAdvData))
         {
            (void)memcpy(appBleAdvData, bleAdvData.advData, bleAdvData.advLen);
            appBleAdvUserDataLen = userDataLen;
         }
         else
         {
             error = APP_UTILITY_FAIL;
         }
    }

    return error;
}

uint8_t APP_UTILITY_UpdateBleScanRspServiceData(uint8_t* p_svcData,uint8_t svcDataLen)
{
    BLE_GAP_AdvDataParams_T bleScanRspData;
    uint8_t error  = APP_UTILITY_SUCCESS;
    (void)memset(bleScanRspData.advData,0x00,BLE_GAP_ADV_MAX_LENGTH);
    bleScanRspData.advLen = CONFIG_BLE_GAP_SCAN_RSP_DATA_ORIG_LEN + svcDataLen + appBleScanRspUserDataLen;

    if(((BLE_GAP_ADV_MAX_LENGTH+1U) < bleScanRspData.advLen) || (CONFIG_APP_BLE_RSP_SVCDATA_APPEND_INDEX == 0U) || (CONFIG_BLE_GAP_ADV_DATA_ORIG_LEN == 0U))
    {
        error = APP_UTILITY_INVALID_LEN;
    }
    else 
    {
        (void)memcpy(&bleScanRspData.advData[0],&appBleScanRspData[0], CONFIG_APP_BLE_RSP_SVCDATA_APPEND_INDEX);
        (void)memcpy(&bleScanRspData.advData[CONFIG_APP_BLE_RSP_SVCDATA_APPEND_INDEX],p_svcData, svcDataLen);
        (void)memcpy(&bleScanRspData.advData[CONFIG_APP_BLE_RSP_SVCDATA_APPEND_INDEX + svcDataLen], &appBleScanRspData[CONFIG_APP_BLE_RSP_SVCDATA_APPEND_INDEX + appBleScanRspSvcDataLen],((CONFIG_BLE_GAP_SCAN_RSP_DATA_ORIG_LEN + appBleScanRspUserDataLen) - CONFIG_APP_BLE_RSP_SVCDATA_APPEND_INDEX));

        bleScanRspData.advData[CONFIG_APP_BLE_RSP_SVCDATA_LENGTH_INDEX] +=(svcDataLen - appBleScanRspSvcDataLen);

        if(MBA_RES_SUCCESS == BLE_GAP_SetScanRspData(&bleScanRspData))
        {
            (void)memcpy(appBleScanRspData, bleScanRspData.advData, bleScanRspData.advLen);
            appBleScanRspSvcDataLen = svcDataLen;
        }
        else
        {
            error = APP_UTILITY_FAIL;
        }
    }

    return error;
}

uint8_t APP_UTILITY_UpdateBleScanRspUserData(uint8_t* p_userData,uint8_t userDataLen)
{
    BLE_GAP_AdvDataParams_T bleScanRspData;
    uint8_t error  = APP_UTILITY_SUCCESS;
    (void)memset(bleScanRspData.advData,0,BLE_GAP_ADV_MAX_LENGTH);
    bleScanRspData.advLen = CONFIG_BLE_GAP_SCAN_RSP_DATA_ORIG_LEN + appBleScanRspSvcDataLen + userDataLen;
    
    if((BLE_GAP_ADV_MAX_LENGTH+1U) < bleScanRspData.advLen)
    {
        error = APP_UTILITY_INVALID_LEN;
    }
    else 
    {   
        (void)memcpy(&bleScanRspData.advData[0],&appBleScanRspData[0], CONFIG_APP_BLE_RSP_USERDATA_APPEND_INDEX + appBleScanRspSvcDataLen);
        (void)memcpy(&bleScanRspData.advData[CONFIG_APP_BLE_RSP_USERDATA_APPEND_INDEX  + appBleScanRspSvcDataLen],p_userData, userDataLen);

         if(MBA_RES_SUCCESS == BLE_GAP_SetScanRspData(&bleScanRspData))
         {
            (void)memcpy(appBleScanRspData, bleScanRspData.advData, bleScanRspData.advLen);
            appBleScanRspUserDataLen = userDataLen;
         }
         else
         {
             error = APP_UTILITY_FAIL;
         }
    }

    return error;
}
