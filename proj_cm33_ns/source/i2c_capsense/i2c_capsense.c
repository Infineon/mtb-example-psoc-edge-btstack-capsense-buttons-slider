/*******************************************************************************
* File Name: capsense.c
*
* Description: This file contains the i2c_capsense task which initializes the I2C
* peripheral and receives capsense data from the PSoC 4000T Capsense chip.
*
* Related Document: README.md
*
********************************************************************************
* Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
* Header files includes
*******************************************************************************/
#include "i2c_capsense.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "board.h"
#include "bt_app.h"
#include "cybsp.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define INTERGER_ASCII_DIFFERENCE    (30U)
#define CAPSENSE_READ_BUFFER_SIZE    (3U)
#define I2C_SLAVE_ADDRESS            (0x8)
#define I2C_SEND_RECEIVE_TIMEOUT_MS  (0U)
#define BUTTON0_INDEX                (0U)
#define BUTTON1_INDEX                (1U)
#define SLIDER_INDEX                 (2U)
#define I2C_READ_SIZE_MIN            (0U)
#define I2C_READ_SIZE_MAX            (1U)
#define CAPSENSE_BTN0_NOT_PRESSED    (0U)
#define CAPSENSE_BTN1_NOT_PRESSED    (1U)
#define CAPSENSE_BTN0_PRESSED        (1U)
#define CAPSENSE_BTN1_PRESSED        (2U)
#define XTASK_NOTIFY_UL_VALUE        (1U)
#define XQUEUE_TICKS_TO_WAIT         (0U)
#define SLIDER_POS_NOT_CHANGED       (0U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
QueueHandle_t capsense_command_q;

/* Variables used for storing buttons and slider data */
capsense_data_t capsense_data = {0};

uint8_t buffer[CAPSENSE_READ_BUFFER_SIZE];
static uint32_t button0_status;
static uint32_t button1_status;
static uint16_t slider_pos;
static uint32_t button0_status_prev;
static uint32_t button1_status_prev;
static uint16_t slider_pos_prev;
static uint8_t size;
static uint8_t *data;

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: capsense_task
********************************************************************************
* Summary:
*  Task that initializes the I2C peripheral and receives Capsense data
*  from PSoC 4000T Capsense chip. Then sends commands to change LED state and to
*  send BLE notifications.
*
* Parameters:
*  void *param : Task parameter defined during task creation (unused)
*
* Return:
*  None
*
*******************************************************************************/
void i2c_capsense_task(void* param)
{
    cy_en_scb_i2c_status_t initStatus;
    cy_stc_scb_i2c_context_t CYBSP_I2C_CONTROLLER_context;

    /* Initialize and enable the I2C in master mode. */
    initStatus = Cy_SCB_I2C_Init(CYBSP_I2C_CONTROLLER_HW,
            &CYBSP_I2C_CONTROLLER_config, &CYBSP_I2C_CONTROLLER_context);

    if(initStatus != CY_SCB_I2C_SUCCESS)
    {
        CY_ASSERT(0u);
    }

    /* Enable I2C master hardware. */
    Cy_SCB_I2C_Enable(CYBSP_I2C_CONTROLLER_HW);

    for(;;)
    {
        /* Variables used for storing command and data for LED Task */
        led_command_data_t led_cmd_data;
        bool send_led_command = false;
        bool send_bt_command = false;

        cy_en_scb_i2c_command_t ack = CY_SCB_I2C_ACK;
        size =CAPSENSE_READ_BUFFER_SIZE;
        data = &buffer[BUTTON0_INDEX];

        /* Start transaction, send dev_addr */
        cy_en_scb_i2c_status_t status = CYBSP_I2C_CONTROLLER_context.state ==
                CY_SCB_I2C_IDLE ? Cy_SCB_I2C_MasterSendStart(CYBSP_I2C_CONTROLLER_HW,
                I2C_SLAVE_ADDRESS, CY_SCB_I2C_READ_XFER, I2C_SEND_RECEIVE_TIMEOUT_MS,
                &CYBSP_I2C_CONTROLLER_context):
        Cy_SCB_I2C_MasterSendReStart(CYBSP_I2C_CONTROLLER_HW,
                I2C_SLAVE_ADDRESS, CY_SCB_I2C_READ_XFER,
                I2C_SEND_RECEIVE_TIMEOUT_MS, &CYBSP_I2C_CONTROLLER_context);

        if (CY_SCB_I2C_SUCCESS == status)
        {
            while (size > I2C_READ_SIZE_MIN) 
            {
                if (size == I2C_READ_SIZE_MAX)
                {
                    ack = CY_SCB_I2C_NAK;
                }
                status = Cy_SCB_I2C_MasterReadByte(CYBSP_I2C_CONTROLLER_HW, ack,
                        (uint8_t *) data,I2C_SEND_RECEIVE_TIMEOUT_MS,
                        &CYBSP_I2C_CONTROLLER_context);
                if (status != CY_SCB_I2C_SUCCESS)
                {
                    break;
                }
                ++data;
                --size;
            }
        }
        /* SCB in I2C mode is very time sensitive. In practice we
         * have to request STOP after each block, otherwise it may break
         * the transmission 
         */
        Cy_SCB_I2C_MasterSendStop(CYBSP_I2C_CONTROLLER_HW,
                I2C_SEND_RECEIVE_TIMEOUT_MS, &CYBSP_I2C_CONTROLLER_context);

       /* Subtract the received ASCII value with the integer
        * ASCII difference to obtain the integer value
        */
        buffer[BUTTON0_INDEX]-=INTERGER_ASCII_DIFFERENCE;
        buffer[BUTTON1_INDEX]-=INTERGER_ASCII_DIFFERENCE;

        /* Copy the value in the received buffer to the status variable */
        button0_status = buffer[BUTTON0_INDEX];
        button1_status = buffer[BUTTON1_INDEX];
        slider_pos = buffer[SLIDER_INDEX];

        /* Detect new touch on Button0 */
        if ((CAPSENSE_BTN0_NOT_PRESSED != button0_status)
                &&(CAPSENSE_BTN0_NOT_PRESSED == button0_status_prev))
        {
            led_cmd_data.command = LED_TURN_ON;
            send_led_command = true;
            capsense_data.buttonstatus1 = CAPSENSE_BTN0_PRESSED;
            send_bt_command = true;
        }

        /* Detect new touch on Button1 */
        if ((CAPSENSE_BTN1_NOT_PRESSED != button1_status) &&
                (CAPSENSE_BTN1_NOT_PRESSED == button1_status_prev))
        {
            led_cmd_data.command = LED_TURN_OFF;
            send_led_command = true;
            capsense_data.buttonstatus1 = CAPSENSE_BTN1_PRESSED;
            send_bt_command = true;
        }

        /* Detect the new touch on slider */
        if ((SLIDER_POS_NOT_CHANGED != slider_pos) && (slider_pos
                != slider_pos_prev))
        {
            led_cmd_data.command = LED_SET_BRIGHTNESS;

            /* Setting brightness value */
            led_cmd_data.brightness = slider_pos;
            send_led_command = true;

            /* Setting ble app data value */
            capsense_data.sliderdata = slider_pos;
            send_bt_command = true;
        }

        /* Send command to update LED state */
        if(send_led_command)
        {
            xQueueSendToBack(led_command_data_q, &led_cmd_data,
                    XQUEUE_TICKS_TO_WAIT);
        }

        if(send_bt_command)
        {
            xTaskNotify(bt_task_handle, XTASK_NOTIFY_UL_VALUE,
                    eSetValueWithoutOverwrite);
        }

        /* Update previous touch status */
        button0_status_prev = button0_status;
        button1_status_prev = button1_status;
        slider_pos_prev = slider_pos;
    }
}


/* END OF FILE [] */
