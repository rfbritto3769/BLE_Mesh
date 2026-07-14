// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2024 Microchip Technology Inc. and its subsidiaries.
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

/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_ble_callbacks.h

  Summary:
    This file contains API functions for the user to implement his business logic.

  Description:
    API functions for the user to implement his business logic.
*******************************************************************************/

#ifndef APP_BLE_CALLBACKS_H
#define	APP_BLE_CALLBACKS_H

#ifdef	__cplusplus
extern "C" {
#endif

/*******************Header section common*****************/
#include "ble_gap.h"
#include "definitions.h"
#include "ble_dm/ble_dm.h" 
/*****************************************************************************/   
    
/************************Macros defined here**************/

/**************************************************************************************/     
/****generated sample application code****/
#define APP_BLE_CREATE_CONN_SCAN_INTERVAL           0x3C
#define APP_BLE_CREATE_CONN_SCAN_WINDOW             0x1E
#define APP_BLE_CREATE_CONN_INTERVAL_MIN            0x10   
#define APP_BLE_CREATE_CONN_INTERVAL_MAX            0x10    
#define APP_BLE_CREATE_CONN_LATENCY                 0
#define APP_BLE_CREATE_CONN_SUPERVISION_TIOMEOUT    0x48
// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
/****generated sample application code****/
extern uint16_t conn_hdl[CONFIG_APP_BLE_MAXIMUM_NUMBER_OF_LINKS];
extern uint8_t no_of_links;
// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// ***************************************************************************** 
void APP_WLS_BLE_DeviceConnected(BLE_GAP_EvtConnect_T  *p_evtConnect);
void APP_WLS_BLE_DeviceDisconnected(BLE_GAP_EvtDisconnect_T *p_evtDisconnect);
void APP_WLS_BLE_AdvertisementReportReceived(BLE_GAP_EvtAdvReport_T *p_evtAdvReport);
void APP_WLS_BLE_ExtendedAdvertisementReportReceived(BLE_GAP_EvtExtAdvReport_T *p_evtExtAdvReport);
void APP_WLS_BLE_ScanTimedOut();
void APP_WLS_BLE_AdvertisementCompleted();
void APP_WLS_BLE_AdvertisementTimedOut();
void APP_WLS_BLE_PathLossThresholdReceived(BLE_GAP_EvtPathLossThreshold_T *p_evtPathLossThreshold);
void APP_WLS_BLE_PairedDeviceDisconnected(BLE_DM_Event_T *p_event);
void APP_WLS_BLE_PairedDeviceConnected(BLE_DM_Event_T *p_event);
void APP_BLE_RescanHandler(void);
void APP_BLE_ConnectNextPeer(void);

#ifdef	__cplusplus
}
#endif
#endif	/* APP_API_EVENTS_H */
