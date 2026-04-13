#include "Serial.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

//usart2
char Serial2_RxPacket[500];
uint8_t Serial2_RxFlag;

// 外部引用 CubeMX 生成的串口句柄
extern UART_HandleTypeDef huart2;

// --- DMA 专用变量 ---
#define RX_BUFFER_SIZE              500U
#define TX_DMA_CHUNK_SIZE           256U
#define RX_DMA_DOUBLE_BUFFER_COUNT  2U
#define TX_DMA_DOUBLE_BUFFER_COUNT  2U

static uint8_t Serial2_RxBuffer_DMA[RX_DMA_DOUBLE_BUFFER_COUNT][RX_BUFFER_SIZE];
static uint8_t Serial2_TxBuffer_DMA[TX_DMA_DOUBLE_BUFFER_COUNT][TX_DMA_CHUNK_SIZE];
static volatile uint8_t Serial2_TxBusy = 0U;
static volatile uint8_t Serial2_TxActiveIndex = 0U;
static volatile uint8_t Serial2_RxActiveIndex = 0U;
static SemaphoreHandle_t Serial2_TxMutex = NULL;

/*==========================================================
 * 内部辅助函数
 *==========================================================*/

static void Serial2_WaitTxFinish(void)
{
    while (Serial2_TxBusy != 0U)
    {
    }
}

static void Serial2_DMATx(uint8_t *buf, uint16_t len)
{
    uint16_t offset = 0U;
    BaseType_t lockTaken = pdFALSE;

    if ((buf == NULL) || (len == 0U))
    {
        return;
    }

    if ((Serial2_TxMutex != NULL) && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED))
    {
        lockTaken = xSemaphoreTake(Serial2_TxMutex, portMAX_DELAY);
        if (lockTaken != pdTRUE)
        {
            return;
        }
    }

    while (offset < len)
    {
        uint8_t txIndex;
        uint16_t chunk = (uint16_t)(len - offset);
        if (chunk > TX_DMA_CHUNK_SIZE)
        {
            chunk = TX_DMA_CHUNK_SIZE;
        }

        Serial2_WaitTxFinish();
        txIndex = Serial2_TxActiveIndex;
        memcpy(Serial2_TxBuffer_DMA[txIndex], &buf[offset], chunk);
        Serial2_TxBusy = 1U;

        if (HAL_UART_Transmit_DMA(&huart2, Serial2_TxBuffer_DMA[txIndex], chunk) != HAL_OK)
        {
            Serial2_TxBusy = 0U;
            HAL_UART_Transmit(&huart2, Serial2_TxBuffer_DMA[txIndex], chunk, 1000U);
        }
        else
        {
            Serial2_TxActiveIndex = (uint8_t)((Serial2_TxActiveIndex + 1U) % TX_DMA_DOUBLE_BUFFER_COUNT);
            Serial2_WaitTxFinish();
        }

        offset = (uint16_t)(offset + chunk);
    }

    if (lockTaken == pdTRUE)
    {
        (void)xSemaphoreGive(Serial2_TxMutex);
    }
}

/*==========================================================
 * 初始化函数
 *==========================================================*/
void Serial2_Init(void)
{
    Serial2_RxFlag = 0U;
    Serial2_TxBusy = 0U;
    Serial2_TxActiveIndex = 0U;
    Serial2_RxActiveIndex = 0U;
    if (Serial2_TxMutex == NULL)
    {
        Serial2_TxMutex = xSemaphoreCreateMutex();
    }

    if ((huart2.hdmarx != NULL) && (huart2.hdmarx->Init.Mode != DMA_NORMAL))
    {
        huart2.hdmarx->Init.Mode = DMA_NORMAL;
        (void)HAL_DMA_Init(huart2.hdmarx);
    }

    if ((huart2.hdmatx != NULL) && (huart2.hdmatx->Init.Mode != DMA_NORMAL))
    {
        huart2.hdmatx->Init.Mode = DMA_NORMAL;
        (void)HAL_DMA_Init(huart2.hdmatx);
    }

    // 启动接收 DMA 双缓冲（ping-pong）：每次回调切换到另一块缓冲区
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart2,
                                       Serial2_RxBuffer_DMA[Serial2_RxActiveIndex],
                                       RX_BUFFER_SIZE);
    if (huart2.hdmarx != NULL)
    {
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
    }
}

/*==========================================================
 * 发送函数集 (保持 DMA 发送逻辑)
 *==========================================================*/

void Serial2_SendByte(uint8_t byte)
{
    uint8_t tempByte = byte;
    Serial2_DMATx(&tempByte, 1U);
}

void Serial2_SendArray(uint8_t *array, uint16_t length)
{
    Serial2_DMATx(array, length);
}

void Serial2_SendString(char *str)
{
    if (str == NULL)
    {
        return;
    }

    Serial2_DMATx((uint8_t *)str, (uint16_t)strlen(str));
}

uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t result = 1;
    while (Y--) result *= X;
    return result;
}

void Serial2_SendNumber(uint32_t num, uint8_t length)
{
    static char numBuffer[20];
    if (length > 19) length = 19;
    for (uint8_t i = 0; i < length; i++)
    {
        uint8_t digit = (num / Serial_Pow(10, length - i - 1)) % 10;
        numBuffer[i] = digit + '0';
    }
    numBuffer[length] = '\0';
    Serial2_SendString(numBuffer);
}

void Serial2_Printf(char *format, ...)
{
    static char buffer_Serial2[500]; 
    va_list args;
    va_start(args, format);
    vsnprintf(buffer_Serial2, sizeof(buffer_Serial2), format, args);
    va_end(args);
    Serial2_DMATx((uint8_t *)buffer_Serial2, (uint16_t)strlen(buffer_Serial2));
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        Serial2_TxBusy = 0U;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        Serial2_TxBusy = 0U;
    }
}

/*==========================================================
 * 接收处理逻辑 (核心修改)
 *==========================================================*/

/**
 * @brief 内部函数：处理单个字节的协议解析
 * 将原来 Serial2_RxHandler 的逻辑移到这里
 */
static void Serial2_ProcessByte(uint8_t byte)
{
    static uint8_t RxState2 = 0;
    static uint16_t pRxPacket2 = 0U;

    if (RxState2 == 0)
    {
        if (byte == '@' && Serial2_RxFlag == 0)
        {
            RxState2 = 1;
            pRxPacket2 = 0;
        }
    }
    else if (RxState2 == 1)
    {
        if (byte == '\r')
        {
            RxState2 = 2;
        }
        else
        {
            if (pRxPacket2 < 499U)
            {
                Serial2_RxPacket[pRxPacket2++] = byte;
            }
        }
    }
    else if (RxState2 == 2)
    {
        if (byte == '\n')
        {
            RxState2 = 0;
            Serial2_RxPacket[pRxPacket2] = '\0';
            Serial2_RxFlag = 1;
        }
    }
}

/**
 * @brief Serial2 专用的 DMA 接收事件处理函数
 * @note  这个函数由 main.c 中的 HAL_UARTEx_RxEventCallback 调用
 * @param Size: 当前 DMA 缓冲区接收到的数据总量 (由 HAL 库自动计算并传入)
 */
void Serial2_DMA_RxEvent(uint16_t Size)
{
    uint16_t i;
    uint16_t validSize = Size;
    uint8_t currRxIndex = Serial2_RxActiveIndex;

    if (validSize > RX_BUFFER_SIZE)
    {
        validSize = RX_BUFFER_SIZE;
    }

    for (i = 0U; i < validSize; i++)
    {
        Serial2_ProcessByte(Serial2_RxBuffer_DMA[currRxIndex][i]);
    }

    Serial2_RxActiveIndex = (uint8_t)((currRxIndex + 1U) % RX_DMA_DOUBLE_BUFFER_COUNT);
    (void)HAL_UARTEx_ReceiveToIdle_DMA(&huart2,
                                       Serial2_RxBuffer_DMA[Serial2_RxActiveIndex],
                                       RX_BUFFER_SIZE);
    if (huart2.hdmarx != NULL)
    {
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
    }
}
