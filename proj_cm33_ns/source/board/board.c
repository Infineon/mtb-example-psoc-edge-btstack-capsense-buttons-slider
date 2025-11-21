/*******************************************************************************
* File Name: board.c
*
* Description: This file contains board supported API's.
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
#include "i2c_capsense.h"
#include "cybsp.h"
#include "board.h"
#include "retarget_io_init.h"

/*******************************************************************************
* Macros
********************************************************************************/
#define PWM_DUTY_CYCLE_0          (0U)
#define PWM_DUTY_CYCLE_100        (100U)
#define BOARD_TASK_PRIORITY       (configMAX_PRIORITIES - 1U)
#define BOARD_TASK_STACK_SIZE     (256U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* FreeRTOS task handle for board task. Button task is used to handle button
 * events */
TaskHandle_t  board_task_handle;

/* Queue handle used for LED data */
QueueHandle_t led_command_data_q;

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)

/* PWM1 Context for SysPm Callback */
mtb_syspm_tcpwm_deepsleep_context_t PWM1dscontext =
{
    .channelNum = PWM1_NUM,
};

/* PWM2 Context for SysPm Callback */
mtb_syspm_tcpwm_deepsleep_context_t PWM2dscontext =
{
    .channelNum = PWM2_NUM,
};


/* SysPm callback parameter structure for PWM_1 */
static cy_stc_syspm_callback_params_t PWM1DSParams =
{
        .context   = &PWM1dscontext,
        .base      = PWM1_HW
};

/* SysPm callback parameter structure for PWM_2 */
static cy_stc_syspm_callback_params_t PWM2DSParams =
{
        .context   = &PWM2dscontext,
        .base      = PWM2_HW
};

/* SysPm callback structure for PWM_1 */
static cy_stc_syspm_callback_t PWM1DeepSleepCallbackHandler =
{
    .callback           = &mtb_syspm_tcpwm_deepsleep_callback,
    .skipMode           = CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_CHECK_READY,
    .type               = CY_SYSPM_DEEPSLEEP,
    .callbackParams     = &PWM1DSParams,
    .prevItm            = NULL,
    .nextItm            = NULL,
    .order              = SYSPM_CALLBACK_ORDER
};

/* SysPm callback structure for PWM_2 */
static cy_stc_syspm_callback_t PWM2DeepSleepCallbackHandler =
{
    .callback           = &mtb_syspm_tcpwm_deepsleep_callback,
    .skipMode           = CY_SYSPM_SKIP_CHECK_FAIL | CY_SYSPM_SKIP_CHECK_READY,
    .type               = CY_SYSPM_DEEPSLEEP,
    .callbackParams     = &PWM2DSParams,
    .prevItm            = NULL,
    .nextItm            = NULL,
    .order              = SYSPM_CALLBACK_ORDER
};

#endif /*CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP) */

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: board_led_init
********************************************************************************
* Summary:
*   Initialize the USERLED1 and USERLED2 with PWM (Pulse Width Modulation).
*******************************************************************************/
static void board_led_init(void)
{

    cy_rslt_t result = CY_TCPWM_SUCCESS;

    /* Initialize TCPWM for LED1 */
    result= Cy_TCPWM_PWM_Init(PWM1_HW, PWM1_NUM, &PWM1_config);

    if(CY_TCPWM_SUCCESS != result)
    {
        printf("Failed to initialize PWM for LED1.\r\n");
        handle_app_error();
    }
    
#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)

    /* SysPm callback registration for PWM1 */
    Cy_SysPm_RegisterCallback(&PWM1DeepSleepCallbackHandler);

#endif

    /* Enable the TCPWM block for LED1 */
    Cy_TCPWM_PWM_Enable(PWM1_HW, PWM1_NUM);

    /* Start the PWM for LED1 */
    Cy_TCPWM_TriggerStart_Single(PWM1_HW, PWM1_NUM);

    /* Initialize TCPWM for LED2 */
    result=Cy_TCPWM_PWM_Init(PWM2_HW, PWM2_NUM, &PWM2_config);

    if(CY_TCPWM_SUCCESS != result)
    {
        printf("Failed to initialize PWM for LED2.\r\n");
        handle_app_error();
    }
    
#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP)

    /* SysPm callback registration for PWM2 */
    Cy_SysPm_RegisterCallback(&PWM2DeepSleepCallbackHandler);

#endif

    /* Enable the TCPWM block for LED2 */
    Cy_TCPWM_PWM_Enable(PWM2_HW, PWM2_NUM);

    /* Start the PWM for LED2 */
    Cy_TCPWM_TriggerStart_Single(PWM2_HW, PWM2_NUM);
}

/*******************************************************************************
* Function Name: board_led1_set_brightness
********************************************************************************
* Summary:
*   Set the USER_LED1 brightness over PWM
*
* Parameters:
*   value: PWM duty cycle/compare 0 value
*
* Return:
*   None
*
*******************************************************************************/
static void board_led1_set_brightness(uint8_t value)
{
    /* Vary the USERLED1 brightness by varying the Compare0 value */
    Cy_TCPWM_PWM_SetCompare0(PWM1_HW, PWM1_NUM, value);
}

/*******************************************************************************
* Function Name: board_led1_set_state
********************************************************************************
* Summary:
*   Set the led state ON/OFF over PWM
*
* Parameters:
*   value: ON/OFF state
*
* Return:
*   None
*
*******************************************************************************/
static void board_led1_set_state( bool value)
{
    /* Turn ON or OFF the USERLED1 by varying the Compare0 value */
    Cy_TCPWM_PWM_SetCompare0(PWM1_HW, PWM1_NUM,
            value?PWM_DUTY_CYCLE_0:PWM_DUTY_CYCLE_100);
}

/*******************************************************************************
* Function Name: board_led2_set_state
********************************************************************************
* Summary:
*   Set the USERLED 2 state over PWM
*
* Parameters:
*   value: ON/OFF state
*
* Return:
*   None
*
*******************************************************************************/
void board_led2_set_state(uint32_t value)
{
    /* Set the state of USER_LED2 by varying the Compare0 value */
    Cy_TCPWM_PWM_SetCompare0(PWM2_HW, PWM2_NUM, value);
}

/*******************************************************************************
* Function Name: board_task
********************************************************************************
* Summary:
*   This task initialize the board and button events
*
* Parameters:
*   void *param: Not used
*
* Return:
*   None
*
*******************************************************************************/
void board_task(void *param)
{
    led_command_data_t led_cmd_data;

    /* Suppress warning for unused parameter */
    CY_UNUSED_PARAMETER(param);

    for(;;)
    {
        /* Block until a command has been received over queue */
        xQueueReceive(led_command_data_q, &led_cmd_data, portMAX_DELAY);

        switch(led_cmd_data.command)
        {
            case LED_TURN_ON:
                board_led1_set_state(LED_ON);
                break;
            case LED_TURN_OFF:
                board_led1_set_state(LED_OFF);
                break;
            case LED_SET_BRIGHTNESS:
                board_led1_set_brightness(led_cmd_data.brightness);
                break;
            default:
                break;
        }
    }
}

/*******************************************************************************
* Function Name: board_init
********************************************************************************
* Summary:
*   Initialize the board with LED's and Buttons
*
* Parameters:
*   None
*
* Return:
*   cy_rslt_t  Result status
*
*******************************************************************************/
cy_rslt_t board_init(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    BaseType_t rtos_result;

    /* Initialize the USERLED1 and USERLED2 with PWM (Pulse Width Modulation) */
    board_led_init();

    /* Create Board Task for processing board events */
    rtos_result = xTaskCreate(board_task,"Board Task", BOARD_TASK_STACK_SIZE,
                              NULL, BOARD_TASK_PRIORITY, &board_task_handle);
    if( pdPASS != rtos_result)
    {
        printf("Failed to create board task.\r\n");
        handle_app_error();
    }
    else
    {
        result = CY_RSLT_SUCCESS;
    }

    return result;
}


/* [] END OF FILE */