// DOM-IGNORE-BEGIN
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
// DOM-IGNORE-END

/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <string.h>
#include "app.h"
#include "definitions.h"
#include "app_ble.h"
#include "app_ble_callbacks.h"
#include "peripheral/sercom/usart/plib_sercom0_usart.h"
#include "ble_trspc/ble_trspc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ble_gap.h"
#include "node_config.h"
#include "mesh_routing.h"
#include "mesh_conn_mgr.h"
#include "led_dimmer.h"
#include "provisioning.h"

#define APP_BLE_SCAN_DURATION       10000




// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
uint16_t conn_hdl[CONFIG_APP_BLE_MAXIMUM_NUMBER_OF_LINKS] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
uint8_t no_of_links;
static uint8_t uart_data;



// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

void uart_cb(SERCOM_USART_EVENT event, uintptr_t context)
{
    APP_Msg_T appMsg;
    if (event == SERCOM_USART_EVENT_READ_THRESHOLD_REACHED)
    {
        SERCOM0_USART_Read(&uart_data, 1);
        appMsg.msgId = APP_MSG_UART_CB;
        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    }
}

static void APP_UartDebugHandler(void)
{
    static uint8_t cmdBuf[16];
    static uint8_t cmdIdx = 0;

    if (uart_data == '\r' || uart_data == '\n')
    {
        if (cmdIdx > 0)
        {
            cmdBuf[cmdIdx] = 0;
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "CMD: %s\r\n", cmdBuf);
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Node=%d Peers=%d\r\n",
                NODE_ID, CONN_MGR_GetActiveCount());
            cmdIdx = 0;
        }
    }
    else if (cmdIdx < 15)
    {
        cmdBuf[cmdIdx++] = uart_data;
    }
}



// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/



// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;


    appData.appQueue = xQueueCreate( 64, sizeof(APP_Msg_T) );
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */

}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    APP_Msg_T    appMsg[1];
    APP_Msg_T   *p_appMsg;
    p_appMsg=appMsg;


/* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;

            PROV_Init();

            APP_BleStackInit();

            SERCOM0_USART_ReadNotificationEnable(true, true);
            SERCOM0_USART_ReadThresholdSet(1);
            SERCOM0_USART_ReadCallbackRegister(uart_cb, (uintptr_t)NULL);

            uint8_t activeNodeId = PROV_GetNodeId();
            if (activeNodeId == 0)
            {
                BLE_GAP_Addr_T myAddr;
                BLE_GAP_GetDeviceAddr(&myAddr);
                activeNodeId = (myAddr.addr[0] % 99) + 1;
            }

            CONN_MGR_Init();
            MESH_Init(activeNodeId);
            LED_Dimmer_Init();

            if (!PROV_IsProvisioned())
            {
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "\r\n=== UNPROVISIONED (temp ID=%d) ===\r\n", (int)activeNodeId);
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Provision: 00 00 00 05 05 XX\r\n");
                LED_Dimmer_SetRGB(0, 0, 0);
            }
            else
            {
                SYS_DEBUG_PRINT(SYS_ERROR_INFO, "\r\n=== DIMMER_%02d (Provisioned) ===\r\n", (int)activeNodeId);
                LED_Dimmer_SetRGB(0, 100, 0);
            }

            BLE_GAP_SetAdvEnable(0x01, 0x00);
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Advertising...\r\n");

            vTaskDelay(pdMS_TO_TICKS(1000));

            BLE_GAP_SetScanningEnable(true, BLE_GAP_SCAN_FD_ENABLE, BLE_GAP_SCAN_MODE_OBSERVER, APP_BLE_SCAN_DURATION);
            SYS_DEBUG_PRINT(SYS_ERROR_INFO, "Scanning for peers...\r\n");

            if (appInitialized)
            {
                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            if (OSAL_QUEUE_Receive(&appData.appQueue, &appMsg, pdMS_TO_TICKS(5000)))
            {
                if(p_appMsg->msgId==APP_MSG_BLE_STACK_EVT)
                {
                    APP_BleStackEvtHandler((STACK_Event_T *)p_appMsg->msgData);
                }
                else if(p_appMsg->msgId==APP_MSG_UART_CB)
                {
                    APP_UartDebugHandler();
                }
                else if(p_appMsg->msgId==APP_MSG_RESCAN)
                {
                    APP_BLE_RescanHandler();
                }
            }
            else
            {
                APP_BLE_RescanHandler();
                APP_BLE_ConnectNextPeer();
            }
            break;
        }
        
        /* TODO: implement your application state machine.*/
     

        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
