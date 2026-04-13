#include "nrf24l01.h"
#include "spi.h"

/*
 * ==============================================================
 *  这份文件是 NRF24L01 的“底层通信司机”
 *
 *  你可以把它想成一个“快递员”：
 *  - CE 像“开始工作开关”
 *  - CSN 像“选中这个设备”
 *  - SPI 像“说话的线”
 *
 *  我们做的事情就是：
 *  1) 先选中芯片（CSN 拉低）
 *  2) 发命令 + 发/收数据（SPI）
 *  3) 结束本次操作（CSN 拉高）
 * ==============================================================
 */

/* 引脚定义：直接复用 CubeMX 在 main.h 里生成的宏，不在这里写死端口 */
#define NRF24L01_CE_PIN       NRF_CE_Pin
#define NRF24L01_CE_PORT      NRF_CE_GPIO_Port
#define NRF24L01_CSN_PIN      NRF_CSN_Pin
#define NRF24L01_CSN_PORT     NRF_CSN_GPIO_Port

/* SPI定义：统一使用 CubeMX 生成的 hspi2 */
#define NRF24L01_SPI          hspi2

/*
 * 地址说明（5字节）：
 * - TxAddress：当前发送目标地址
 * - RxAddress：本机接收地址
 *
 * 提醒：两个设备要通信，地址、信道、速率、包长都要匹配。
 */
uint8_t NRF24L01_TxAddress[5] = {0x55, 0x66, 0x77, 0x88, 0x99};
uint8_t NRF24L01_TxPacket[NRF24L01_TX_PACKET_WIDTH];
uint8_t NRF24L01_RxAddress[5] = {0x55, 0x66, 0x77, 0x88, 0x99};
uint8_t NRF24L01_RxPacket[NRF24L01_RX_PACKET_WIDTH];

/* 写 CE 引脚：0=停在待机，1=进入收发工作状态 */
static inline void NRF24L01_W_CE(uint8_t BitValue)
{
    HAL_GPIO_WritePin(NRF24L01_CE_PORT, NRF24L01_CE_PIN, (GPIO_PinState)BitValue);
}

/* 写 CSN 引脚：0=选中 NRF24L01；1=取消选中 */
static inline void NRF24L01_W_CSN(uint8_t BitValue)
{
    HAL_GPIO_WritePin(NRF24L01_CSN_PORT, NRF24L01_CSN_PIN, (GPIO_PinState)BitValue);
}

/*
 * SPI 交换 1 字节（发1字节，同时收1字节）
 * - TxData：发出去的字节
 * - 返回值：收到的字节
 */
static uint8_t NRF24L01_SPI_SwapByte(uint8_t TxData)
{
    uint8_t RxData = 0xFF;

    /* HAL_OK 说明通信成功；失败时返回 0xFF 作为异常提示 */
    if (HAL_SPI_TransmitReceive(&NRF24L01_SPI, &TxData, &RxData, 1, 100) != HAL_OK)
    {
        return 0xFF;
    }

    return RxData;
}

/* 读 1 个寄存器 */
uint8_t NRF24L01_ReadReg(uint8_t RegAddress)
{
    uint8_t Data;

    NRF24L01_W_CSN(0);                                         /* 开始事务 */
    NRF24L01_SPI_SwapByte(NRF24L01_R_REGISTER | RegAddress);   /* 发“读寄存器”命令 */
    Data = NRF24L01_SPI_SwapByte(NRF24L01_NOP);                /* 再发1字节，占位并读回数据 */
    NRF24L01_W_CSN(1);                                         /* 结束事务 */

    return Data;
}

/* 连续读多个寄存器字节（常用于读地址） */
void NRF24L01_ReadRegs(uint8_t RegAddress, uint8_t *DataArray, uint8_t Count)
{
    uint8_t i;

    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_R_REGISTER | RegAddress);

    for (i = 0; i < Count; i++)
    {
        DataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
    }

    NRF24L01_W_CSN(1);
}

/* 写 1 个寄存器 */
void NRF24L01_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_W_REGISTER | RegAddress);
    NRF24L01_SPI_SwapByte(Data);
    NRF24L01_W_CSN(1);
}

/* 连续写多个寄存器字节（常用于写地址） */
void NRF24L01_WriteRegs(uint8_t RegAddress, const uint8_t *DataArray, uint8_t Count)
{
    uint8_t i;

    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_W_REGISTER | RegAddress);

    for (i = 0; i < Count; i++)
    {
        NRF24L01_SPI_SwapByte(DataArray[i]);
    }

    NRF24L01_W_CSN(1);
}

/* 读取接收 FIFO 中的有效载荷（真正收到的用户数据） */
void NRF24L01_ReadRxPayload(uint8_t *DataArray, uint8_t Count)
{
    uint8_t i;

    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_R_RX_PAYLOAD);

    for (i = 0; i < Count; i++)
    {
        DataArray[i] = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
    }

    NRF24L01_W_CSN(1);
}

/* 写发送 FIFO 的有效载荷（准备发送的数据） */
void NRF24L01_WriteTxPayload(const uint8_t *DataArray, uint8_t Count)
{
    uint8_t i;

    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_W_TX_PAYLOAD);

    for (i = 0; i < Count; i++)
    {
        NRF24L01_SPI_SwapByte(DataArray[i]);
    }

    NRF24L01_W_CSN(1);
}

/* 清空发送 FIFO，防止历史数据残留 */
void NRF24L01_FlushTx(void)
{
    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_TX);
    NRF24L01_W_CSN(1);
}

/* 清空接收 FIFO，防止旧数据影响 */
void NRF24L01_FlushRx(void)
{
    NRF24L01_W_CSN(0);
    NRF24L01_SPI_SwapByte(NRF24L01_FLUSH_RX);
    NRF24L01_W_CSN(1);
}

/* 读状态寄存器（非常常用） */
uint8_t NRF24L01_ReadStatus(void)
{
    uint8_t Status;

    NRF24L01_W_CSN(0);
    Status = NRF24L01_SPI_SwapByte(NRF24L01_NOP);
    NRF24L01_W_CSN(1);

    return Status;
}

/* 进入掉电模式：省电，但不能马上收发 */
void NRF24L01_PowerDown(void)
{
    uint8_t Config;

    NRF24L01_W_CE(0);

    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) { return; }     /* 读到 0xFF 常见于SPI/接线异常 */

    Config &= (uint8_t)~0x02;           /* 清 PWR_UP 位 */
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);
}

/* 进入待机1模式：可快速切换到收发 */
void NRF24L01_StandbyI(void)
{
    uint8_t Config;

    NRF24L01_W_CE(0);

    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) { return; }

    Config |= 0x02;                      /* 置 PWR_UP 位 */
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);

    /* 官方时序：掉电->待机要 >1.5ms */
    HAL_Delay(2);
}

/* 进入接收模式（PRIM_RX=1） */
void NRF24L01_Rx(void)
{
    uint8_t Config;

    NRF24L01_W_CE(0);

    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) { return; }

    Config |= 0x03;                      /* PWR_UP=1, PRIM_RX=1 */
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);

    HAL_Delay(2);                        /* 给芯片一点反应时间 */
    NRF24L01_W_CE(1);                    /* CE=1，真正开始监听空口 */
}

/* 进入发送模式（PRIM_RX=0） */
void NRF24L01_Tx(void)
{
    uint8_t Config;

    NRF24L01_W_CE(0);

    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if (Config == 0xFF) { return; }

    Config |= 0x02;                      /* PWR_UP=1 */
    Config &= (uint8_t)~0x01;            /* PRIM_RX=0 */
    NRF24L01_WriteReg(NRF24L01_CONFIG, Config);

    HAL_Delay(2);
    NRF24L01_W_CE(1);                    /* CE=1，触发发送 */
}

/*
 * 模块初始化（建议开机只调一次，异常时也可重调）
 * 初始化后默认切到接收模式。
 */
void NRF24L01_Init(void)
{
    NRF24L01_W_CE(0);                    /* 先别工作 */
    NRF24L01_W_CSN(1);                   /* SPI空闲状态取消片选 */

    HAL_Delay(100);                      /* 等芯片上电稳定 */

    /* 基础无线参数配置 */
    NRF24L01_WriteReg(NRF24L01_CONFIG, 0x08);                 /* 先关电，开CRC默认配置 */
    NRF24L01_WriteReg(NRF24L01_EN_AA, 0x3F);                  /* 开自动应答 */
    NRF24L01_WriteReg(NRF24L01_EN_RXADDR, 0x01);              /* 使能通道0 */
    NRF24L01_WriteReg(NRF24L01_SETUP_AW, 0x03);               /* 地址宽度=5字节 */
    NRF24L01_WriteReg(NRF24L01_SETUP_RETR, 0x03);             /* 自动重发参数 */
    NRF24L01_WriteReg(NRF24L01_RF_CH, 0x02);                  /* 信道 */
    NRF24L01_WriteReg(NRF24L01_RF_SETUP, 0x0E);               /* 速率/发射功率 */

    /* 设置接收包长度和本机接收地址 */
    NRF24L01_WriteReg(NRF24L01_RX_PW_P0, NRF24L01_RX_PACKET_WIDTH);
    NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, 5);

    /* 清FIFO和状态标志 */
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_STATUS, 0x70);                 /* 清中断标志位 */

    /* 默认进入接收监听 */
    NRF24L01_Rx();
}

/*
 * 发送 1 包数据
 * 返回：
 * 1=发送成功
 * 2=达到最大重发次数
 * 3=状态寄存器不合法
 * 4=超时
 */
uint8_t NRF24L01_Send(void)
{
    uint8_t Status;
    uint8_t SendFlag;
    uint32_t Timeout;

    /*
     * 发数据前关键步骤：
     * 1) 设置目标地址（TX_ADDR）
     * 2) 为了自动应答，RX_ADDR_P0 也要同步成目标地址
     * 3) 把待发数据写进 TX FIFO
     */
    NRF24L01_WriteRegs(NRF24L01_TX_ADDR, NRF24L01_TxAddress, 5);
    NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_TxAddress, 5);
    NRF24L01_WriteTxPayload(NRF24L01_TxPacket, NRF24L01_TX_PACKET_WIDTH);

    NRF24L01_Tx();

    /* 简单轮询等待结果 */
    Timeout = 10000;
    while (1)
    {
        Status = NRF24L01_ReadStatus();

        Timeout--;
        if (Timeout == 0)
        {
            SendFlag = 4;
            NRF24L01_Init();
            break;
        }

        /* 状态异常：MAX_RT 和 TX_DS 同时置位，视为不合法 */
        if ((Status & 0x30) == 0x30)
        {
            SendFlag = 3;
            NRF24L01_Init();
            break;
        }
        /* 达到最大重发次数 */
        else if ((Status & 0x10) == 0x10)
        {
            SendFlag = 2;
            NRF24L01_Init();
            break;
        }
        /* 发送成功 */
        else if ((Status & 0x20) == 0x20)
        {
            SendFlag = 1;
            break;
        }
    }

    /* 发完收尾：清标志、清发送FIFO、恢复本机接收地址并回到RX */
    NRF24L01_WriteReg(NRF24L01_STATUS, 0x30);
    NRF24L01_FlushTx();
    NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, 5);
    NRF24L01_Rx();

    return SendFlag;
}

/*
 * 接收检查函数（轮询版）
 * 返回：
 * 0=没收到
 * 1=收到新包
 * 2=状态异常
 * 3=还在掉电
 */
uint8_t NRF24L01_Receive(void)
{
    uint8_t Status, Config;
    uint8_t ReceiveFlag;

    Status = NRF24L01_ReadStatus();
    Config = NRF24L01_ReadReg(NRF24L01_CONFIG);

    /* 还没上电工作 */
    if ((Config & 0x02) == 0x00)
    {
        ReceiveFlag = 3;
        NRF24L01_Init();
    }
    /* 状态不合法 */
    else if ((Status & 0x30) == 0x30)
    {
        ReceiveFlag = 2;
        NRF24L01_Init();
    }
    /* 收到数据 */
    else if ((Status & 0x40) == 0x40)
    {
        ReceiveFlag = 1;

        NRF24L01_ReadRxPayload(NRF24L01_RxPacket, NRF24L01_RX_PACKET_WIDTH);
        NRF24L01_WriteReg(NRF24L01_STATUS, 0x40); /* 清RX_DR标志 */
        NRF24L01_FlushRx();
    }
    else
    {
        ReceiveFlag = 0;
    }

    return ReceiveFlag;
}

/* 当你修改了 RxAddress 数组后，调用这个函数把新地址写入芯片 */
void NRF24L01_UpdateRxAddress(void)
{
    NRF24L01_WriteRegs(NRF24L01_RX_ADDR_P0, NRF24L01_RxAddress, 5);
}
