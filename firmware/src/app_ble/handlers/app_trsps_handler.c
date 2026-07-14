#include <string.h>
#include "app_trsps_handler.h"
#include "osal/osal_freertos.h"
#include "mesh_routing.h"
#include "mesh_conn_mgr.h"
#include "led_dimmer.h"
#include "definitions.h"
#include "ble_gap.h"

static uint8_t hexCharToNibble(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0xFF;
}

static uint16_t parseHexString(uint8_t *asciiIn, uint16_t asciiLen, uint8_t *binOut, uint16_t binMax)
{
    uint16_t binIdx = 0;
    uint16_t i = 0;

    while (i < asciiLen && binIdx < binMax)
    {
        while (i < asciiLen && (asciiIn[i] == ' ' || asciiIn[i] == '\r' || asciiIn[i] == '\n'))
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
                    params.intervalMin = 0x50;
                    params.intervalMax = 0xA0;
                    params.latency = 0;
                    params.supervisionTimeout = 0x0190;
                    BLE_GAP_UpdateConnParam(hdl, &params);
                }
            }
        }
        break;

        case BLE_TRSPS_EVT_RECEIVE_DATA:
        {
            uint16_t dataLen = 0;
            uint16_t result;
            uint8_t rawBuf[64];
            uint8_t binBuf[32];
            uint16_t binLen;
            bool isAsciiHex = false;
            uint16_t idx;

            BLE_TRSPS_GetDataLength(p_event->eventField.onReceiveData.connHandle, &dataLen);

            if (dataLen > 0 && dataLen <= 64)
            {
                result = BLE_TRSPS_GetData(p_event->eventField.onReceiveData.connHandle, rawBuf);
                if (result == 0)
                {
                    for (idx = 0; idx < dataLen; idx++)
                    {
                        if (rawBuf[idx] == ' ')
                        {
                            isAsciiHex = true;
                            break;
                        }
                    }

                    if (!isAsciiHex && dataLen >= 5)
                    {
                        MeshConn_T *conn = CONN_MGR_GetByHandle(p_event->eventField.onReceiveData.connHandle);
                        if (conn && conn->type != CONN_TYPE_LOCAL)
                            CONN_MGR_SetType(p_event->eventField.onReceiveData.connHandle, CONN_TYPE_LOCAL);
                        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "BIN dst=%02X cmd=%02X\r\n", (int)rawBuf[0], (int)rawBuf[4]);
                        MESH_ProcessIncoming(p_event->eventField.onReceiveData.connHandle, dataLen, rawBuf);
                    }
                    else if (isAsciiHex)
                    {
                        MeshConn_T *conn = CONN_MGR_GetByHandle(p_event->eventField.onReceiveData.connHandle);
                        if (conn && conn->type != CONN_TYPE_PHONE)
                            CONN_MGR_SetType(p_event->eventField.onReceiveData.connHandle, CONN_TYPE_PHONE);
                        binLen = parseHexString(rawBuf, dataLen, binBuf, 32);
                        SYS_DEBUG_PRINT(SYS_ERROR_INFO, "HEX parsed %d B\r\n", (int)binLen);
                        if (binLen >= 5)
                        {
                            MESH_ProcessIncoming(p_event->eventField.onReceiveData.connHandle, binLen, binBuf);
                        }
                    }
                }
            }
        }
        break;

        default:
            break;
    }
}
