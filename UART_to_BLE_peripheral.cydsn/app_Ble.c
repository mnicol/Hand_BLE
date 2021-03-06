/*******************************************************************************
* File Name: app_Ble.c
*
* Description:
*  Common BLE application code for client devices.
*
*******************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "app_ble.h"

/* MTU size to be used by Client and Server after MTU exchange */
uint16 mtuSize      = CYBLE_GATT_MTU;   

uint8 txDataClientConfigDesc[2] = {0, 0};   
CYBLE_GATT_HANDLE_VALUE_PAIR_T  attrValue = { 
                                                    {(uint8 *)txDataClientConfigDesc, 2, 2}, 
                                                    CYBLE_SERVER_UART_SERVER_UART_TX_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE
                                                };

/*******************************************************************************
* Function Name: HandleBleProcessing
********************************************************************************
*
* Summary:
*   Handles the BLE state machine for intiating different procedures
*   during different states of BLESS.
*
* Parameters:
*   None.
*
* Return:
*   None.
*
*******************************************************************************/
void HandleBleProcessing(void)
{    
    
    switch (cyBle_state)
    {
        case CYBLE_STATE_ADVERTISING:
            break;
        
        case CYBLE_STATE_CONNECTED:
            
            /* read CCCD for TX characteristic for checking if notifications are enabled*/
            CyBle_GattsReadAttributeValue(&attrValue, &cyBle_connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
            
            /* if stack is free, handle UART traffic */
            if(CyBle_GattGetBusStatus() != CYBLE_STACK_STATE_BUSY)
            {
                //TODO send data here
                // This event gets called when U12 is pressed because it sends a signal to P1[4] which is uart rx
                
                HandleUartTxTraffic((uint16)txDataClientConfigDesc[0]);
            }
            break;
                
        case CYBLE_STATE_DISCONNECTED:
        
            txDataClientConfigDesc[0] = NOTIFICATON_DISABLED;
            txDataClientConfigDesc[1] = NOTIFICATON_DISABLED;
            
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);

            break;
       
        case CYBLE_STATE_INITIALIZING:
        case CYBLE_STATE_STOPPED:
        default:
            break;       
    }
}


/*******************************************************************************
* Function Name: AppCallBack
********************************************************************************
*
* Summary:
*   Call back function for BLE stack to handle BLESS events
*
* Parameters:
*   event       - the event generated by stack
*   eventParam  - the parameters related to the corresponding event
*
* Return:
*   None.
*
*******************************************************************************/
void AppCallBack(uint32 event, void *eventParam)
{   
    CYBLE_GATT_ERR_CODE_T           errorCode;
    CYBLE_GATTS_WRITE_REQ_PARAM_T   *writeReqParam;
    
    switch (event)
    {
        case CYBLE_EVT_STACK_ON:
            break;
            
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            
            #ifdef PRINT_MESSAGE_LOG   
                UART_UartPutString("\n\r DISCONNECTED!!! \n\r ");
                while(0 != (UART_SpiUartGetTxBufferSize() + UART_GET_TX_FIFO_SR_VALID));
            #endif
            
            /* RESET Uart and flush all buffers */
            UART_Stop();
            UART_SpiUartClearTxBuffer();
            UART_SpiUartClearRxBuffer();
            UART_Start();
            break;
            
        case CYBLE_EVT_GATT_CONNECT_IND:
            
            #ifdef PRINT_MESSAGE_LOG   
                UART_UartPutString("\n\rConnection established");             
            #endif
            
            break;
        
        case CYBLE_EVT_GATTS_WRITE_CMD_REQ:
            
            HandleUartRxTraffic((CYBLE_GATTS_WRITE_REQ_PARAM_T *) eventParam);
            
            break;
        
        case CYBLE_EVT_GATTS_XCNHG_MTU_REQ:
            
            if(CYBLE_GATT_MTU > ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu)
            {
                mtuSize = ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu;
            }
            else
            {
                mtuSize = CYBLE_GATT_MTU;
            }
            
            break;
            
        case CYBLE_EVT_GATTS_WRITE_REQ:
            
            writeReqParam = (CYBLE_GATTS_WRITE_REQ_PARAM_T *) eventParam;
            
            if(CYBLE_SERVER_UART_SERVER_UART_TX_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE == \
                                                                    writeReqParam->handleValPair.attrHandle)
            {
                errorCode = CyBle_GattsWriteAttributeValue(&(writeReqParam->handleValPair), \
                                                0, &cyBle_connHandle, CYBLE_GATT_DB_PEER_INITIATED);
                
                if (CYBLE_GATT_ERR_NONE  == errorCode)
                {
                    CyBle_GattsWriteRsp(cyBle_connHandle);
                    #ifdef PRINT_MESSAGE_LOG   
                        UART_UartPutString("\n\rNotifications enabled\n\r");
                        UART_UartPutString("\n\rStart entering data:\n\r");
                    #endif
                }
            }
            
            break;
        
        default:
            break;
    }
}

/* [] END OF FILE */
