/*******************************************************************************
* File Name: bt_app.c
*
* Description: This file contains the task that handles bluetooth events and
* notifications.
*
* Related Document: See README.md
*
********************************************************************************
 * (c) 2023-2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header file includes
*******************************************************************************/
#include "cycfg_gap.h"
#include "cybsp_bt_config.h"
#include "cycfg_gatt_db.h"
#include "cycfg_bt_settings.h"
#include "wiced_bt_types.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_stack.h"
#include "bt_app.h"
#include "board.h"
#include "i2c_capsense.h"
#include "retarget_io_init.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define FILLED_VALUE_ZERO                           (0U)
#define VTASK_DELAY_TICKS                           (10U)
#define INIT_VALUE_ZERO                             (0U)
#define BITS_TO_CLEAR_ON_ENTRY                      (0U)
#define BITS_TO_CLEAR_ON_EXIT                       (0U)
#define SET_DUTY_CYCLE_100                          (1000U)
#define SET_DUTY_CYCLE_50                           (500U)
#define BUTTON_COUNT                                (0U)
#define BUTTON_STATUS                               (1U)
#define SLIDER_DATA                                 (0U)
#define APP_CAPSENSE_BUTTON                         (2U)
#define APP_CAPSENSE_SLIDER                         (4U)
#define NOTIFICATION_ENABLED                        (0U)
#define CAPSENSE_BUTTON_CLIENT_CHAR_CONFIG_LEN      (2U)
#define CAPSENSE_SLIDER_CLIENT_CHAR_CONFIG_LEN      (2U)
#define NO_OF_CAPSENSE_BUTTONS                      (2U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Holds the connection ID */
volatile uint16_t bt_connection_id = INIT_VALUE_ZERO;

/* Typdef for function used to free allocated buffer to stack */
typedef void (*pfn_free_buffer_t)(uint8_t *);

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: bt_task
********************************************************************************
* Summary:
*  Task that handles Bluetooth initialization and updates GATT notification data.
*
* Parameters:
*  void *param : Task parameter defined during task creation (unused)
*
* Return:
*  None
*
*******************************************************************************/
void bt_task(void* param)
{
    static uint32_t nofify_value;

    /* Suppress warning for unused parameter */
     CY_UNUSED_PARAMETER(param);

    /* Repeatedly running part of the task */
    for(;;)
    {
        /* Block till a notification is received. */
        xTaskNotifyWait(BITS_TO_CLEAR_ON_ENTRY, BITS_TO_CLEAR_ON_EXIT,
                &nofify_value, portMAX_DELAY);

       /* Command has been received from queue */
        if(NOTIFIY_ON == nofify_value)
        {
            bt_app_send_notification();
        }

        vTaskDelay(pdMS_TO_TICKS(VTASK_DELAY_TICKS));
    }
}

/*******************************************************************************
* Function Name: bt_app_init
********************************************************************************
* Summary:
*  This function handles application level initialization tasks and is
*  called from the BT management callback once the LE stack enabled event
*  (BTM_ENABLED_EVT) is triggered This function is executed in the
*  BTM_ENABLED_EVT management callback.
*******************************************************************************/
void bt_app_init(void)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    printf("Discover the device with name: \"%s\"\r\n", app_gap_device_name);

    /* Register with BT stack to receive GATT callback */
    status = wiced_bt_gatt_register(bt_app_gatt_event_cb);
    printf("GATT event handler registration status: %d \r\n",status);

    /* Initialize GATT Database */
    status = wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);
    printf("GATT database initialization status: %d \r\n",status);

    /* Allow peer to pair */
    wiced_bt_set_pairable_mode(FALSE, FALSE);

    /* Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, 
                                            cy_bt_adv_packet_data);

    /* Start Undirected LE Advertisements on device startup.
     * The corresponding parameters are contained in 'app_bt_cfg.c' 
     */
    result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH,
            BLE_ADDR_PUBLIC, NULL);

    /* Failed to start advertisement. Stop program execution */
    if (WICED_BT_SUCCESS != result)
    {
        printf("Failed to start advertisement! \r\n");
        handle_app_error();
    }
}

/*******************************************************************************
* Function Name: bt_app_management_cb
********************************************************************************
* Summary:
*  This is a Bluetooth stack event handler function to receive management events
*  from the LE stack and process as per the application.
*
* Parameters:
*  wiced_bt_management_evt_t event             : LE event code of one byte length
*  wiced_bt_management_evt_data_t *p_event_data: Pointer to LE management event
*                                                 structures
*
* Return:
*  wiced_result_t: Error code from WICED_RESULT_LIST or BT_RESULT_LIST
*
*******************************************************************************/
wiced_result_t bt_app_management_cb(wiced_bt_management_evt_t event,
                                   wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    wiced_bt_device_address_t local_bda = {INIT_VALUE_ZERO};
    printf("Bluetooth app management callback: 0x%x\r\n", event);

    switch (event)
    {
        case BTM_ENABLED_EVT:

            /* Bluetooth Controller and Host Stack Enabled */
            if (WICED_BT_SUCCESS == p_event_data->enabled.status)
            {
                wiced_bt_set_local_bdaddr(cy_bt_device_address, BLE_ADDR_PUBLIC);
                wiced_bt_dev_read_local_addr(local_bda);
                printf("Bluetooth local device address: ");
                bt_print_bd_address(local_bda);

                /* Perform application-specific initialization */
                bt_app_init();
            }
            else
            {
                printf("Bluetooth enable failed, status = %d \r\n",
                p_event_data->enabled.status);
            }

            break;

        case BTM_DISABLED_EVT:
            break;

        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:

            /* Advertisement State Changed */
            printf("Bluetooth advertisement state change: 0x%x\r\n",
                                     p_event_data->ble_advert_state_changed);

            if(BTM_BLE_ADVERT_OFF == p_event_data->ble_advert_state_changed)
            {
                board_led2_set_state(SET_DUTY_CYCLE_100);
            }

            if(BTM_BLE_ADVERT_UNDIRECTED_HIGH == p_event_data
                    ->ble_advert_state_changed)
            {
                board_led2_set_state(SET_DUTY_CYCLE_50);
            }

            if(BTM_BLE_ADVERT_UNDIRECTED_LOW == p_event_data
                    ->ble_advert_state_changed)
            {
                board_led2_set_state(SET_DUTY_CYCLE_50);
            }

            break;

        case BTM_BLE_CONNECTION_PARAM_UPDATE:
            printf("Bluetooth connection parameter update status:%d\r\n"
                   "parameter interval: %d ms\r\n"
                   "parameter latency: %d ms\r\n"
                   "parameter timeout: %d ms\r\n",
                   p_event_data->ble_connection_param_update.status,
                   p_event_data->ble_connection_param_update.conn_interval,
                   p_event_data->ble_connection_param_update.conn_latency,
                   p_event_data->ble_connection_param_update.supervision_timeout);
            result = WICED_SUCCESS;
            break;

        case BTM_BLE_PHY_UPDATE_EVT:
            /* Print the updated BLE physical link*/
            printf("Bluetooth phy update selected TX - %dM\r\n"
                   "Bluetooth phy update selected RX - %dM\r\n",
                   p_event_data->ble_phy_update_event.tx_phy,
                   p_event_data->ble_phy_update_event.rx_phy);
            break;

        case BTM_PIN_REQUEST_EVT:
        case BTM_PASSKEY_REQUEST_EVT:
            result = WICED_BT_ERROR;
            break;

        default:
            break;
    }

    return result;
}

/*******************************************************************************
* Function Name: bt_app_gatt_event_cb
********************************************************************************
* Summary:
*  This function handles GATT events from the BT stack.
*
* Parameters:
*  wiced_bt_gatt_evt_t event                : LE GATT event code of one byte length
*  wiced_bt_gatt_event_data_t *p_event_data : Pointer to LE GATT event structures
*
* Return:
*  wiced_bt_gatt_status_t: Status codes in wiced_bt_gatt_status_e
*
*******************************************************************************/
wiced_bt_gatt_status_t bt_app_gatt_event_cb(wiced_bt_gatt_evt_t event,
                                        wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;

    wiced_bt_gatt_attribute_request_t *p_attr_req = &p_event_data
            ->attribute_request;

    /* Call the appropriate callback function based on the GATT event type, and 
     * pass the relevant event parameters to the callback function */
    switch (event)
    {
        case GATT_CONNECTION_STATUS_EVT:
            status = bt_app_gatt_conn_status_cb(&p_event_data
                    ->connection_status );
            if(WICED_BT_GATT_SUCCESS != status)
            {
               printf("GATT connection status failed: 0x%x\r\n", status);
            }

            break;

        case GATT_ATTRIBUTE_REQUEST_EVT:
            status = bt_app_gatt_req_cb(p_attr_req);
            break;

        case GATT_GET_RESPONSE_BUFFER_EVT:
            p_event_data->buffer_request.buffer.p_app_rsp_buffer =
                bt_app_alloc_buffer(p_event_data->buffer_request.len_requested);
            p_event_data->buffer_request.buffer.p_app_ctxt =
                    (void *)bt_app_free_buffer;
            status = WICED_BT_GATT_SUCCESS;
            break;

            /* GATT buffer transmitted event,
             * check \ref wiced_bt_gatt_buffer_transmitted_t*/
        case GATT_APP_BUFFER_TRANSMITTED_EVT:
        {
            pfn_free_buffer_t pfn_free =
                (pfn_free_buffer_t)p_event_data->buffer_xmitted.p_app_ctxt;

            /* If the buffer is dynamic, the context will point to a function
             * to free it. */
            if (pfn_free)
            {
                pfn_free(p_event_data->buffer_xmitted.p_app_data);
                status = WICED_BT_GATT_SUCCESS;
            }
            
        }
            break;

        default:
            break;
    }

    return status;
}

/*******************************************************************************
* Function Name: bt_app_gatt_req_cb
********************************************************************************
* Summary:
*   This function handles GATT server events from the BT stack.
*
* Parameters:
*  wiced_bt_gatt_attribute_request_t p_attr_req : Pointer to GATT connection
*                                                 status
*
* Return:
*  wiced_bt_gatt_status_t: Status codes in wiced_bt_gatt_status_e
*
*******************************************************************************/
wiced_bt_gatt_status_t bt_app_gatt_req_cb
               (wiced_bt_gatt_attribute_request_t *p_attr_req)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;
    switch ( p_attr_req->opcode )
    {
        case GATT_REQ_READ:
        case GATT_REQ_READ_BLOB:
            /* Attribute read request */
            status = bt_app_gatt_req_read_handler(p_attr_req->conn_id,
                                                  p_attr_req->opcode,
                                                  &p_attr_req->data.read_req,
                                                  p_attr_req->len_requested);
            break;

        case GATT_REQ_READ_BY_TYPE:
            status = bt_app_gatt_req_read_by_type_handler(p_attr_req->conn_id,
                   p_attr_req->opcode, &p_attr_req->data.read_by_type,
                   p_attr_req->len_requested);
            break;

        case GATT_REQ_READ_MULTI:
            break;

        case GATT_REQ_MTU:
            status = wiced_bt_gatt_server_send_mtu_rsp(p_attr_req->conn_id,
                                          p_attr_req->data.remote_mtu,
            CY_BT_RX_PDU_SIZE);
            break;

        case GATT_REQ_WRITE:
        case GATT_CMD_WRITE:
            /* Attribute write request */
            status = bt_app_gatt_req_write_handler(p_attr_req->conn_id,
                                                   p_attr_req->opcode,
                                                   &p_attr_req->data.write_req,
                                                   p_attr_req->len_requested);

            if ((GATT_REQ_WRITE == p_attr_req->opcode) &&
            (WICED_BT_GATT_SUCCESS == status ))
            {
            wiced_bt_gatt_write_req_t *p_write_request = &p_attr_req
                    ->data.write_req;
            wiced_bt_gatt_server_send_write_rsp(p_attr_req->conn_id,
                                                p_attr_req->opcode,
                                                p_write_request->handle);
            }
            break;
        case GATT_HANDLE_VALUE_CONF:
        case GATT_HANDLE_VALUE_NOTIF:
            break;

        case GATT_HANDLE_VALUE_IND:
            printf("bt_app_gatt:ind\r\n");
            break;
        default:
            printf("bt_app_gatt: unhandled GATT request: %d\r\n",
                    p_attr_req->opcode);
            break;
    }

    return status;
}

/*******************************************************************************
* Function Name : bt_app_gatt_req_read_by_type_handler
* ******************************************************************************
* Summary :
*  Process read-by-type request from peer device
*
* Parameters:
*  uint16_t                      conn_id       : Connection ID
*  wiced_bt_gatt_opcode_t        opcode        : LE GATT request type opcode
*  wiced_bt_gatt_read_by_type_t  p_read_req    : Pointer to read request
*                                                containing the handle to read
*  uint16_t                      len_req        : Length of data requested
*
* Return:
*  wiced_bt_gatt_status_t  : LE GATT status
*******************************************************************************/
wiced_bt_gatt_status_t bt_app_gatt_req_read_by_type_handler(uint16_t conn_id,
        wiced_bt_gatt_opcode_t opcode, wiced_bt_gatt_read_by_type_t *p_read_req,
        uint16_t len_req)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    gatt_db_lookup_table_t *puAttribute;
    uint16_t last_handle = INIT_VALUE_ZERO;
    uint16_t attr_handle = p_read_req->s_handle;
    uint8_t *p_rsp = bt_app_alloc_buffer(len_req);
    uint8_t pair_len = INIT_VALUE_ZERO;
    int used_len = INIT_VALUE_ZERO;

    if (NULL == p_rsp)
    {
        printf("bt_app_gatt:no memory found, len_req: %d!!\r\n",len_req);
        status = WICED_BT_GATT_INSUF_RESOURCE;
    }
    else
    {
        /* Read by type returns all attributes of the specified type,
         * between the start and end handles */
        while (WICED_TRUE)
        {
            last_handle = attr_handle;
            attr_handle = wiced_bt_gatt_find_handle_by_type(attr_handle,
                        p_read_req->e_handle, &p_read_req->uuid);
            if (INIT_VALUE_ZERO == attr_handle)
                break;


            if ( NULL == (puAttribute = bt_app_find_by_handle(attr_handle)))
            {
                printf("bt_app_gatt:found type but no attribute for %d \r\n",
                        last_handle);
                status = WICED_BT_GATT_INVALID_HANDLE;
                break;
            }

            int filled = wiced_bt_gatt_put_read_by_type_rsp_in_stream
                    (p_rsp + used_len, len_req - used_len, &pair_len,
                     attr_handle, puAttribute->cur_len, puAttribute->p_data);

            if (FILLED_VALUE_ZERO == filled)
            {
                break;
            }
            used_len += filled;

            /* Increment starting handle for next search to one past current */
            attr_handle++;
        }

        if (FILLED_VALUE_ZERO == used_len)
        {
            printf("bt_app_gatt:attr not found start_handle: 0x%04x  "
                    "end_handle: 0x%04x type: 0x%04x\r\n",
                    p_read_req->s_handle,
                    p_read_req->e_handle,
                    p_read_req->uuid.uu.uuid16);
            status = WICED_BT_GATT_INVALID_HANDLE;
        }
        else
        {
            /* Send the response */
            status = wiced_bt_gatt_server_send_read_by_type_rsp(conn_id,
                                            opcode, pair_len, used_len,
                                            p_rsp, (void *)bt_app_free_buffer);
        }
    }

    if (p_rsp != NULL)
    {
        bt_app_free_buffer(p_rsp);
    }

    return status;
}


/*******************************************************************************
* Function Name: bt_app_gatt_req_write_value
********************************************************************************
* Summary:
* This function handles writing to the attribute handle in the GATT database
* using the data passed from the BT stack. The value to write is stored in a
* buffer whose starting address is passed as one of the function parameters
*
* Parameters:
*  uint16_t attr_handle      : GATT attribute handle
*  uint8_t p_val            : Pointer to BLE GATT write request value
*  uint16_t len              : length of GATT write request
* Return:
*  wiced_bt_gatt_status_t: Status codes in wiced_bt_gatt_status_e
*
*******************************************************************************/
wiced_bt_gatt_status_t bt_app_gatt_req_write_value(uint16_t attr_handle,
                                                    uint8_t *p_val, uint16_t len)
{
    wiced_bt_gatt_status_t gatt_status  = WICED_BT_GATT_INVALID_HANDLE;
    wiced_bool_t isHandleInTable = WICED_FALSE;
    wiced_bool_t validLen = WICED_FALSE;
    printf("APP_BT_REQ_WRITE_VALUE\n");

    /* Check for a matching handle entry */
    for (int i = 0; i < app_gatt_db_ext_attr_tbl_size; i++)
    {

        if (app_gatt_db_ext_attr_tbl[i].handle == attr_handle)
        {
            /* Detected a matching handle in external lookup table */
            isHandleInTable = WICED_TRUE;

            /* Check if the buffer has space to store the data */
            validLen = (app_gatt_db_ext_attr_tbl[i].max_len >= len);

            if (validLen)
            {
                /* Value fits within the supplied buffer; copy over the value */
                app_gatt_db_ext_attr_tbl[i].cur_len = len;
                memcpy(app_gatt_db_ext_attr_tbl[i].p_data, p_val, len);
                gatt_status = WICED_BT_GATT_SUCCESS;

                /* Add code for any action required when this attribute is written.
                 * In this case, we Initialize the characteristic value */

                switch ( attr_handle )
                {
                    case HDLD_CAPSENSE_BUTTON_CLIENT_CHAR_CONFIG:
                        if (CAPSENSE_BUTTON_CLIENT_CHAR_CONFIG_LEN != len)
                        {
                            gatt_status = WICED_BT_GATT_INVALID_ATTR_LEN;
                        }
                        else
                        {
                            if(GATT_CLIENT_CONFIG_NOTIFICATION ==
                                    app_capsense_button_client_char_config
                                    [NOTIFICATION_ENABLED])
                            {
                                capsense_data.buttoncount= NO_OF_CAPSENSE_BUTTONS;
                                bt_app_send_notification();
                            }
                        }
                        break;

                    case HDLD_CAPSENSE_SLIDER_CLIENT_CHAR_CONFIG:
                        if (CAPSENSE_SLIDER_CLIENT_CHAR_CONFIG_LEN != len)
                        {
                            gatt_status = WICED_BT_GATT_INVALID_ATTR_LEN;
                        }
                        else
                        {
                            if(GATT_CLIENT_CONFIG_NOTIFICATION ==
                                    app_capsense_slider_client_char_config
                                    [NOTIFICATION_ENABLED])
                            {
                                bt_app_send_notification();
                            }
                        }
                        break;

                    default:
                        break;
                }
            }
            else
            {
                /* Value to write does not meet size constraints */
                gatt_status = WICED_BT_GATT_INVALID_HANDLE;
                printf("GATT write request to invalid handle: 0x%x\r\n",
                        attr_handle);
            }

            break;
        }
    }
    if (!isHandleInTable)
    {
        switch(attr_handle)
        {
            default:
                /* The write operation was not performed for the
                 * indicated handle */
                gatt_status = WICED_BT_GATT_WRITE_NOT_PERMIT;
                printf("GATT write request to invalid handle: 0x%x\n",
                        attr_handle);
                break;
        }
    }

    return gatt_status;
}


/*******************************************************************************
* Function Name: bt_app_gatt_req_write_handler
********************************************************************************
* Summary:
*   This function handles Write Requests received from the client device
*
* Parameters:
*  uint16_t conn_id       : Connection ID
*  wiced_bt_gatt_opcode_t opcode        : LE GATT request type opcode
*  wiced_bt_gatt_write_req_t p_write_req   : Pointer to LE GATT write request
*  uint16_t len_req       : length of data requested
*
* Return:
*  wiced_bt_gatt_status_t: Status codes in wiced_bt_gatt_status_e
*
*******************************************************************************/
wiced_bt_gatt_status_t bt_app_gatt_req_write_handler(uint16_t conn_id,
                wiced_bt_gatt_opcode_t opcode,
                wiced_bt_gatt_write_req_t *p_write_req,
                uint16_t len_req)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_INVALID_HANDLE;

    printf("bt_app_gatt_write_handler: conn_id:%d handle:0x%x offset:%d "
            "len:%d\r\n",
            conn_id, 
            p_write_req->handle, 
            p_write_req->offset, 
            p_write_req->val_len );

    /* Attempt to perform the Write Request */
    status = bt_app_gatt_req_write_value(p_write_req->handle,
                                         p_write_req->p_val,
                                         p_write_req->val_len);

    if(WICED_BT_GATT_SUCCESS != status)
    {
        printf("bt_app_gatt:GATT set attr status : 0x%x\n", status);
    }

    return (status);
}


/*******************************************************************************
* Function Name: bt_app_gatt_req_read_handler
********************************************************************************
* Summary:
*   This function handles Read Requests received from the client device
*
* Parameters:
* conn_id       : Connection ID
* opcode        : LE GATT request type opcode
* p_read_req    : Pointer to read request containing the handle to read
* len_req       : length of data requested
*
* Return:
*  wiced_bt_gatt_status_t: Status codes in wiced_bt_gatt_status_e
*
*******************************************************************************/
wiced_bt_gatt_status_t bt_app_gatt_req_read_handler( uint16_t conn_id,
                wiced_bt_gatt_opcode_t opcode,
                wiced_bt_gatt_read_t *p_read_req,
                uint16_t len_req)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    gatt_db_lookup_table_t  *puAttribute;
    int attr_len_to_copy;
    uint8_t *from;
    int  to_send;

    printf("APP_BT_REQ_READ_HANDLER\n");
    puAttribute = bt_app_find_by_handle(p_read_req->handle);

    if (NULL == puAttribute)
    {
        wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, p_read_req->handle,
                                            WICED_BT_GATT_INVALID_HANDLE);
        status = WICED_BT_GATT_INVALID_HANDLE;
    }
    else
    {
        attr_len_to_copy = puAttribute->cur_len;
        printf("bt_app_gatt_read_handler: conn_id:%d handle:0x%x offset:%d "
                "len:%d\r\n",
                conn_id, p_read_req->handle,
                p_read_req->offset,
                attr_len_to_copy);
        if (p_read_req->offset >= puAttribute->cur_len)
        {
            wiced_bt_gatt_server_send_error_rsp(conn_id, opcode, p_read_req->handle,
                                                WICED_BT_GATT_INVALID_OFFSET);
            status = WICED_BT_GATT_INVALID_OFFSET;
        }
        else
        {
            to_send = MIN(len_req, attr_len_to_copy - p_read_req->offset);
            from = ((uint8_t *)puAttribute->p_data) + p_read_req->offset;

            switch ( p_read_req->handle )
            {
                case HDLC_CAPSENSE_BUTTON_VALUE:
                    /* CapSense buttons data to be read */
                    app_capsense_button[BUTTON_COUNT] =
                            capsense_data.buttoncount;
                    app_capsense_button[BUTTON_STATUS] =
                            capsense_data.buttonstatus1;
                    break;

                case HDLC_CAPSENSE_SLIDER_VALUE:
                    /* CapSense slider data to be read */
                    app_capsense_slider[SLIDER_DATA] = capsense_data.sliderdata;
                    break;

                default:
                    break;
            }
            status = wiced_bt_gatt_server_send_read_handle_rsp(conn_id, opcode,
                    to_send, from, NULL);
        }
    }

    return status;
}


/*******************************************************************************
* Function Name: bt_app_gatt_conn_status_cb
********************************************************************************
* Summary:
*   This callback function handles connection status changes.
*
* Parameters:
*   wiced_bt_gatt_connection_status_t *p_conn_status  : Pointer to data that 
*                                                       has connection details
*
* Return:
*  wiced_bt_gatt_status_t: Status codes in wiced_bt_gatt_status_e
*
*******************************************************************************/
wiced_bt_gatt_status_t bt_app_gatt_conn_status_cb
             (wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;
    wiced_result_t result = WICED_BT_ERROR;

    if ( NULL != p_conn_status )
    {
        if (p_conn_status->connected)
        {
            /* Device has connected */
            printf("Bluetooth connected with device address:" );
            bt_print_bd_address(p_conn_status->bd_addr);
            printf("Bluetooth device connection id: 0x%x\r\n",
                    p_conn_status->conn_id);

            /* Store the connection ID */
            bt_connection_id = p_conn_status->conn_id;
        }
        else
        {
            /* Device has disconnected */
            printf("Bluetooth disconnected with device address:" );
            bt_print_bd_address(p_conn_status->bd_addr);
            printf("Bluetooth device connection id: 0x%x\r\n",
                    p_conn_status->conn_id);

            /* Set the connection id to zero to indicate disconnected state */
            bt_connection_id = INIT_VALUE_ZERO;

            /* Restart the advertisements */
            result = wiced_bt_start_advertisements
            (BTM_BLE_ADVERT_UNDIRECTED_HIGH,
            BLE_ADDR_PUBLIC, NULL);

            /* Failed to start advertisement. Stop program execution */
            if (CY_RSLT_SUCCESS != result)
            {
                handle_app_error();
            }
        }
        status = WICED_BT_GATT_SUCCESS;
    }

    return status;
}

/*******************************************************************************
* Function Name: bt_app_free_buffer
********************************************************************************
* Summary:
*  This function frees up the memory buffer
*
* Parameters:
*  uint8_t *p_data: Pointer to the buffer to be free
*
* Return:
*  None
*
*******************************************************************************/
void bt_app_free_buffer(uint8_t *p_buf)
{
    vPortFree(p_buf);
}

/*******************************************************************************
* Function Name: bt_app_alloc_buffer
********************************************************************************
* Summary:
*  This function allocates a memory buffer.
*
*
* Parameters:
*  int len: Length to allocate
*
* Return:
*  None
*
*******************************************************************************/
void* bt_app_alloc_buffer(int len)
{
    return pvPortMalloc(len);
}

/*******************************************************************************
* Function Name : bt_app_find_by_handle
* ******************************************************************************
* Summary :
*  Find attribute description by handle
*
* Parameters:
*  uint16_t handle    handle to look up
*
* Return:
*  gatt_db_lookup_table_t   pointer containing handle data
*
*******************************************************************************/
gatt_db_lookup_table_t  *bt_app_find_by_handle(uint16_t handle)
{

    gatt_db_lookup_table_t  *result = NULL;

    for (uint8_t i = INIT_VALUE_ZERO; i < app_gatt_db_ext_attr_tbl_size; i++)
    {
        if (handle == app_gatt_db_ext_attr_tbl[i].handle)
        {
            result = &app_gatt_db_ext_attr_tbl[i];
            break;
        }
    }

    return result;
}

/*******************************************************************************
* Function Name: bt_app_send_notification
********************************************************************************
* Summary: Sends GATT notification.
*******************************************************************************/
void bt_app_send_notification(void)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_ERROR;

    if((GATT_CLIENT_CONFIG_NOTIFICATION == \
    app_capsense_button_client_char_config[NOTIFICATION_ENABLED])
    && (INIT_VALUE_ZERO != bt_connection_id))
    {
        /* CapSense button data to be send*/
        app_capsense_button[BUTTON_COUNT] = capsense_data.buttoncount;
        app_capsense_button[BUTTON_STATUS] = capsense_data.buttonstatus1;
        status = wiced_bt_gatt_server_send_notification(bt_connection_id,
                   HDLC_CAPSENSE_BUTTON_VALUE,
                   app_gatt_db_ext_attr_tbl[APP_CAPSENSE_BUTTON].cur_len,
                   app_gatt_db_ext_attr_tbl[APP_CAPSENSE_BUTTON].p_data,NULL);
        if(WICED_BT_GATT_SUCCESS != status)
        {
            printf("Sending CapSense button notification failed\r\n");
        }

    }

    if((GATT_CLIENT_CONFIG_NOTIFICATION == \
    app_capsense_slider_client_char_config[NOTIFICATION_ENABLED])
    && (INIT_VALUE_ZERO != bt_connection_id))
    {
        /* CapSense slider data to be send */
        app_capsense_slider[0] = capsense_data.sliderdata;
        status = wiced_bt_gatt_server_send_notification( bt_connection_id,
                    HDLC_CAPSENSE_SLIDER_VALUE,
                    app_gatt_db_ext_attr_tbl[APP_CAPSENSE_SLIDER].cur_len,
                    app_gatt_db_ext_attr_tbl[APP_CAPSENSE_SLIDER].p_data,NULL);
        if(WICED_BT_GATT_SUCCESS != status)
        {
            printf("Sending CapSense slider notification failed\r\n");
        }
    }
}

/*******************************************************************************
* Function Name: bt_print_bd_address
********************************************************************************
* Summary: This is the utility function that prints the address of the 
*          Bluetooth device
*
* Parameters:
*  wiced_bt_device_address_t bdaddr : Bluetooth address
*
* Return:
*  None
*
*******************************************************************************/
void bt_print_bd_address(wiced_bt_device_address_t bdadr)
{
    for(uint8_t i=INIT_VALUE_ZERO;i<BD_ADDR_LEN-1;i++)
    {
        printf("%02X:",bdadr[i]);
    }

    printf("%02X\n",bdadr[BD_ADDR_LEN-1]);
}


/* END OF FILE [] */
