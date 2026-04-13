#include "oled_task.h"

#include "app_rtos_config.h"

#include "key_task.h"
#include "motor_task.h"
#include "nrf_task.h"
#include "stepper_task.h"

#include "OLED.h"
#include "dc_motor.h"

/*
 * OLED 任务模块说明
 * -----------------
 * - 管理 UI 模式与菜单状态。
 * - 通过按键任务 API 消费按键事件。
 * - 优先从显示快照队列渲染，必要时回退到互斥保护的共享状态。
 */

TaskHandle_t OLEDTaskHandle = NULL;
QueueHandle_t OLEDDataQueueHandle = NULL;
SemaphoreHandle_t AppDataMutexHandle = NULL;
volatile AppMode_t g_app_mode = APP_MODE_ALL_CONTROL;
volatile uint8_t g_menu_cursor = MENU_ITEM_ALL_CONTROL;
volatile uint8_t g_in_test = 1U;
static OLEDDisplayData_t g_oled_display_data;

/* 创建共享互斥锁、显示队列并启动 OLED 任务。 */
void OLED_Task_Init(void)
{
    if (AppDataMutexHandle == NULL)
    {
        AppDataMutexHandle = xSemaphoreCreateMutex();
    }

    if (OLEDDataQueueHandle == NULL)
    {
        OLEDDataQueueHandle = xQueueCreate(OLED_DATA_QUEUE_LENGTH, sizeof(OLEDDisplayData_t));
    }

    xTaskCreate(OLED_Task_Entry,
                "OLEDTask",
                OLED_TASK_STACK_SIZE,
                NULL,
                OLED_TASK_PRIORITY,
                &OLEDTaskHandle);
}

/* 通过按键模块对外 API 拉取一个按键事件。 */
static uint8_t App_FetchKeyEvent(void)
{
    uint8_t event;

    event = 0U;
    (void)Key_GetValue(&event, 0U);

    return event;
}

/* 根据光标进入对应测试模式。 */
static void App_EnterSelectedMode(void)
{
    g_in_test = 1U;

    switch ((MenuItem_t)g_menu_cursor)
    {
        case MENU_ITEM_MOTOR:
            g_app_mode = APP_MODE_MOTOR;
            break;
        case MENU_ITEM_STEPPER:
            g_app_mode = APP_MODE_STEPPER;
            break;
        case MENU_ITEM_NRF:
            g_app_mode = APP_MODE_NRF;
            break;
        case MENU_ITEM_ALL_CONTROL:
            g_app_mode = APP_MODE_ALL_CONTROL;
            break;
        default:
            g_app_mode = APP_MODE_IDLE;
            g_in_test = 0U;
            break;
    }
}

/* 处理单个按键事件并更新菜单/模式状态。 */
static void App_HandleMenuKey(uint8_t key_event)
{
    BaseType_t locked = pdFALSE;

    if (AppDataMutexHandle != NULL)
    {
        locked = xSemaphoreTake(AppDataMutexHandle, APP_MUTEX_TIMEOUT_TICKS);
    }

    if (g_in_test == 0U)
    {
        switch (key_event)
        {
            case 1U:
                g_menu_cursor = (g_menu_cursor == 0U) ?
                                (uint8_t)(MENU_ITEM_COUNT - 1U) :
                                (uint8_t)(g_menu_cursor - 1U);
                break;
            case 2U:
                g_menu_cursor++;
                if (g_menu_cursor >= (uint8_t)MENU_ITEM_COUNT)
                {
                    g_menu_cursor = 0U;
                }
                break;
            case 3U:
                App_EnterSelectedMode();
                break;
            default:
                break;
        }
    }
    else if (key_event == 4U)
    {
        g_in_test = 0U;
        g_app_mode = APP_MODE_IDLE;
    }

    if (locked == pdTRUE)
    {
        (void)xSemaphoreGive(AppDataMutexHandle);
    }
}

/* 读取共享状态并构建一份显示快照。 */
static OLEDDisplayData_t App_BuildDisplayDataFromShared(void)
{
    OLEDDisplayData_t data;
    BaseType_t locked = pdFALSE;

    if (AppDataMutexHandle != NULL)
    {
        locked = xSemaphoreTake(AppDataMutexHandle, APP_MUTEX_TIMEOUT_TICKS);
    }

    data.app_mode = g_app_mode;
    data.in_test = g_in_test;
    data.menu_cursor = g_menu_cursor;
    data.motor_status = g_MotorStatus;
    data.stepper_enable = g_stepper_enable;
    data.stepper_dir_l = g_stepper_dir_l;
    data.stepper_dir_r = g_stepper_dir_r;
    data.stepper_rpm_l = g_stepper_rpm_l;
    data.stepper_rpm_r = g_stepper_rpm_r;
    data.nrf_last_rx_flag = g_nrf_last_rx_flag;
    data.nrf_rx_ok_count = g_nrf_rx_ok_count;
    data.nrf_rx_err_count = g_nrf_rx_err_count;
    data.nrf_last_payload[0] = g_nrf_last_payload[0];
    data.nrf_last_payload[1] = g_nrf_last_payload[1];
    data.nrf_last_payload[2] = g_nrf_last_payload[2];
    data.nrf_last_payload[3] = g_nrf_last_payload[3];
    data.js_left = g_js_left;
    data.js_right = g_js_right;
    data.steer_target_deg = g_steer_target_deg;
    data.steer_current_deg = g_steer_current_deg;

    if (locked == pdTRUE)
    {
        (void)xSemaphoreGive(AppDataMutexHandle);
    }

    return data;
}

/* 从共享状态同步一份显示快照到模块内缓存。 */
static void App_SyncDisplayDataFromShared(void)
{
    g_oled_display_data = App_BuildDisplayDataFromShared();
}

/* 面向生产者的接口：把最新快照覆盖写入 OLED 队列。 */
void OLED_PublishDisplayData(void)
{
    OLEDDisplayData_t publish_data;

    if (OLEDDataQueueHandle == NULL)
    {
        return;
    }

    publish_data = App_BuildDisplayDataFromShared();
    (void)xQueueOverwrite(OLEDDataQueueHandle, &publish_data);
}

/* 面向读取方的接口：窥视最新快照且不消费队列元素。 */
bool OLED_GetValue(OLEDDisplayData_t *out_data, uint32_t timeout_ms)
{
    if ((out_data == NULL) || (OLEDDataQueueHandle == NULL))
    {
        return false;
    }

    if (xQueuePeek(OLEDDataQueueHandle, out_data, pdMS_TO_TICKS(timeout_ms)) == pdPASS)
    {
        return true;
    }

    return false;
}

/* 根据模块内快照绘制主菜单页面。 */
static void App_DrawMainMenu(void)
{
    OLED_ShowString(24, 0, "Test Menu", OLED_6X8);
    OLED_ShowString(0, 12, "Motor Test", OLED_6X8);
    OLED_ShowString(0, 24, "Stepper Test", OLED_6X8);
    OLED_ShowString(0, 36, "NRF Test", OLED_6X8);
    OLED_ShowString(0, 48, "All Control", OLED_6X8);
    OLED_ShowString(0, 56, "K1/2 Sel K3 OK", OLED_6X8);

    switch ((MenuItem_t)g_oled_display_data.menu_cursor)
    {
        case MENU_ITEM_MOTOR:
            OLED_ReverseArea(0, 12, 128, 8);
            break;
        case MENU_ITEM_STEPPER:
            OLED_ReverseArea(0, 24, 128, 8);
            break;
        case MENU_ITEM_NRF:
            OLED_ReverseArea(0, 36, 128, 8);
            break;
        case MENU_ITEM_ALL_CONTROL:
            OLED_ReverseArea(0, 48, 128, 8);
            break;
        default:
            break;
    }
}

/* 根据模块内快照绘制直流电机诊断页面。 */
static void App_DrawMotorPage(void)
{
    DCMotor_Status_t status = g_oled_display_data.motor_status;

    OLED_ShowString(0, 0, "Motor Test", OLED_6X8);
    OLED_ShowString(72, 0, "K4 Back", OLED_6X8);

    OLED_ShowString(0, 12, "L:", OLED_6X8);
    OLED_ShowString(12, 12, (char *)DCMotor_DirectionString(status.left_dir), OLED_6X8);
    OLED_ShowSignedNum(36, 12, status.left_duty_percent, 3, OLED_6X8);
    OLED_ShowString(60, 12, "%", OLED_6X8);

    OLED_ShowString(0, 24, "R:", OLED_6X8);
    OLED_ShowString(12, 24, (char *)DCMotor_DirectionString(status.right_dir), OLED_6X8);
    OLED_ShowSignedNum(36, 24, status.right_duty_percent, 3, OLED_6X8);
    OLED_ShowString(60, 24, "%", OLED_6X8);

    OLED_ShowString(0, 40, "L m/s:", OLED_6X8);
    OLED_ShowFloatNum(36, 40, status.left_speed_mps, 4, 2, OLED_6X8);
    OLED_ShowString(0, 52, "R m/s:", OLED_6X8);
    OLED_ShowFloatNum(36, 52, status.right_speed_mps, 4, 2, OLED_6X8);
}

/* 根据模块内快照绘制步进电机诊断页面。 */
static void App_DrawStepperPage(void)
{
    OLED_ShowString(0, 0, "Stepper Test", OLED_6X8);
    OLED_ShowString(66, 0, "K4 Back", OLED_6X8);

    OLED_ShowString(0, 12, "L Dir:", OLED_6X8);
    OLED_ShowString(42, 12, g_oled_display_data.stepper_dir_l == STEPPER_DIR_CW ? "CW" : "CCW", OLED_6X8);
    OLED_ShowString(72, 12, "EN:", OLED_6X8);
    OLED_ShowSignedNum(90, 12, g_oled_display_data.stepper_enable, 1, OLED_6X8);

    OLED_ShowString(0, 24, "L RPM:", OLED_6X8);
    OLED_ShowFloatNum(42, 24, g_oled_display_data.stepper_rpm_l, 4, 1, OLED_6X8);

    OLED_ShowString(0, 40, "R Dir:", OLED_6X8);
    OLED_ShowString(42, 40, g_oled_display_data.stepper_dir_r == STEPPER_DIR_CW ? "CW" : "CCW", OLED_6X8);

    OLED_ShowString(0, 52, "R RPM:", OLED_6X8);
    OLED_ShowFloatNum(42, 52, g_oled_display_data.stepper_rpm_r, 4, 1, OLED_6X8);
}

/* 根据模块内快照绘制 NRF 诊断页面。 */
static void App_DrawNrfPage(void)
{
    OLED_ShowString(0, 0, "NRF Test", OLED_6X8);
    OLED_ShowString(78, 0, "K4 Back", OLED_6X8);

    OLED_ShowString(0, 12, "Flg:", OLED_6X8);
    OLED_ShowNum(24, 12, g_oled_display_data.nrf_last_rx_flag, 1, OLED_6X8);

    OLED_ShowString(0, 24, "OK:", OLED_6X8);
    OLED_ShowNum(18, 24, g_oled_display_data.nrf_rx_ok_count % 10000U, 4, OLED_6X8);
    OLED_ShowString(54, 24, "ERR:", OLED_6X8);
    OLED_ShowNum(78, 24, g_oled_display_data.nrf_rx_err_count % 10000U, 4, OLED_6X8);

    OLED_ShowString(0, 40, "RX:", OLED_6X8);
    OLED_ShowHexNum(18, 40, g_oled_display_data.nrf_last_payload[0], 2, OLED_6X8);
    OLED_ShowChar(30, 40, ' ', OLED_6X8);
    OLED_ShowHexNum(36, 40, g_oled_display_data.nrf_last_payload[1], 2, OLED_6X8);
    OLED_ShowChar(48, 40, ' ', OLED_6X8);
    OLED_ShowHexNum(54, 40, g_oled_display_data.nrf_last_payload[2], 2, OLED_6X8);
    OLED_ShowChar(66, 40, ' ', OLED_6X8);
    OLED_ShowHexNum(72, 40, g_oled_display_data.nrf_last_payload[3], 2, OLED_6X8);

    OLED_ShowString(0, 56, "Recv data preview", OLED_6X8);
}

/* 根据模块内快照绘制总控运行页面。 */
static void App_DrawAllControlPage(void)
{
    DCMotor_Status_t s = g_oled_display_data.motor_status;

    OLED_ShowString(0, 0, "All Control", OLED_6X8);
    OLED_ShowString(66, 0, "K4 Back", OLED_6X8);

    OLED_ShowString(0, 12, "LJoy:", OLED_6X8);
    OLED_ShowNum(30, 12, g_oled_display_data.js_left, 3, OLED_6X8);
    OLED_ShowString(64, 12, "RJoy:", OLED_6X8);
    OLED_ShowNum(94, 12, g_oled_display_data.js_right, 3, OLED_6X8);

    OLED_ShowString(0, 24, "DCL:", OLED_6X8);
    OLED_ShowFloatNum(24, 24, s.left_speed_mps, 1, 2, OLED_6X8);
    OLED_ShowString(64, 24, "DCR:", OLED_6X8);
    OLED_ShowFloatNum(88, 24, s.right_speed_mps, 1, 2, OLED_6X8);

    OLED_ShowString(0, 40, "Steer T:", OLED_6X8);
    OLED_ShowFloatNum(42, 40, g_oled_display_data.steer_target_deg, 2, 1, OLED_6X8);

    OLED_ShowString(0, 52, "Steer C:", OLED_6X8);
    OLED_ShowFloatNum(42, 52, g_oled_display_data.steer_current_deg, 2, 1, OLED_6X8);
}

/*
 * OLED 渲染主循环：
 * - 50ms 周期刷新；
 * - 清空当前周期内待处理按键事件；
 * - 获取最新显示快照并完成绘制。
 */
void OLED_Task_Entry(void *argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50);

    (void)argument;

    App_SyncDisplayDataFromShared();

    while (1)
    {
        uint8_t key_event;

        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        key_event = App_FetchKeyEvent();
        while (key_event != 0U)
        {
            App_HandleMenuKey(key_event);
            key_event = App_FetchKeyEvent();
        }

        /* 优先使用队列快照，失败时回退到共享状态直读。 */
        if ((OLEDDataQueueHandle == NULL) ||
            (xQueueReceive(OLEDDataQueueHandle, &g_oled_display_data, 0U) != pdPASS))
        {
            App_SyncDisplayDataFromShared();
        }

        OLED_Clear();
        if (g_oled_display_data.in_test == 0U)
        {
            App_DrawMainMenu();
        }
        else
        {
            switch (g_oled_display_data.app_mode)
            {
                case APP_MODE_MOTOR:
                    App_DrawMotorPage();
                    break;
                case APP_MODE_STEPPER:
                    App_DrawStepperPage();
                    break;
                case APP_MODE_NRF:
                    App_DrawNrfPage();
                    break;
                case APP_MODE_ALL_CONTROL:
                    App_DrawAllControlPage();
                    break;
                case APP_MODE_IDLE:
                default:
                    App_DrawMainMenu();
                    break;
            }
        }

        OLED_Update();
    }
}
