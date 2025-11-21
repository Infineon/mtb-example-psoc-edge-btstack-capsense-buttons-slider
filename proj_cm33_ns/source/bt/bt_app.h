/*******************************************************************************
* File Name: bt_app.h
*
* Description: This file is the public interface of bt_app.c source file
*
* Related Document: README.md
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
* Include guard
*******************************************************************************/
#ifndef BT_APP_H
#define BT_APP_H

/*******************************************************************************
* Header file includes
*******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "wiced_bt_uuid.h"
#include "wiced_result.h"
#include "wiced_bt_stack.h"
#include "wiced_memory.h"
#include "cybt_platform_config.h"
#include "cycfg_gatt_db.h"

/*******************************************************************************
* Global Constants
*******************************************************************************/
/* Notification parameters */
enum
{
    NOTIFIY_OFF,
    NOTIFIY_ON,
};

/*******************************************************************************
* Extern Variables
*******************************************************************************/
extern TaskHandle_t  bt_task_handle;
extern volatile uint16_t bt_connection_id;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void bt_task(void* param);
void bt_app_send_notification(void);
void bt_app_send_indication(void);;
void  bt_app_init(void);
void* bt_app_alloc_buffer(int len);
void  bt_app_free_buffer(uint8_t *p_event_data);
void  bt_print_bd_address(wiced_bt_device_address_t bdadr);
gatt_db_lookup_table_t *bt_app_find_by_handle(uint16_t handle);
wiced_result_t bt_app_management_cb(wiced_bt_management_evt_t event,
                                    wiced_bt_management_evt_data_t *p_event_data);
wiced_bt_gatt_status_t bt_app_gatt_event_cb(wiced_bt_gatt_evt_t event,
                                wiced_bt_gatt_event_data_t *p_data);
wiced_bt_gatt_status_t bt_app_gatt_conn_status_cb
(wiced_bt_gatt_connection_status_t *p_conn_status);
wiced_bt_gatt_status_t bt_app_gatt_req_cb
                                (wiced_bt_gatt_attribute_request_t *p_attr_req);
wiced_bt_gatt_status_t bt_app_gatt_req_write_value(uint16_t attr_handle,
                                                uint8_t *p_val, uint16_t len);
wiced_bt_gatt_status_t bt_app_gatt_req_write_handler(uint16_t conn_id,
                                       wiced_bt_gatt_opcode_t opcode,
                                       wiced_bt_gatt_write_req_t *p_write_req,
                                       uint16_t len_req);
wiced_bt_gatt_status_t bt_app_gatt_req_read_handler(uint16_t conn_id,
                                       wiced_bt_gatt_opcode_t opcode,
                                       wiced_bt_gatt_read_t *p_read_req,
                                       uint16_t len_req);
wiced_bt_gatt_status_t bt_app_gatt_req_read_by_type_handler (uint16_t conn_id,
                                       wiced_bt_gatt_opcode_t opcode,
                                       wiced_bt_gatt_read_by_type_t *p_read_req,
                                       uint16_t len_requested);

#endif /* BT_APP_H */
