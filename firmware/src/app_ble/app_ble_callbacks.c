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
    app_ble_callbacks.c

  Summary:
    This file contains API functions for the user to implement his business logic.

  Description:
    API functions for the user to implement his business logic.
*******************************************************************************/
// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "app_ble_callbacks.h"
#include "definitions.h"
#include "app_ble_handler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "mesh_conn_mgr.h"
#include "mesh_routing.h"
#include "node_config.h"
#include "provisioning.h"
#include "ble_gap.h"

#define MESH_RESCAN_DURATION       10000
#define MESH_MAX_DISCOVERED        6

typedef struct {
    BLE_GAP_Addr_T addr;
    uint8_t nodeId;
} DiscoveredPeer_T;

static uint8_t s_pendingPeerNodeId = 0;
static bool s_initialScanDone = false;
static DiscoveredPeer_T s_discovered[MESH_MAX_DISCOVERED];
static uint8_t s_discoveredCount = 0;
static uint8_t s_connectIndex = 0;

static void mesh_ConnectNext(void);


void APP_BLE_ConnectNextPeer(void)
{
    if (!s_initialScanDone)
    {
        s_initialScanDone = true;
        BLE_GAP_SetScanningEnable(false, 0, 0, 0);
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Discovery ended found=%d\r\n", (int)s_discoveredCount);
    }

    if (s_pendingPeerNodeId != 0)
        return;

    while (s_connectIndex < s_discoveredCount)
    {
        DiscoveredPeer_T *peer = &s_discovered[s_connectIndex];
        s_connectIndex++;

        if (CONN_MGR_GetCentralCount() >= MESH_MAX_CENTRAL)
            return;
        if (CONN_MGR_IsConnectedToPeer(peer->nodeId))
            continue;

        BLE_GAP_CreateConnParams_T params;
        params.scanInterval = APP_BLE_CREATE_CONN_SCAN_INTERVAL;
        params.scanWindow = APP_BLE_CREATE_CONN_SCAN_WINDOW;
        params.filterPolicy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
        params.peerAddr = peer->addr;
        params.connParams.intervalMin = 0x50;
        params.connParams.intervalMax = 0xA0;
        params.connParams.latency = 0;
        params.connParams.supervisionTimeout = 0x0190;

        s_pendingPeerNodeId = peer->nodeId;
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connecting to DIMMER_%02d\r\n", peer->nodeId);
        uint16_t status = BLE_GAP_CreateConnection(&params);
        if (status != 0)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "CreateConn FAIL 0x%04X\r\n", status);
            s_pendingPeerNodeId = 0;
            continue;
        }
        return;
    }
}

void APP_BLE_RescanHandler(void)
{
    BLE_GAP_SetAdvEnable(0x01, 0x00);
}
// *****************************************************************************
// *****************************************************************************
// Section: Macros
// *****************************************************************************
// *****************************************************************************
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
/****generated sample application code****/
static void APP_WLS_HexToAscii(uint8_t byteNum, uint8_t *p_hex, uint8_t *p_ascii)
{
    uint8_t i, j, c;
    uint8_t digitNum = byteNum * 2;

    if (p_hex == NULL || p_ascii == NULL)
        return;

    for (i = 0; i < digitNum; i++)
    {
        j = i / 2;
        c = p_hex[j] & 0x0F;

        if (c >= 0x00 && c <= 0x09)
        {
            p_ascii[digitNum - i - 1] = c + 0x30;
        }
        else if (c >= 0x0A && c <= 0x0F)
        {
            p_ascii[digitNum - i - 1] = c - 0x0A + 'A';
        }

        p_hex[j] /= 16;
    }
}

/*******************************************************************************
  Function:
    void APP_WLS_BLE_DeviceConnected(BLE_GAP_EvtConnect_T  *p_evtConnect)

  Summary:
     Function for handling GAP connected indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_DeviceConnected(BLE_GAP_EvtConnect_T  *p_evtConnect)
{
    ConnRole_T role;
    ConnType_T type = CONN_TYPE_UNKNOWN;

    if (p_evtConnect->role == 0)
        role = CONN_ROLE_CENTRAL;
    else
        role = CONN_ROLE_PERIPHERAL;

    if (!CONN_MGR_AddConnection(p_evtConnect->connHandle, role))
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "AddConn FULL hdl=0x%04X\r\n", p_evtConnect->connHandle);
        BLE_GAP_Disconnect(p_evtConnect->connHandle, 0x13);
        return;
    }

    if (!s_initialScanDone)
    {
        s_initialScanDone = true;
        BLE_GAP_SetScanningEnable(false, 0, 0, 0);
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Discovery ended found=%d\r\n", (int)s_discoveredCount);
    }

    if (role == CONN_ROLE_CENTRAL)
    {
        type = CONN_TYPE_LOCAL;
        if (s_pendingPeerNodeId != 0)
        {
            CONN_MGR_SetPeerNodeId(p_evtConnect->connHandle, s_pendingPeerNodeId);
            s_pendingPeerNodeId = 0;
        }
    }

    CONN_MGR_SetType(p_evtConnect->connHandle, type);

    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connected hdl=0x%04X role=%s C=%d P=%d\r\n",
        p_evtConnect->connHandle, (role == CONN_ROLE_CENTRAL) ? "C" : "P",
        CONN_MGR_GetCentralCount(), CONN_MGR_GetPeripheralCount());

    BLE_GAP_SetAdvEnable(0x01, 0x00);
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_DeviceDisconnected(BLE_GAP_EvtDisconnect_T *p_evtDisconnect)

  Summary:
     Function for handling GAP disconnected indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_DeviceDisconnected(BLE_GAP_EvtDisconnect_T *p_evtDisconnect)
{
    CONN_MGR_RemoveConnection(p_evtDisconnect->connHandle);
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Disconnected hdl=0x%04X reason=0x%02X\r\n",
        p_evtDisconnect->connHandle, p_evtDisconnect->reason);

    BLE_GAP_SetAdvEnable(0x01, 0x00);
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_AdvertisementReportReceived(BLE_GAP_EvtAdvReport_T *p_evtAdvReport)

  Summary:
     Function for handling GAP advertisement report received indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
static bool mesh_IsPeerDiscovered(uint8_t nodeId)
{
    uint8_t i;
    for (i = 0; i < s_discoveredCount; i++)
    {
        if (s_discovered[i].nodeId == nodeId)
            return true;
    }
    return false;
}

void APP_WLS_BLE_AdvertisementReportReceived(BLE_GAP_EvtAdvReport_T *p_evtAdvReport)
{
    uint8_t *p_advData = p_evtAdvReport->advData;
    uint8_t advLen = p_evtAdvReport->length;
    uint8_t offset = 0;
    uint8_t myId = MESH_GetNodeId();

    while (offset < advLen)
    {
        uint8_t fieldLen = p_advData[offset];
        if (fieldLen == 0) break;
        if ((offset + fieldLen) >= advLen) break;

        uint8_t fieldType = p_advData[offset + 1];

        if ((fieldType == 0x09 || fieldType == 0x08) && fieldLen >= 10)
        {
            if (memcmp(&p_advData[offset + 2], "DIMMER_", 7) == 0)
            {
                uint8_t d1 = p_advData[offset + 9] - '0';
                uint8_t d2 = p_advData[offset + 10] - '0';
                uint8_t peerNodeId = d1 * 10 + d2;

                if (peerNodeId == myId) return;
                if (myId <= peerNodeId) return;
                if (mesh_IsPeerDiscovered(peerNodeId)) return;
                if (s_discoveredCount >= MESH_MAX_DISCOVERED) return;

                s_discovered[s_discoveredCount].addr.addrType = p_evtAdvReport->addr.addrType;
                memcpy(s_discovered[s_discoveredCount].addr.addr, p_evtAdvReport->addr.addr, GAP_MAX_BD_ADDRESS_LEN);
                s_discovered[s_discoveredCount].nodeId = peerNodeId;
                s_discoveredCount++;

                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Found DIMMER_%02d (%d total)\r\n",
                    peerNodeId, (int)s_discoveredCount);
                return;
            }
        }
        offset += fieldLen + 1;
    }
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_ExtendedAdvertisementReportReceived(BLE_GAP_EvtExtAdvReport_T *p_evtExtAdvReport)

  Summary:
     Function for handling GAP advertisement report received indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_ExtendedAdvertisementReportReceived(BLE_GAP_EvtExtAdvReport_T *p_evtExtAdvReport)
{
/* TODO: implement your application code.*/
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_ScanTimedOut()

  Summary:
     Function for handling GAP scan timeout indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_ScanTimedOut()
{
    s_initialScanDone = true;

    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Scan done C=%d P=%d found=%d\r\n",
        CONN_MGR_GetCentralCount(), CONN_MGR_GetPeripheralCount(),
        (int)s_discoveredCount);

    CONN_MGR_SweepStale(pdMS_TO_TICKS(15000));

    BLE_GAP_SetAdvEnable(0x01, 0x00);

    s_connectIndex = 0;
    APP_BLE_ConnectNextPeer();
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_AdvertisementCompleted()

  Summary:
     Function for handling GAP advertisement complete indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_AdvertisementCompleted()
{
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_AdvertisementTimedOut()

  Summary:
     Function for handling GAP advertisement timeout indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_AdvertisementTimedOut()
{
/* TODO: implement your application code.*/ 
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_PathLossThresholdReceived(BLE_GAP_EvtPathLossThreshold_T *p_evtPathLossThreshold)

  Summary:
     Function for handling pathLoss threshold received indication EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_PathLossThresholdReceived(BLE_GAP_EvtPathLossThreshold_T *p_evtPathLossThreshold)
{
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_PairedDeviceDisconnected()

  Summary:
     Function for handling paired device link terminated EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_PairedDeviceDisconnected(BLE_DM_Event_T *p_event)
{
}
/*******************************************************************************
  Function:
    void APP_WLS_BLE_PairedDeviceConnected(BLE_DM_Event_T *p_event)

  Summary:
     Function for handling paired device link connected EVENT message.

  Description:

  Precondition:

  Parameters:                
              

  Returns:
    None.
*/
void APP_WLS_BLE_PairedDeviceConnected(BLE_DM_Event_T *p_event)
{
}
