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
#include "app_ble.h"
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
/* Must exceed the realistic time to acquire a peer. The initiator scans at 20%
   duty and peers advertise every 200-400 ms across three channels, so
   acquisition averages several seconds with a long tail. At 10 s this guard
   was cancelling attempts that were about to succeed, which both backed the
   peer off for no reason and raced the cancel against the connection actually
   landing. Kept under MESH_RESCAN_PERIOD_TICKS so a dead attempt cannot block
   rescanning for a whole cycle. */
#define MESH_CONNECT_GUARD_TICKS   pdMS_TO_TICKS(15000)
/* Deliberately far below NODE_ID_MAX. A node only ever holds five links, so
   the table exists to rank nearby candidates, not to mirror the network: when
   it fills, the weakest RSSI is evicted and the nearest peers survive, which
   is exactly the set worth connecting to. It cannot be smaller than the number
   of nodes in radio range though - at 16 a dense bench of 100 evicted
   continuously, and every eviction resets that peer's failure count and
   backoff, so the hold-off never accumulated and APP_BLE_IsNodeIdObserved
   started missing IDs that do exist. Raising it further is not free either:
   mesh_FindPeerByAddress runs per advertising report and mesh_SortCandidates
   is O(n^2) per scan. */
#define MESH_MAX_DISCOVERED        48
#define MESH_RESCAN_PERIOD_TICKS   pdMS_TO_TICKS(30000)
/* lastSeenTick only advances while scanning, and a node with healthy links
   never scans, so this is really "how long a candidate survives between repair
   scans". At 120 s the whole list expired during normal operation and every
   repair started from an empty table. */
#define MESH_PEER_MAX_AGE_TICKS    pdMS_TO_TICKS(300000)

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
/* Both heals throw the discovery table away and start over from a fresh scan.
   That is the right move for a node whose view has genuinely gone stale, and
   the wrong one for a node that is simply waiting its turn: bringing up 100
   nodes means long stretches with no link while peer backoffs (up to 75 s)
   run down, and wiping a good candidate list in the middle of that discards
   the information the node was about to act on and slows convergence instead
   of helping it. Both are now longer than the worst-case backoff ladder. */
#define MESH_ISOLATION_HEAL_TICKS  pdMS_TO_TICKS(180000)
#define MESH_MIN_HEALTHY_LINKS     1U
#define MESH_DEGRADED_HEAL_TICKS   pdMS_TO_TICKS(300000)

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
    bool reportedLost;
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
static bool s_connectCancelPending = false;
static uint32_t s_lastLinkedTick = 0;
static uint32_t s_degradedSinceTick = 0;
static bool s_repairRequested = false;
/* The current scan is looking for a better parent rather than for a first
   link, so candidates are filtered on depth gain. */
static bool s_rebalanceScan = false;
static uint32_t s_lastRebalanceTick = 0;
/* Deliberately long. A rebalance is an optimisation, never a repair, so it
   must never compete with discovery for radio: one scan window every few
   minutes unwinds a chain in a handful of steps and costs almost nothing. */
#define MESH_REBALANCE_PERIOD_TICKS pdMS_TO_TICKS(150000)
/* Last resort for a node that no lower id will take: see the comment in
   APP_BLE_ConnectNextPeer. */
static bool s_allowUpwardLink = false;

static int16_t mesh_FindPeerByAddress(const BLE_GAP_Addr_T *addr);

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

/* Single decision point for the connection parameters of an incoming link.
   Called when the transport opens and again every time the peer's identity
   becomes better known, because the classification is not reliable at connect
   time: a node whose mesh is already formed has stopped scanning, so a
   newcomer's address is absent from the discovery table and the link looks
   like the phone.
 *
 * The default is deliberately the mesh profile. Being wrong towards mesh only
 * costs radio efficiency - a longer supervision timeout never kills a link.
 * Being wrong towards the app puts a 5 s timeout and peripheral latency on a
 * mesh link, which is exactly what made late-joining nodes drop with reason
 * 0x08 and then back themselves off out of the network. So the app profile is
 * applied only on positive identification (CONN_TYPE_PHONE), never on a
 * guess. */
void APP_BLE_ApplyLinkConnParams(uint16_t connHandle)
{
    MeshConn_T *conn = CONN_MGR_GetByHandle(connHandle);
    BLE_GAP_ConnParams_T params;

    if (conn == NULL || conn->role != CONN_ROLE_PERIPHERAL)
        return;

    if (conn->type == CONN_TYPE_PHONE && !conn->meshPeer)
    {
        /* Apple's rules, so the update is actually applied:
           intervalMax * (latency + 1) * 3 = 750 ms < 5 s. */
        params.intervalMin = 0x18;          /* 30 ms */
        params.intervalMax = 0x28;          /* 50 ms */
        params.latency = 4;
        params.supervisionTimeout = 0x01F4; /* 5 s */
    }
    else
    {
        params.intervalMin = 0x20;          /* 40 ms */
        params.intervalMax = 0x40;          /* 80 ms */
        params.latency = 0;
        params.supervisionTimeout = 0x07D0; /* 20 s, see the central side */
    }
    (void)BLE_GAP_UpdateConnParam(connHandle, &params);
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
    uint32_t now = xTaskGetTickCount();
    for (i = 0U; i < s_discoveredCount; i++)
    {
        if (s_discovered[i].nodeId == nodeId)
        {
            if (s_discovered[i].freeSlots == 0U)
                s_discovered[i].freeSlots = 1U;
            s_discovered[i].failures = 0U;
            s_discovered[i].retryAfterTick = 0U;
            /* The peer was exchanging BLE packets until this disconnect, so
               an old advertising timestamp does not mean it disappeared.
               Connected controllers commonly suppress duplicate advertising
               reports. Keep its known address immediately eligible for the
               repair attempt. */
            s_discovered[i].lastSeenTick = now;
            s_discovered[i].reportedLost = false;
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

    if (s_pendingPeerNodeId != 0 || s_connectCancelPending)
        return;

    /* The WBZ controller is much more reliable when GATT discovery and the
       TRSPC/CBFC handshake of one central link finish before another create
       procedure starts.  This also prevents the 0x3E pattern seen in logs. */
    if (CONN_MGR_HasUnreadyMeshLink())
        return;

    for (;;)
    {
    while (s_connectIndex < s_discoveredCount)
    {
        DiscoveredPeer_T *peer = &s_discovered[s_connectIndex];
        s_connectIndex++;

        /* Each candidate is evaluated once per scan pass, so logging the
           rejection reason here is self-rate-limiting. Without it a node that
           stops reconnecting gives no clue which gate is holding it back. */
        /* Not logged: having every uplink in use is the healthy steady state,
           and this path is reached once per maintenance tick, so printing here
           floods the console and drowns the skips that do indicate a fault. */
        if (CONN_MGR_GetCentralCount() >= MESH_MAX_CENTRAL)
            return;
        if ((xTaskGetTickCount() - peer->lastSeenTick) >= MESH_PEER_MAX_AGE_TICKS)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Skip DIMMER_%02d: not seen\r\n",
                peer->nodeId);
            continue;
        }
        if (peer->nodeId == MESH_GetNodeId())
            continue;
        /* Edges normally run downwards only, which keeps the direction
           deterministic and prevents reciprocal links. The cost is that the
           highest node id in the network offers child slots nobody can ever
           take, so the usable supply is structurally smaller than the demand
           and one node can be left with no parent at all. A node that has been
           completely isolated is allowed to climb instead, which makes those
           slots reachable and lets the graph always close. */
        if (peer->nodeId > MESH_GetNodeId() && !s_allowUpwardLink)
            continue;
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
        /* Already in the tree and only looking for a better parent: spend the
           single uplink slot on a candidate that is actually an improvement,
           not on the first lower id that happens to have a slot free. Without
           this the rebalance scan would just re-attach at the same depth. */
        if (s_rebalanceScan && !MESH_WouldImproveDepth(peer->rootId, peer->depth))
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Skip DIMMER_%02d: no depth gain\r\n",
                peer->nodeId);
            continue;
        }

        BLE_GAP_CreateConnParams_T params;
        params.scanInterval = APP_BLE_CREATE_CONN_SCAN_INTERVAL;
        params.scanWindow = APP_BLE_CREATE_CONN_SCAN_WINDOW;
        params.filterPolicy = BLE_GAP_SCAN_FP_ACCEPT_ALL;
        params.peerAddr = peer->addr;
        params.connParams.intervalMin = 0x20; /* 40 ms */
        params.connParams.intervalMax = 0x40; /* 80 ms */
        params.connParams.latency = 0;
        /* 20 s. Must comfortably exceed the periodic scan window
           (MESH_SCAN_DURATION_MS), during which this node's connection events
           compete with a 20% duty scan: at 8 s a single scan plus ordinary
           contention was enough to time out healthy links with reason 0x08.
           Failure detection does not depend on this - the mesh ages a silent
           link out after MESH_LINK_DEAD_TICKS. */
        params.connParams.supervisionTimeout = 0x07D0; /* 20 s */

        s_pendingPeerNodeId = peer->nodeId;
        s_connectCancelPending = false;
        s_pendingSinceTick = xTaskGetTickCount();
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connecting to DIMMER_%02d\r\n", peer->nodeId);
        uint16_t status = BLE_GAP_CreateConnection(&params);
        if (status != 0)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "CreateConn FAIL 0x%04X\r\n", status);
            s_pendingPeerNodeId = 0;
            /* These two say nothing about the peer, only about our own stack:
               the initiator is still busy (BLE_GAP_CreateConnectionCancel is
               asynchronous and has not completed yet) or a link to it already
               exists. Backing the peer off here punishes the whole candidate
               list in a single burst - every remaining peer fails the same way
               - and leaves the node with nothing left to try. Give the
               controller a maintenance cycle and start over. */
            if (status == MBA_RES_COMMAND_DISALLOWED ||
                status == MBA_RES_CONN_ALREADY_EXISTS)
                return;
            mesh_BackoffPeer(peer->nodeId);
            continue;
        }
        return;
    }

    /* Downward candidates exhausted without filling the uplink quota. Because
       edges only ever run towards lower node ids, a node whose visible lower
       ids are all full has nowhere left to go and stays out of the mesh - and
       the child slots of the highest ids can never be taken by anyone. Retry
       the same list allowing upward links before giving up. Simulation of the
       partial-visibility case puts orphaned and partitioned topologies at
       13% and 23% without this, and at zero with it. */
    if (s_allowUpwardLink || CONN_MGR_GetCentralCount() >= MESH_MAX_CENTRAL)
        return;
    /* A single link is connected but not redundant: if it is also the path by
       which a broadcast arrived, there is nowhere to forward it. Permit an
       upward edge until the node has two independent local links. */
    if (CONN_MGR_GetLocalLinkCount() >= MESH_MIN_HEALTHY_LINKS)
        return;
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Degraded, allowing upward\r\n");
    s_allowUpwardLink = true;
    s_connectIndex = 0U;
    }
}

void APP_BLE_RescanHandler(void)
{
    static uint32_t lastAdvKickTick = 0;
    uint32_t now = xTaskGetTickCount();

    /* Connect, disconnect, adv-complete and adv-timeout all re-enable
       advertising already. This is only a safety net, so poll it slowly
       instead of issuing an HCI command every maintenance tick.
       A node that silently stops advertising becomes invisible to everyone
       else and can never be reconnected to, so surface the failure: the
       result was being discarded everywhere. MBA_RES_COMMAND_DISALLOWED just
       means it is already advertising, which is the normal case. */
    if ((now - lastAdvKickTick) >= pdMS_TO_TICKS(10000))
    {
        uint16_t advStatus = BLE_GAP_SetAdvEnable(0x01, 0x00);
        lastAdvKickTick = now;
        if (advStatus != MBA_RES_SUCCESS &&
            advStatus != MBA_RES_COMMAND_DISALLOWED)
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "ADV enable FAIL 0x%04X\r\n", advStatus);
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
        uint16_t cancelStatus = BLE_GAP_CreateConnectionCancel();
        if (cancelStatus == MBA_RES_SUCCESS)
        {
            /* CreateConnectionCancel is asynchronous. BLE_GAP_EVT_CONNECTED
               with a failure status is the completion event; until it arrives
               the controller still rejects every new create with 0x010C. */
            mesh_BackoffPeer(s_pendingPeerNodeId);
            s_connectCancelPending = true;
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connect cancel pending\r\n");
        }
        else
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Connect cancel FAIL 0x%04X\r\n",
                cancelStatus);
        }
    }

    if (CONN_MGR_GetLocalLinkCount() > 0U)
    {
        s_lastLinkedTick = now;
    }
    else if ((now - s_lastLinkedTick) >= MESH_ISOLATION_HEAL_TICKS &&
             !s_scanActive && s_pendingPeerNodeId == 0U &&
             !s_connectCancelPending)
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Isolated %ds, resetting discovery\r\n",
            (int)(MESH_ISOLATION_HEAL_TICKS / configTICK_RATE_HZ));
        /* Being alone this long can also mean nobody can see us, so rebuild
           the advertising state rather than only re-issuing the enable. */
        APP_BLE_RestartAdvertising();
        memset(s_discovered, 0, sizeof(s_discovered));
        s_discoveredCount = 0U;
        s_connectIndex = 0U;
        s_pendingPeerNodeId = 0U;
        s_connectCancelPending = false;
        s_lastLinkedTick = now;
        s_repairRequested = true;
        s_lastScanTick = 0U;
    }

    /* A node with one link is not isolated, but it is still a leaf with no
       alternate forwarding path. Never let that state become permanent. */
    if (CONN_MGR_GetLocalLinkCount() >= MESH_MIN_HEALTHY_LINKS)
    {
        s_degradedSinceTick = 0U;
    }
    else
    {
        if (s_degradedSinceTick == 0U)
            s_degradedSinceTick = now;
        else if ((now - s_degradedSinceTick) >= MESH_DEGRADED_HEAL_TICKS &&
                 !s_scanActive && s_pendingPeerNodeId == 0U &&
                 !s_connectCancelPending)
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                "Degraded 120s, rebuilding discovery links=%u\r\n",
                CONN_MGR_GetLocalLinkCount());
            APP_BLE_RestartAdvertising();
            memset(s_discovered, 0, sizeof(s_discovered));
            s_discoveredCount = 0U;
            s_connectIndex = 0U;
            s_allowUpwardLink = false;
            s_repairRequested = true;
            s_lastScanTick = 0U;
            s_degradedSinceTick = now;
        }
    }

    if (!s_scanActive && s_pendingPeerNodeId == 0U &&
        !s_connectCancelPending &&
        !CONN_MGR_HasUnreadyMeshLink() &&
        (s_repairRequested ||
         CONN_MGR_GetLocalLinkCount() < MESH_MIN_HEALTHY_LINKS) &&
        (s_repairRequested || (now - s_lastScanTick) >= MESH_RESCAN_PERIOD_TICKS))
    {
        s_connectIndex = 0U;
        s_allowUpwardLink = false; /* each scan retries downward first */
        s_rebalanceScan = false;
        mesh_SortCandidates();
        if (APP_BLE_StartScan())
        {
            s_repairRequested = false;
            s_lastScanTick = now;
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Periodic topology scan\r\n");
        }
    }
    /* Rebalance scan. Strictly lower priority than the repair scan above: it
       only runs when the node is already linked and idle, has its uplink slot
       free, and sits deep enough for a better parent to be worth the radio.
       s_rebalanceScan makes the candidate filter require a real depth gain. */
    else if (!s_scanActive && s_pendingPeerNodeId == 0U &&
             !s_connectCancelPending &&
             !CONN_MGR_HasUnreadyMeshLink() &&
             CONN_MGR_GetCentralCount() < MESH_MAX_CENTRAL &&
             MESH_WantsRebalance() &&
             (now - s_lastRebalanceTick) >= MESH_REBALANCE_PERIOD_TICKS)
    {
        s_lastRebalanceTick = now;
        s_connectIndex = 0U;
        s_allowUpwardLink = false; /* a parent is always a lower id */
        mesh_SortCandidates();
        if (APP_BLE_StartScan())
        {
            s_rebalanceScan = true;
            s_lastScanTick = now;
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Rebalance scan, depth=%u\r\n",
                MESH_GetDepth());
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
        s_connectCancelPending = false;
        s_repairRequested = true;
        (void)BLE_GAP_SetAdvEnable(true, 0U);
        return;
    }

    if (p_evtConnect->role == 0)
        role = CONN_ROLE_CENTRAL;
    else
        role = CONN_ROLE_PERIPHERAL;

    /* Classify the peer before it has said anything. This node only ever
       initiates towards a DIMMER_xx it has seen advertising, so a central link
       is always a mesh link; an incoming link is a mesh child if its address is
       already in the discovery table and is assumed to be the phone/GUI
       otherwise. The guess is corrected by the first packet either way (JOIN
       retypes it LOCAL, an app frame retypes it PHONE) - it only has to hold
       long enough to keep a peripheral slot free for the app and to keep an
       idle app link out of the mesh discovery gate. */
    bool meshPeer = (role == CONN_ROLE_CENTRAL) ||
        (mesh_FindPeerByAddress(&p_evtConnect->remoteAddr) >= 0);

    if (!CONN_MGR_AddConnection(p_evtConnect->connHandle, role, meshPeer))
    {
        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "AddConn FULL hdl=0x%04X mesh=%d\r\n",
            p_evtConnect->connHandle, (int)meshPeer);
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
        s_connectCancelPending = false;
        type = CONN_TYPE_LOCAL;
        if (s_pendingPeerNodeId != 0)
        {
            CONN_MGR_SetPeerNodeId(p_evtConnect->connHandle, s_pendingPeerNodeId);
            s_pendingPeerNodeId = 0;
        }
        else
        {
            /* The attempt was given up on (the guard timed out and cancelled)
               but the connection landed anyway, so the pending id is gone.
               Recover it from the advertising address instead of leaving the
               link with peer id 0, which makes this node address its JOIN and
               heartbeats to node 0 and hides the peer from
               CONN_MGR_IsConnectedToPeer. */
            int16_t idx = mesh_FindPeerByAddress(&p_evtConnect->remoteAddr);
            if (idx >= 0)
            {
                CONN_MGR_SetPeerNodeId(p_evtConnect->connHandle,
                    s_discovered[idx].nodeId);
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Late connect resolved DIMMER_%02d\r\n",
                    s_discovered[idx].nodeId);
            }
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
    /* Only the loss of a mesh link is a reason to go looking for a new one.
       The app connecting and disconnecting used to force an immediate
       discovery scan every time, and a 20% duty scan window is radio time
       taken from every live connection event - which is how the next app link
       died of supervision timeout (reason 0x08), forcing another scan. */
    bool lostMeshLink = (lost != NULL) &&
        (lost->type == CONN_TYPE_LOCAL || lost->type == CONN_TYPE_GATEWAY ||
         (lost->meshPeer && lost->type != CONN_TYPE_PHONE));
    /* A peripheral or another established link may disconnect while an
       outgoing create/cancel procedure is pending.  Its disconnect event is
       unrelated and must not release the controller gate. */
    if (lostNodeId != 0U)
    {
        if (lifetime >= MESH_LINK_STABLE_TICKS)
            APP_BLE_MarkPeerDisconnected(lostNodeId);
        else
            APP_BLE_MarkPeerUnstable(lostNodeId);
    }
    /* Drops the routing state that still points at this handle. Without it
       s_lastPhoneConnHandle outlives the app link and status responses keep
       being written to a dead handle - or worse, to whatever mesh child the
       controller later reuses that handle for. */
    MESH_OnConnectionClosed(p_evtDisconnect->connHandle);
    CONN_MGR_RemoveConnection(p_evtDisconnect->connHandle);
    /* A simultaneous reciprocal connection may have just been removed. Do
       not declare the parent lost when the surviving link reaches the same
       peer. */
    if (lostNodeId == 0U || !CONN_MGR_IsConnectedToPeer(lostNodeId))
        MESH_OnLinkLost(lostNodeId);
    MESH_TopologyChanged();
    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Disconnected hdl=0x%04X reason=0x%02X %s\r\n",
        p_evtDisconnect->connHandle, p_evtDisconnect->reason,
        lostMeshLink ? "mesh" : "app");

    BLE_GAP_SetAdvEnable(0x01, 0x00);
    if (lostMeshLink)
    {
        s_repairRequested = true;
        s_lastScanTick = 0U;
    }
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
                    if (s_discovered[peerIndex].reportedLost)
                    {
                        s_discovered[peerIndex].reportedLost = false;
                        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "PEER BACK DIMMER_%02d\r\n",
                            peerNodeId);
                    }
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
                    s_discovered[peerIndex].reportedLost = false;
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

    /* Same 30 s budget as the periodic sweep in APP_Tasks: GATT discovery plus
       CCC setup can take well over 15 s under load, and sweeping earlier tears
       down links that were about to come up. */
    CONN_MGR_SweepStale(pdMS_TO_TICKS(30000));

    BLE_GAP_SetAdvEnable(0x01, 0x00);

    /* Report a peer that has stopped advertising, once, from whichever node
       happens to be monitored. A node that already has all its uplinks never
       walks the candidate list, so without this a healthy neighbour gives no
       sign at all that somebody dropped off the air - and which node fails is
       not predictable, so it cannot be watched directly. */
    {
        uint8_t i;
        uint32_t now = xTaskGetTickCount();
        for (i = 0U; i < s_discoveredCount; i++)
        {
            if (s_discovered[i].reportedLost || s_discovered[i].nodeId == 0U ||
                CONN_MGR_IsConnectedToPeer(s_discovered[i].nodeId))
                continue;
            if ((now - s_discovered[i].lastSeenTick) >= MESH_PEER_MAX_AGE_TICKS)
            {
                s_discovered[i].reportedLost = true;
                SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                    "PEER LOST DIMMER_%02d (silent %us)\r\n",
                    s_discovered[i].nodeId,
                    (unsigned)((now - s_discovered[i].lastSeenTick) /
                        configTICK_RATE_HZ));
            }
        }
    }

    mesh_SortCandidates();
    s_connectIndex = 0;
    s_allowUpwardLink = false; /* each scan retries downward first */
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
