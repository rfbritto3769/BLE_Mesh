/*******************************************************************************
  Application Utility Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_utility.h

  Summary:
    This file contains the application utility functions for BLE project.

  Description:
    This file contains the application utility functions for BLE project.
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

#ifndef _APP_UTILITY_H    /* Guard against multiple inclusion */
#define _APP_UTILITY_H


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "configuration.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif


// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
#define APP_UTILITY_SUCCESS       0U
#define APP_UTILITY_INVALID_LEN   1U
#define APP_UTILITY_FAIL          2U

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Function Prototypes
// *****************************************************************************
// *****************************************************************************
/**@brief append data on ble service data of advertising packet.
 *
 * @param[in] p_svcData                 Pointer to the uint8_t buffer.
 * @param[in] svcDataLen                Length of the buffer.
 *
 * @retval APP_UTILITY_SUCCESS          Successfully appended to service data.
 * @retval APP_UTILITY_INVALID_LEN      Append not allowed due to invalid length.
 * @retval APP_UTILITY_FAIL             Append failure due to stack API return failed.
*/
uint8_t APP_UTILITY_UpdateBleAdvServiceData(uint8_t* p_svcData,uint8_t svcDataLen);

/**@brief append data on ble user data of advertising packet.
 *
 * @param[in] p_userData                Pointer to the uint8_t buffer.
 * @param[in] userDataLen               Length of the buffer.
 *
 * @retval APP_UTILITY_SUCCESS          Successfully appended to user data.
 * @retval APP_UTILITY_INVALID_LEN      Append not allowed due to invalid length.
 * @retval APP_UTILITY_FAIL             Append failure due to stack API return failure.
*/
uint8_t APP_UTILITY_UpdateBleAdvUserData(uint8_t* p_userData,uint8_t userDataLen);

/**@brief append data on ble service data of scan response packet.
 *
 * @param[in] p_svcData                 Pointer to the uint8_t buffer.
 * @param[in] svcDataLen                Length of the buffer.
 *
 * @retval APP_UTILITY_SUCCESS          Successfully appended to service data.
 * @retval APP_UTILITY_INVALID_LEN      Append not allowed due to invalid length.
 * @retval APP_UTILITY_FAIL             Append failure due to stack API return failed.
*/
uint8_t APP_UTILITY_UpdateBleScanRspServiceData(uint8_t* p_svcData,uint8_t svcDataLen);

/**@brief append data on ble user data of scan response packet.
 *
 * @param[in] p_userData                Pointer to the uint8_t buffer.
 * @param[in] userDataLen               Length of the buffer.
 *
 * @retval APP_UTILITY_SUCCESS          Successfully appended to user data.
 * @retval APP_UTILITY_INVALID_LEN      Append not allowed due to invalid length.
 * @retval APP_UTILITY_FAIL             Append failure due to stack API return failure.
*/
uint8_t APP_UTILITY_UpdateBleScanRspUserData(uint8_t* p_userData,uint8_t userDataLen);


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _APP_UTILITY_H */