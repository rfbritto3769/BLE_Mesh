/*******************************************************************************
  Application BLE Profile Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_trspc_handler.c

  Summary:
    This file contains the Application BLE functions for this project.

  Description:
    This file contains the Application BLE functions for this project.
 *******************************************************************************/

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

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************






 
#include <string.h>
#include <stdint.h>
#include "ble_trspc/ble_trspc.h"
#include "definitions.h"
#include "gatt.h"
#include "app_trspc_handler.h"
#include "mesh_routing.h"
#include "app_ble_callbacks.h"
#include "mesh_conn_mgr.h"


// *****************************************************************************
// *****************************************************************************
// Section: Global Variables
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************




void APP_TrspcEvtHandler(BLE_TRSPC_Event_T *p_event)
{

switch(p_event->eventId)
{
    case BLE_TRSPC_EVT_UL_STATUS:
    {
        /* TODO: implement your application code.*/
    }
    break;

    case BLE_TRSPC_EVT_DL_STATUS:
    {
        /* NOT a one-shot "session opened". The server reuses opcode 0x14 for
           both the CBFC enable response and every credit replenishment, so
           this fires continuously while data flows. Sending the JOIN on each
           one creates a feedback loop - the JOIN burns a credit, the refill
           raises this event again - and makes the peer re-run its admission
           check, which tears down an established link with JOIN_REDIRECT once
           it has filled up. Only act on the first one, while the link is
           still unjoined; MESH_Maintenance retries from there. */
        uint16_t hdl = p_event->eventField.onDownlinkStatus.connHandle;
        MeshConn_T *conn = CONN_MGR_GetByHandle(hdl);
        if (conn != NULL && !conn->topologyReady)
        {
            MESH_SendHello(hdl);
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "TRSPC DL open hdl=0x%04X\r\n", hdl);
        }
    }
    break;

    case BLE_TRSPC_EVT_RECEIVE_DATA:
    {
            uint16_t dataLen;
            uint16_t hdl = p_event->eventField.onReceiveData.connHandle;
            /* Drain every queued packet. BLE_TRSPC_GetData is the only place
               that returns an uplink CBFC credit to the peer, so a packet left
               in the queue costs one credit permanently and eventually mutes
               the peer's notifications towards us. Static buffer: a malloc
               failure here used to skip the dequeue entirely. */
            static uint8_t rxBuf[BLE_ATT_MAX_MTU_LEN - ATT_HANDLE_VALUE_HEADER_SIZE];

            for (;;)
            {
                BLE_TRSPC_GetDataLength(hdl, &dataLen);
                /* GetDataLength reports 0 both for an empty queue and for a
                   zero-length packet, so always attempt the dequeue: a failing
                   GetData is the only reliable "queue is empty". */
                if (BLE_TRSPC_GetData(hdl, rxBuf) != 0U)
                    break;
                if (dataLen == 0U || dataLen > sizeof(rxBuf))
                    continue; /* dequeued, so the credit is returned; unusable */
                MESH_ProcessIncoming(hdl, dataLen, rxBuf);
            }
    }
    break;

    case BLE_TRSPC_EVT_VENDOR_CMD:
    {
        /* TODO: implement your application code.*/
    }            
    break;

    case BLE_TRSPC_EVT_VENDOR_CMD_RSP:
    {
        /* TODO: implement your application code.*/
    }            
    break;

    case BLE_TRSPC_EVT_DISC_COMPLETE:
    {
        /* Discovery is done but the transport is not open yet: the stack only
           starts the CBFC downlink handshake after emitting this event. The
           link is marked ready by MESH_SendHello once a write succeeds. */
        MESH_SendHello(p_event->eventField.onDiscComplete.connHandle);
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "TRSPC disc complete hdl=0x%04X\r\n",
            p_event->eventField.onDiscComplete.connHandle);
        APP_BLE_ConnectNextPeer();
    }
    break;

    case BLE_TRSPC_EVT_ERR_NO_MEM:
    {
        /* TODO: implement your application code.*/
    }
    break;

    default:
    break;
}


}

