#include <string.h>
#include "app_trsps_handler.h"
#include "osal/osal_freertos.h"
#include "mesh_routing.h"
#include "mesh_conn_mgr.h"
#include "node_config.h"
#include "led_dimmer.h"
#include "definitions.h"
#include "ble_gap.h"
#include "gatt.h"

static uint8_t hexCharToNibble(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0xFF;
}

static bool isHexSeparator(uint8_t c)
{
    return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') ||
           (c == ':') || (c == ',') || (c == '-');
}

static bool isAsciiHexPacket(const uint8_t *data, uint16_t len)
{
    uint16_t digits = 0;
    uint16_t i;
    for (i = 0; i < len; i++)
    {
        if (isHexSeparator(data[i]))
            continue;
        if (hexCharToNibble(data[i]) == 0xFFU)
            return false;
        digits++;
    }
    return (digits >= (MESH_HEADER_SIZE * 2U)) && ((digits & 1U) == 0U);
}

static uint16_t parseHexString(uint8_t *asciiIn, uint16_t asciiLen, uint8_t *binOut, uint16_t binMax)
{
    uint16_t binIdx = 0;
    uint16_t i = 0;

    while (i < asciiLen && binIdx < binMax)
    {
        while (i < asciiLen && isHexSeparator(asciiIn[i]))
            i++;

        if (i + 1 >= asciiLen) break;

        uint8_t hi = hexCharToNibble(asciiIn[i]);
        uint8_t lo = hexCharToNibble(asciiIn[i + 1]);

        if (hi == 0xFF || lo == 0xFF) break;

        binOut[binIdx++] = (hi << 4) | lo;
        i += 2;
    }
    return binIdx;
}

void APP_TrspsEvtHandler(BLE_TRSPS_Event_T *p_event)
{
    switch (p_event->eventId)
    {
        case BLE_TRSPS_EVT_CTRL_STATUS:
        {
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "TRSPS CTRL\r\n");
        }
        break;

        case BLE_TRSPS_EVT_TX_STATUS:
        {
            if (p_event->eventField.onTxStatus.status == BLE_TRSPS_STATUS_TX_OPENED)
            {
                uint16_t hdl = p_event->eventField.onTxStatus.connHandle;
                CONN_MGR_SetReady(hdl);
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "TRSPS TX ready hdl=0x%04X\r\n", hdl);

                MeshConn_T *conn = CONN_MGR_GetByHandle(hdl);
                if (conn && conn->role == CONN_ROLE_PERIPHERAL)
                {
                    BLE_GAP_ConnParams_T params;
                    params.intervalMin = 0x20; /* 40 ms */
                    params.intervalMax = 0x40; /* 80 ms */
                    params.latency = 0;
                    params.supervisionTimeout = 0x07D0; /* 20 s, see central side */
                    BLE_GAP_UpdateConnParam(hdl, &params);
                }
            }
        }
        break;

        case BLE_TRSPS_EVT_RECEIVE_DATA:
        {
            uint16_t dataLen = 0;
            uint16_t result;
            /* Sized for the largest write the peer can make (ATT MTU minus the
               write header): BLE_TRSPS_GetData copies the stored length with
               no bound check. Static because this handler only ever runs in
               the app task, and 244 bytes do not belong on its stack. */
            static uint8_t rawBuf[BLE_ATT_MAX_MTU_LEN - ATT_WRITE_HEADER_SIZE];
            static uint8_t binBuf[32];
            uint16_t binLen;
            bool isAsciiHex;

            /* Drain every queued packet. BLE_TRSPS_GetData is the only place
               that returns a CBFC credit to the sender, and the pool is
               granted once (16 credits, never replenished otherwise). Any
               packet left in the queue - because it was oversized, or because
               its RECEIVE_DATA event was dropped by a full app queue - costs
               one credit permanently. After 16 the peer's BLE_TRSPC_SendData
               fails with MBA_RES_NO_RESOURCE for ever and the link goes mute
               in one direction: broadcast works, then silently stops. */
            uint16_t hdl = p_event->eventField.onReceiveData.connHandle;

            for (;;)
            {
                MeshConn_T *conn;

                BLE_TRSPS_GetDataLength(hdl, &dataLen);
                /* GetDataLength reports 0 both for an empty queue and for a
                   zero-length packet, so always attempt the dequeue: a failing
                   GetData is the only reliable "queue is empty". */
                result = BLE_TRSPS_GetData(hdl, rawBuf);
                if (result != 0)
                    break;
                if (dataLen == 0U || dataLen > sizeof(rawBuf))
                    continue; /* dequeued, so the credit is returned; unusable */

                isAsciiHex = isAsciiHexPacket(rawBuf, dataLen);
                conn = CONN_MGR_GetByHandle(hdl);

                if (!isAsciiHex && dataLen >= 5)
                {
                    if (conn && rawBuf[1] == NODE_ID_PHONE)
                        CONN_MGR_SetType(hdl, CONN_TYPE_PHONE);
                    else if (conn && conn->type != CONN_TYPE_LOCAL)
                        CONN_MGR_SetType(hdl, CONN_TYPE_LOCAL);
                    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "BIN dst=%02X cmd=%02X\r\n",
                        (int)rawBuf[0], (int)rawBuf[4]);
                    MESH_ProcessIncoming(hdl, dataLen, rawBuf);
                }
                else if (isAsciiHex)
                {
                    if (conn && conn->type != CONN_TYPE_PHONE)
                        CONN_MGR_SetType(hdl, CONN_TYPE_PHONE);
                    binLen = parseHexString(rawBuf, dataLen, binBuf, sizeof(binBuf));
                    SYS_DEBUG_PRINT(SYS_ERROR_INFO, "HEX parsed %d B\r\n", (int)binLen);
                    if (binLen >= 5)
                        MESH_ProcessIncoming(hdl, binLen, binBuf);
                }
            }
        }
        break;

        default:
            break;
    }
}
