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

/* BLE_GAP_SetScanningEnable() takes the duration in units of 100 ms, not in
   milliseconds. Passing a millisecond value keeps the radio scanning for tens
   of minutes, so BLE_GAP_EVT_SCAN_TIMEOUT never arrives and the node never
   leaves the discovery window to connect to anyone. */
#define MESH_SCAN_DURATION_MS      6000UL
#define MESH_SCAN_DURATION_UNITS   ((uint16_t)(MESH_SCAN_DURATION_MS / 100UL))
#define MESH_SCAN_GUARD_TICKS      pdMS_TO_TICKS(MESH_SCAN_DURATION_MS + 3000UL)
#define MESH_CONNECT_GUARD_TICKS   pdMS_TO_TICKS(10000)
#define MESH_MAX_DISCOVERED        16
#define MESH_RESCAN_PERIOD_TICKS   pdMS_TO_TICKS(30000)
#define MESH_PEER_MAX_AGE_TICKS    pdMS_TO_TICKS(120000)

/* A link that dies this fast was refused (JOIN_REDIRECT, connection table
   full), not lost. Retrying it immediately produces a permanent
   connect/disconnect loop, so such a peer is put on an escalating backoff. */
#define MESH_LINK_STABLE_TICKS     pdMS_TO_TICKS(10000)
#define MESH_PEER_BACKOFF_STEP_MS  15000UL
#define MESH_PEER_BACKOFF_MAX_STEP 5U

/* Having no mesh link at all for this long means the discovery state itself
   has gone stale: a peer aged out, a backoff outlived the condition that set
   it, or a freeSlots=0 from an old redirect never got refreshed. Rebuild it
   from scratch instead of requiring a manual reset of the node. */
#define MESH_ISOLATION_HEAL_TICKS  pdMS_TO_TICKS(60000)

typedef struct {
    BLE_GAP_Addr_T addr;
    uint8_t nodeId;
    int8_t rssi;
    uint32_t lastSeenTick;
    uint8_t failures;
    uint8_t rootId;
    uint8_t depth;
    uint8_t freeSlots;
    uint8_t flags;
    uint32_t retryAfterTick;
} DiscoveredPeer_T;

static uint8_t s_pendingPeerNodeId = 0;
static bool s_initialScanDone = false;
static DiscoveredPeer_T s_discovered[MESH_MAX_DISCOVERED];
static uint8_t s_discoveredCount = 0;
static uint8_t s_connectIndex = 0;
static bool s_scanActive = true;
static uint32_t s_lastScanTick = 0;
static uint32_t s_scanStartTick = 0;
static uint32_t s_pendingSinceTick = 0;
static uint32_t s_lastLinkedTick = 0;
static bool s_repairRequested = false;

static bool mesh_SameAddress(const BLE_GAP_Addr_T *a, const BLE_GAP_Addr_T *b)
{
    return a->addrType == b->addrType &&
           memcmp(a->addr, b->addr, GAP_MAX_BD_ADDRESS_LEN) == 0;
}

bool APP_BLE_IsNodeIdObserved(uint8_t nodeId)
{
    uint8_t i;
    uint32_t now = xTaskGetTickCount();
    for (i = 0; i < s_discoveredCount; i++)
        if (s_discovered[i].nodeId == nodeId &&
            (now - s_discovered[i].lastSeenTick) < MESH_PEER_MAX_AGE_TICKS)
            return true;
    return false;
}

/* Escalating hold-off so a peer that refuses us is not hammered. The delay is
   derived from the failure count, which only clears once a link to that peer
   has actually stayed up (see APP_BLE_MarkPeerDisconnected). */
static void mesh_BackoffPeer(uint8_t nodeId)
{
    uint8_t i;
    uint32_t now = xTaskGetTickCount();
    for (i = 0U; i < s_discoveredCount; i++)
    {
        if (s_discovered[i].nodeId != nodeId) continue;
        if (s_discovered[i].failures < 0xFFU) s_discovered[i].failures++;
        s_discovered[i].retryAfterTick = now + pdMS_TO_TICKS(
            MESH_PEER_BACKOFF_STEP_MS *
            ((s_discovered[i].failures < MESH_PEER_BACKOFF_MAX_STEP) ?
                s_discovered[i].failures : MESH_PEER_BACKOFF_MAX_STEP));
    }
}

void APP_BLE_MarkPeerRejected(uint8_t nodeId)
{
    uint8_t i;
    for (i = 0U; i < s_discoveredCount; i++)
        if (s_discovered[i].nodeId == nodeId)
            s_discovered[i].freeSlots = 0U;
    mesh_BackoffPeer(nodeId);
}

void APP_BLE_MarkPeerUnstable(uint8_t nodeId)
{
    mesh_BackoffPeer(nodeId);
}

bool APP_BLE_StartScan(void)
{
    if (BLE_GAP_SetScanningEnable(true, BLE_GAP_SCAN_FD_ENABLE,
        BLE_GAP_SCAN_MODE_OBSERVER, MESH_SCAN_DURATION_UNITS) != 0U)
        return false;
    s_scanActive = true;
    s_scanStartTick = xTaskGetTickCount();
    return true;
}

/* Called only for links that were up long enough to be considered healthy.
   Losing one of those releases a slot on both ends, so the peer becomes an
   immediate candidate again and its trouble history is cleared. */
void APP_BLE_MarkPeerDisconnected(uint8_t nodeId)
{
    uint8_t i;
    for (i = 0U; i < s_discoveredCount; i++)
    {
        if (s_discovered[i].nodeId == nodeId)
        {
            if (s_discovered[i].freeSlots == 0U)
                s_discovered[i].freeSlots = 1U;
            s_discovered[i].failures = 0U;
            s_discovered[i].retryAfterTick = 0U;
        }
    }
}

static void mesh_SortCandidates(void)
{
    uint8_t i, j;
    for (i = 0; i + 1U < s_discoveredCount; i++)
        for (j = i + 1U; j < s_discoveredCount; j++)
            if ((s_discovered[j].freeSlots > 0U && s_discovered[i].freeSlots == 0U) ||
                (s_discovered[j].freeSlots == s_discovered[i].freeSlots &&
                 s_discovered[j].rootId < s_discovered[i].rootId) ||
                (s_discovered[j].freeSlots == s_discovered[i].freeSlots &&
                 s_discovered[j].rootId == s_discovered[i].rootId &&
                 s_discovered[j].depth < s_discovered[i].depth) ||
                (s_discovered[j].rootId == s_discovered[i].rootId &&
                 s_discovered[j].depth == s_discovered[i].depth &&
                 s_discovered[j].failures < s_discovered[i].failures) ||
                (s_discovered[j].rootId == s_discovered[i].rootId &&
                 s_discovered[j].depth == s_discovered[i].depth &&
                 s_discovered[j].failures == s_discovered[i].failures &&
                 s_discovered[j].rssi > s_discovered[i].rssi))
            {
                DiscoveredPeer_T tmp = s_discovered[i];
                s_discovered[i] = s_discovered[j];
                s_discovered[j] = tmp;
            }
}



void APP_BLE_ConnectNextPeer(void)
{
    /* Candidate selection starts only after the complete scan window. */
    if (!s_initialScanDone || s_scanActive)
        return;

    if (s_pendingPeerNodeId != 0)
        return;

    while (s_connectIndex < s_discoveredCount)
    {
        DiscoveredPeer_T *peer = &s_discovered[s_connectIndex];
        s_connectIndex++;

        /* Each candidate is evaluated once per scan pass, so logging the
           rejection reason here is self-rate-limiting. Without it a node that
           stops reconnecting gives no clue which gate is holding it back. */
        if (CONN_MGR_GetCentralCount() >= MESH_MAX_CENTRAL)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Skip DIMMER_%02d: central full\r\n",
                peer->nodeId);
            return;
        }
        if ((xTaskGetTickCount() - peer->lastSeenTick) >= MESH_PEER_MAX_AGE_TICKS)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Skip DIMMER_%02d: not seen\r\n",
                peer->nodeId);
            continue;
        }
        if (peer->nodeId >= MESH_GetNodeId())
            continue; /* deterministic direction prevents reciprocal links */
        if (peer->freeSlots == 0U)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Skip DIMMER_%02d: no free slot\r\n",
                peer->nodeId);
            continue;
        }
        if (peer->retryAfterTick != 0U &&
            (int32_t)(xTaskGetTickCount() - peer->retryAfterTick) < 0)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Skip DIMMER_%02d: backoff f=%d\r\n",
                peer->nodeId, (int)peer->failures);
            continue; /* refused us recently, still on hold-off */
        }
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
        /* 8 s. A node runs up to six links while it also advertises and
           periodically scans, so a 4 s supervision timeout (20 connection
           events at 200 ms) drops links purely from radio contention. */
        params.connParams.supervisionTimeout = 0x0320;

        s_pendingPeerNodeId = peer->nodeId;
        s_pendingSinceTick = xTaskGetTickCount();
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connecting to DIMMER_%02d\r\n", peer->nodeId);
        uint16_t status = BLE_GAP_CreateConnection(&params);
        if (status != 0)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "CreateConn FAIL 0x%04X\r\n", status);
            s_pendingPeerNodeId = 0;
            mesh_BackoffPeer(peer->nodeId);
            continue;
        }
        return;
    }
}

void APP_BLE_RescanHandler(void)
{
    static uint32_t lastAdvKickTick = 0;
    uint32_t now = xTaskGetTickCount();

    /* Connect, disconnect, adv-complete and adv-timeout all re-enable
       advertising already. This is only a safety net, so poll it slowly
       instead of issuing an HCI command every maintenance tick. */
    if ((now - lastAdvKickTick) >= pdMS_TO_TICKS(10000))
    {
        lastAdvKickTick = now;
        (void)BLE_GAP_SetAdvEnable(0x01, 0x00);
    }

    /* BLE_GAP_EVT_SCAN_TIMEOUT is the only event that releases the discovery
       gate. If it is lost or delayed the node stays a passive peripheral for
       ever, so close the window locally once the guard time has passed. */
    if (s_scanActive && (now - s_scanStartTick) >= MESH_SCAN_GUARD_TICKS)
    {
        (void)BLE_GAP_SetScanningEnable(false, 0, 0, 0);
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Scan window closed locally\r\n");
        APP_WLS_BLE_ScanTimedOut();
        now = xTaskGetTickCount();
    }

    /* BLE_GAP_CreateConnection never times out on its own. A peer that
       disappears mid-attempt would otherwise block every further connection
       attempt and every rescan, because both require no pending peer. */
    if (s_pendingPeerNodeId != 0U &&
        (now - s_pendingSinceTick) >= MESH_CONNECT_GUARD_TICKS)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connect timeout DIMMER_%02d\r\n",
            s_pendingPeerNodeId);
        (void)BLE_GAP_CreateConnectionCancel();
        mesh_BackoffPeer(s_pendingPeerNodeId);
        s_pendingPeerNodeId = 0U;
    }

    if (CONN_MGR_GetLocalLinkCount() > 0U)
    {
        s_lastLinkedTick = now;
    }
    else if ((now - s_lastLinkedTick) >= MESH_ISOLATION_HEAL_TICKS)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Isolated %ds, resetting discovery\r\n",
            (int)(MESH_ISOLATION_HEAL_TICKS / configTICK_RATE_HZ));
        memset(s_discovered, 0, sizeof(s_discovered));
        s_discoveredCount = 0U;
        s_connectIndex = 0U;
        s_pendingPeerNodeId = 0U;
        s_lastLinkedTick = now;
        s_repairRequested = true;
        s_lastScanTick = 0U;
    }

    if (!s_scanActive && s_pendingPeerNodeId == 0U &&
        (s_repairRequested || (now - s_lastScanTick) >= MESH_RESCAN_PERIOD_TICKS))
    {
        s_connectIndex = 0U;
        mesh_SortCandidates();
        if (APP_BLE_StartScan())
        {
            s_repairRequested = false;
            s_lastScanTick = now;
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Periodic topology scan\r\n");
        }
    }
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

    /* BLE_GAP_EVT_CONNECTED is also emitted when an outgoing connection
       attempt fails.  Never add an invalid handle to the mesh table. */
    if (p_evtConnect->status != 0U)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connect failed status=0x%02X\r\n",
            p_evtConnect->status);
        s_pendingPeerNodeId = 0U;
        (void)BLE_GAP_SetAdvEnable(true, 0U);
        return;
    }

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
        s_scanActive = false;
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
    MESH_TopologyChanged();

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
    MeshConn_T *lost = CONN_MGR_GetByHandle(p_evtDisconnect->connHandle);
    uint8_t lostNodeId = (lost != NULL) ? lost->peerNodeId : 0U;
    uint32_t lifetime = (lost != NULL) ?
        (xTaskGetTickCount() - lost->createdTick) : 0U;
    s_pendingPeerNodeId = 0U;
    if (lostNodeId != 0U)
    {
        if (lifetime >= MESH_LINK_STABLE_TICKS)
            APP_BLE_MarkPeerDisconnected(lostNodeId);
        else
            APP_BLE_MarkPeerUnstable(lostNodeId);
    }
    MESH_OnLinkLost(lostNodeId);
    CONN_MGR_RemoveConnection(p_evtDisconnect->connHandle);
    MESH_TopologyChanged();
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Disconnected hdl=0x%04X reason=0x%02X\r\n",
        p_evtDisconnect->connHandle, p_evtDisconnect->reason);

    BLE_GAP_SetAdvEnable(0x01, 0x00);
    s_repairRequested = true;
    s_lastScanTick = 0U;
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
static int16_t mesh_FindPeerByAddress(const BLE_GAP_Addr_T *addr)
{
    uint8_t i;
    for (i = 0; i < s_discoveredCount; i++)
        if (mesh_SameAddress(&s_discovered[i].addr, addr)) return (int16_t)i;
    return -1;
}

void APP_WLS_BLE_AdvertisementReportReceived(BLE_GAP_EvtAdvReport_T *p_evtAdvReport)
{
    uint8_t *p_advData = p_evtAdvReport->advData;
    uint8_t advLen = p_evtAdvReport->length;
    uint8_t offset = 0;
    uint8_t myId = MESH_GetNodeId();
    int16_t reportPeerIndex = mesh_FindPeerByAddress(&p_evtAdvReport->addr);

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

                int16_t peerIndex;
                uint32_t now = xTaskGetTickCount();
                if (peerNodeId == myId) {
                    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "DUPLICATE NODE ID %u detected\r\n", peerNodeId);
                    return;
                }
                peerIndex = mesh_FindPeerByAddress(&p_evtAdvReport->addr);
                if (peerIndex >= 0)
                {
                    s_discovered[peerIndex].nodeId = peerNodeId;
                    s_discovered[peerIndex].rssi = p_evtAdvReport->rssi;
                    s_discovered[peerIndex].lastSeenTick = now;
                    reportPeerIndex = peerIndex;
                }
                else if (s_discoveredCount >= MESH_MAX_DISCOVERED)
                {
                    uint8_t weakest = 0U, i;
                    for (i = 1U; i < s_discoveredCount; i++)
                        if (s_discovered[i].rssi < s_discovered[weakest].rssi) weakest = i;
                    if (p_evtAdvReport->rssi <= s_discovered[weakest].rssi) return;
                    peerIndex = weakest;
                }
                else peerIndex = s_discoveredCount++;

                if (reportPeerIndex < 0)
                {
                    s_discovered[peerIndex].addr = p_evtAdvReport->addr;
                    s_discovered[peerIndex].nodeId = peerNodeId;
                    s_discovered[peerIndex].rssi = p_evtAdvReport->rssi;
                    s_discovered[peerIndex].lastSeenTick = now;
                    s_discovered[peerIndex].failures = 0U;
                    s_discovered[peerIndex].rootId = peerNodeId;
                    s_discovered[peerIndex].depth = 0U;
                    s_discovered[peerIndex].freeSlots = 1U;
                    s_discovered[peerIndex].flags = 0U;
                    s_discovered[peerIndex].retryAfterTick = 0U;
                    reportPeerIndex = peerIndex;

                    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Found DIMMER_%02d (%d total)\r\n",
                        peerNodeId, (int)s_discoveredCount);
                }
            }
        }
        else if (fieldType == 0x16U && fieldLen >= 9U &&
                 p_advData[offset + 2] == 0xDAU && p_advData[offset + 3] == 0xFEU &&
                 p_advData[offset + 5] >= 0x02U)
        {
            if (reportPeerIndex < 0)
                reportPeerIndex = mesh_FindPeerByAddress(&p_evtAdvReport->addr);
            if (reportPeerIndex >= 0)
            {
                s_discovered[reportPeerIndex].flags = p_advData[offset + 6];
                s_discovered[reportPeerIndex].freeSlots = p_advData[offset + 7];
                s_discovered[reportPeerIndex].rootId = p_advData[offset + 8];
                s_discovered[reportPeerIndex].depth = p_advData[offset + 9];
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
    s_scanActive = false;
    s_lastScanTick = xTaskGetTickCount();

    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Scan done C=%d P=%d found=%d\r\n",
        CONN_MGR_GetCentralCount(), CONN_MGR_GetPeripheralCount(),
        (int)s_discoveredCount);

    CONN_MGR_SweepStale(pdMS_TO_TICKS(15000));

    BLE_GAP_SetAdvEnable(0x01, 0x00);

    mesh_SortCandidates();
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
    /* Advertising may complete asynchronously. Keep the node discoverable. */
    (void)BLE_GAP_SetAdvEnable(true, 0U);
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
    (void)BLE_GAP_SetAdvEnable(true, 0U);
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
