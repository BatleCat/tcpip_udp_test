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

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <peripheral/peripheral.h>
#include <proc/p32mz2048efh144.h>

#include "app.h"
#include "app_commands.h"
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

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

/* TODO:  Add any necessary callback functions.
*/
void tmr_callBack( uintptr_t context, uint32_t alarmCount )
{
    appData.bflag_UDP_enable = true;
    SYS_PORTS_PinToggle(PORTS_ID_0, PORT_CHANNEL_D, PORTS_BIT_POS_10);
}
// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/
void UDP_Tasks(void)
{
    const char          *netName, *netBiosName;
    static IPV4_ADDR    dwLastIP[2] = { {-1}, {-1} };
    IPV4_ADDR           ipAddr;
    TCPIP_NET_HANDLE    netH;
    
    switch (appData.udp_state)
    {
        case UDP_STATE_INIT:
        {
            break;
        }
        case UDP_STATE_Start:
        {
            SYS_STATUS tcpipStat = TCPIP_STACK_Status(sysObj.tcpip);
            if(tcpipStat < 0)
            {   // some error occurred
                SYS_CONSOLE_MESSAGE(" APP: TCP/IP stack initialization failed!\r\n");
                SYS_CONSOLE_MESSAGE(" APP: UDP_STATE_Error\r\n");
                appData.udp_state = UDP_STATE_Error;
            }
            else if(SYS_STATUS_READY == tcpipStat)
            {
                int nNets, i;
                
                nNets = TCPIP_STACK_NumberOfNetworksGet();
                for(i = 0; i < nNets; i++)
                {

                    netH = TCPIP_STACK_IndexToNet(i);
                    netName = TCPIP_STACK_NetNameGet(netH);
                    netBiosName = TCPIP_STACK_NetBIOSName(netH);

                    SYS_CONSOLE_PRINT("    Interface %s on host %s - NBNS disabled\r\n", netName, netBiosName);

                }
                SYS_CONSOLE_MESSAGE(" APP: UDP_STATE_WAIT_FOR_IP\r\n");
                appData.udp_state = UDP_STATE_WAIT_FOR_IP;
            }
            break;
        }
        case UDP_STATE_WAIT_FOR_IP:
        {
            int i;
            int nNets = TCPIP_STACK_NumberOfNetworksGet();

            for (i = 0; i < nNets; i++)
            {
                netH = TCPIP_STACK_IndexToNet(i);
                if(false == TCPIP_STACK_NetIsReady(netH))
                {
//                    SYS_CONSOLE_MESSAGE("APP: Net not ready \r\n");//(int)TCPIP_STACK_NetIsReady(netH));
//                    SYS_CONSOLE_MESSAGE(" APP: UDP_STATE_Error\r\n");
//                    appData.udp_state = UDP_STATE_Error;
                    return; // interface not ready yet!
                }
                ipAddr.Val = TCPIP_STACK_NetAddress(netH);
//                if(dwLastIP[i].Val != ipAddr.Val)
                {
                    dwLastIP[i].Val = ipAddr.Val;

                    SYS_CONSOLE_MESSAGE(TCPIP_STACK_NetNameGet(netH));
                    SYS_CONSOLE_MESSAGE(" IP Address: ");
                    SYS_CONSOLE_PRINT("%d.%d.%d.%d \r\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);

                }
            }

            SYS_CONSOLE_MESSAGE(" APP: UDP_STATE_WAIT_SERVER_OPEN\r\n");
            appData.udp_state = UDP_STATE_WAIT_SERVER_OPEN;
            
            break;
        }
        case UDP_STATE_WAIT_SERVER_OPEN:
        {
            IP_MULTI_ADDRESS dest_adr = {.v4Add.v = {10, 2, 22, 220}};
            UDP_PORT  dest_port = 1500;

            IP_MULTI_ADDRESS local_adr = {.v4Add.v = {10, 2, 22, 245}};
            UDP_PORT  local_port = 1500;

            appData.udp_rx_socket = TCPIP_UDP_ServerOpen(IP_ADDRESS_TYPE_IPV4, local_port, &local_adr);
            if (appData.udp_rx_socket != INVALID_SOCKET)
            {
                if (true == TCPIP_UDP_Bind(appData.udp_rx_socket, IP_ADDRESS_TYPE_IPV4, local_port, &local_adr))
                {
                    SYS_CONSOLE_MESSAGE(" APP: UDP server open\r\n");
                }
                SYS_CONSOLE_MESSAGE(" APP: UDP_STATE_WAIT_CLIENT_OPEN\r\n");
                appData.udp_state = UDP_STATE_WAIT_CLIENT_OPEN;
            }
            break;
        }
        case UDP_STATE_WAIT_CLIENT_OPEN:
        {
//            IP_MULTI_ADDRESS dest_adr = {.v4Add.v = {10, 2, 22, 220}};
//            UDP_PORT  dest_port = 1500;
//
//            IP_MULTI_ADDRESS local_adr = {.v4Add.v = {10, 2, 22, 245}};
//            UDP_PORT  local_port = 1500;
//
//            appData.udp_tx_socket = TCPIP_UDP_ClientOpen(IP_ADDRESS_TYPE_IPV4, dest_port, 0);//&dest_adr);
//            if (appData.udp_tx_socket != INVALID_SOCKET)
//            {
//                if (true == TCPIP_UDP_Bind(appData.udp_tx_socket, IP_ADDRESS_TYPE_IPV4, local_port, &local_adr))
//                {
//                    SYS_CONSOLE_MESSAGE(" APP: UDP client open\r\n");
//                }
                SYS_CONSOLE_MESSAGE(" APP: UDP_STATE_WAIT_FOR_CONNECTION\r\n");
                appData.udp_state = UDP_STATE_WAIT_FOR_CONNECTION;
//            }
            break;
        }
        case UDP_STATE_WAIT_FOR_CONNECTION:
        {
//            if (TCPIP_UDP_IsConnected(appData.udp_tx_socket))
            if (TCPIP_UDP_IsConnected(appData.udp_rx_socket))
            {
                SYS_CONSOLE_MESSAGE(" APP: TCPIP_UDP_IsConnected\r\n");
                appData.udp_state = UDP_STATE_Rx;
            }
            break;
        }
        case UDP_STATE_Rx:
        {
            if (appData.bflag_UDP_enable)
            {
                appData.bflag_UDP_enable = false;
                appData.udp_state = UDP_STATE_Tx;
                
                appData.udp_rx_data_len = TCPIP_UDP_GetIsReady(appData.udp_rx_socket);
                
                if (appData.udp_rx_data_len > 0)
                {
                    uint16_t read_len = TCPIP_UDP_ArrayGet(appData.udp_rx_socket, appData.udp_rx_data, appData.udp_rx_data_len);
                    
                    int i;
                    for (i = 0; i < read_len; i++) appData.udp_tx_data[i] = appData.udp_rx_data[i];

                    appData.udp_tx_data_len = read_len;
                    appData.bflag_UDP_send_data = true;
                    
//                    SYS_CONSOLE_MESSAGE(" APP: an UDP-packet receive\r\n");
                }
            }
            break;
        }
        
        case UDP_STATE_Tx:
        {
            if (appData.bflag_UDP_enable)
            {
                appData.bflag_UDP_enable = false;
                appData.udp_state = UDP_STATE_Rx;
                
                if (appData.bflag_UDP_send_data)
                {
//                    SYS_CONSOLE_MESSAGE(" APP: UDP-packet send start\r\n");
                    uint16_t udp_buf_len;
//                    udp_buf_len = TCPIP_UDP_PutIsReady(appData.udp_tx_socket);
                    udp_buf_len = TCPIP_UDP_PutIsReady(appData.udp_rx_socket);
//                    SYS_CONSOLE_PRINT("APP: tx_bufer size = %d \r\n", udp_buf_len);
                    if (udp_buf_len >= appData.udp_tx_data_len)// 1472)
                    {
//                        TCPIP_UDP_ArrayPut(appData.udp_tx_socket, appData.udp_tx_data, appData.udp_tx_data_len);
//                        TCPIP_UDP_Flush(appData.udp_tx_socket);
                        TCPIP_UDP_ArrayPut(appData.udp_rx_socket, appData.udp_tx_data, appData.udp_tx_data_len);
                        TCPIP_UDP_Flush(appData.udp_rx_socket);
//                        SYS_CONSOLE_MESSAGE(" APP: UDP-packet send complite\r\n");
                        
                        appData.udp_tx_data_len = 1472;
                        int i;
                        for (i = 0; i < appData.udp_tx_data_len; i++) appData.udp_tx_data[i] = (uint8_t)i;
                        
//                        appData.udp_state = UDP_STATE_Rx;
                        appData.bflag_UDP_send_data = false;
                    }
                    else
                    {
//                        appData.bflag_UDP_enable = true;
                    }
                }
                else
                {
//                    appData.udp_state = UDP_STATE_Rx;
                }
            }
            break;
        }
        case UDP_STATE_CLOSING_CONNECTION:
        {
            break;
        }
        case UDP_STATE_Error:
        {
            break;
        }
        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


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
    
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
    appData.bflag_UDP_enable    = false;
    appData.bflag_UDP_send_data = false;
    appData.udp_state  = UDP_STATE_INIT;
    appData.tmr_handle = DRV_HANDLE_INVALID;
    APP_Commands_Init();
    
    appData.udp_rx_data_len = 0;
    appData.udp_tx_data_len = 1472;
    
    int i;
    for (i = 0; i < 1472; i++) appData.udp_rx_data[i] = 0;
    for (i = 0; i < 1472; i++) appData.udp_tx_data[i] = 0;
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = false;
       
            appData.tmr_handle = DRV_TMR_Open(DRV_TMR_INDEX_1, DRV_IO_INTENT_EXCLUSIVE);
            if (DRV_HANDLE_INVALID != appData.tmr_handle)
            {
                if (true == DRV_TMR_AlarmRegister(appData.tmr_handle, 4*96, true, 0, &tmr_callBack))
                {
                    SYS_PORTS_PinSet(PORTS_ID_0, PORT_CHANNEL_D, PORTS_BIT_POS_10);
                    appInitialized = true;
//                    DRV_TMR_Start(appData.tmr_handle);
                }
                else
                {
                    DRV_TMR_Close(appData.tmr_handle);
                    appData.tmr_handle = DRV_HANDLE_INVALID;
                }
            }
            
            if (appInitialized)
            {
                appData.state = APP_STATE_SERVICE_TASKS;
                appData.udp_state = UDP_STATE_Start;
                SYS_PORTS_PinClear(PORTS_ID_0, PORT_CHANNEL_D, PORTS_BIT_POS_10);
                SYS_PORTS_PinSet(PORTS_ID_0, PORT_CHANNEL_H, PORTS_BIT_POS_0);
                SYS_CONSOLE_MESSAGE(" APP: Initialization complite\r\n");
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            SYS_CMD_READY_TO_READ();
            
            if (APP_Send_Packet)
            {
                APP_Send_Packet = false;
                
                SYS_CONSOLE_MESSAGE(" APP: Start UDP transmit command received\r\n");
                SYS_PORTS_PinToggle(PORTS_ID_0, PORT_CHANNEL_H, PORTS_BIT_POS_0);
                
                DRV_TMR_Stop(appData.tmr_handle);
                DRV_TMR_CounterClear(appData.tmr_handle);
                DRV_TMR_AlarmPeriodSet(appData.tmr_handle, 96);
                DRV_TMR_AlarmEnable(appData.tmr_handle, true);
                DRV_TMR_Start(appData.tmr_handle);
                
                SYS_PORTS_PinSet(PORTS_ID_0, PORT_CHANNEL_D, PORTS_BIT_POS_10);

                appData.udp_tx_data_len = 1472;
                int i;
                for (i = 0; i < appData.udp_tx_data_len; i++) appData.udp_tx_data[i] = (uint8_t)i;

                appData.bflag_UDP_send_data = true;

            }
            
            if (APP_Stop_Packet)
            {
                APP_Stop_Packet = false;
                
                SYS_CONSOLE_MESSAGE(" APP: Stop UDP transmit command received\r\n");
                SYS_PORTS_PinToggle(PORTS_ID_0, PORT_CHANNEL_H, PORTS_BIT_POS_0);
                
                DRV_TMR_Stop(appData.tmr_handle);
                DRV_TMR_CounterClear(appData.tmr_handle);
                
                SYS_PORTS_PinSet(PORTS_ID_0, PORT_CHANNEL_D, PORTS_BIT_POS_10);

                appData.bflag_UDP_send_data = false;

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
    
    UDP_Tasks();
}
/*******************************************************************************
 End of File
 */
