/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
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

#ifndef BLE_TRSPS_H
#define BLE_TRSPS_H

#include <stdint.h>
#include "stack_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_TRSPS_MAX_CONN_NBR                  BLE_GAP_MAX_LINK_NBR

#define BLE_TRSPS_STATUS_CTRL_DISABLED          (0x00U)
#define BLE_TRSPS_STATUS_CTRL_OPENED            (0x01U)
#define BLE_TRSPS_STATUS_TX_DISABLED            (0x00U)
#define BLE_TRSPS_STATUS_TX_OPENED              (0x01U)

typedef enum BLE_TRSPS_EventId_T
{
    BLE_TRSPS_EVT_NULL = 0x00U,
    BLE_TRSPS_EVT_CTRL_STATUS,
    BLE_TRSPS_EVT_TX_STATUS,
    BLE_TRSPS_EVT_CBFC_ENABLED,
    BLE_TRSPS_EVT_CBFC_CREDIT,
    BLE_TRSPS_EVT_RECEIVE_DATA,
    BLE_TRSPS_EVT_VENDOR_CMD,
    BLE_TRSPS_EVT_ERR_UNSPECIFIED,
    BLE_TRSPS_EVT_ERR_NO_MEM,
    BLE_TRSPS_EVT_END
} BLE_TRSPS_EventId_T;

typedef struct BLE_TRSPS_EvtCtrlStatus_T
{
    uint16_t         connHandle;
    uint8_t          status;
} BLE_TRSPS_EvtCtrlStatus_T;

typedef struct BLE_TRSPS_EvtTxStatus_T
{
    uint16_t         connHandle;
    uint8_t          status;
} BLE_TRSPS_EvtTxStatus_T;

typedef struct BLE_TRSPS_EvtCbfcEnabled_T
{
    uint16_t         connHandle;
} BLE_TRSPS_EvtCbfcEnabled_T;

typedef struct BLE_TRSPS_EvtReceiveData_T
{
    uint16_t         connHandle;
} BLE_TRSPS_EvtReceiveData_T;

typedef struct BLE_TRSPS_EvtVendorCmd_T
{
    uint16_t         connHandle;
    uint16_t         length;
    uint8_t          *p_payLoad;
} BLE_TRSPS_EvtVendorCmd_T;

typedef union
{
    BLE_TRSPS_EvtCtrlStatus_T        onCtrlStatus;
    BLE_TRSPS_EvtTxStatus_T          onTxStatus;
    BLE_TRSPS_EvtCbfcEnabled_T       onCbfcEnabled;
    BLE_TRSPS_EvtReceiveData_T       onReceiveData;
    BLE_TRSPS_EvtVendorCmd_T         onVendorCmd;
} BLE_TRSPS_EventField_T;

typedef struct BLE_TRSPS_Event_T
{
    BLE_TRSPS_EventId_T       eventId;
    BLE_TRSPS_EventField_T    eventField;
} BLE_TRSPS_Event_T;

typedef void(*BLE_TRSPS_EventCb_T)(BLE_TRSPS_Event_T *p_event);

void BLE_TRSPS_EventRegister(BLE_TRSPS_EventCb_T bleTranServHandler);
uint16_t BLE_TRSPS_Init(void);
uint16_t BLE_TRSPS_SendVendorCommand(uint16_t connHandle, uint8_t commandID, uint8_t commandLength, uint8_t *p_commandPayload);
uint16_t BLE_TRSPS_SendData(uint16_t connHandle, uint16_t len, uint8_t *p_data);
void BLE_TRSPS_GetDataLength(uint16_t connHandle, uint16_t *p_dataLength);
uint16_t BLE_TRSPS_GetData(uint16_t connHandle, uint8_t *p_data);
void BLE_TRSPS_BleEventHandler(STACK_Event_T *p_stackEvent);

#ifdef __cplusplus
}
#endif

#endif
