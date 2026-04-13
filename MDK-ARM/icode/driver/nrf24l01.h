#ifndef __NRF24L01_H
#define __NRF24L01_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ==================== NRF24L01 指令/寄存器定义 ==================== */
/* 指令 */
#define NRF24L01_R_REGISTER      0x00
#define NRF24L01_W_REGISTER      0x20
#define NRF24L01_R_RX_PAYLOAD    0x61
#define NRF24L01_W_TX_PAYLOAD    0xA0
#define NRF24L01_FLUSH_TX        0xE1
#define NRF24L01_FLUSH_RX        0xE2
#define NRF24L01_NOP             0xFF

/* 寄存器地址 */
#define NRF24L01_CONFIG          0x00
#define NRF24L01_EN_AA           0x01
#define NRF24L01_EN_RXADDR       0x02
#define NRF24L01_SETUP_AW        0x03
#define NRF24L01_SETUP_RETR      0x04
#define NRF24L01_RF_CH           0x05
#define NRF24L01_RF_SETUP        0x06
#define NRF24L01_STATUS          0x07
#define NRF24L01_RX_ADDR_P0      0x0A
#define NRF24L01_TX_ADDR         0x10
#define NRF24L01_RX_PW_P0        0x11

/* 数据包宽度 */
#define NRF24L01_TX_PACKET_WIDTH 4u
#define NRF24L01_RX_PACKET_WIDTH 4u

/* 外部可调用全局数组 */
extern uint8_t NRF24L01_TxAddress[5];
extern uint8_t NRF24L01_TxPacket[NRF24L01_TX_PACKET_WIDTH];
extern uint8_t NRF24L01_RxAddress[5];
extern uint8_t NRF24L01_RxPacket[NRF24L01_RX_PACKET_WIDTH];

/* 指令实现 */
uint8_t NRF24L01_ReadReg(uint8_t RegAddress);
void NRF24L01_ReadRegs(uint8_t RegAddress, uint8_t *DataArray, uint8_t Count);
void NRF24L01_WriteReg(uint8_t RegAddress, uint8_t Data);
void NRF24L01_WriteRegs(uint8_t RegAddress, const uint8_t *DataArray, uint8_t Count);
void NRF24L01_ReadRxPayload(uint8_t *DataArray, uint8_t Count);
void NRF24L01_WriteTxPayload(const uint8_t *DataArray, uint8_t Count);
void NRF24L01_FlushTx(void);
void NRF24L01_FlushRx(void);
uint8_t NRF24L01_ReadStatus(void);

/* 功能函数 */
void NRF24L01_PowerDown(void);
void NRF24L01_StandbyI(void);
void NRF24L01_Rx(void);
void NRF24L01_Tx(void);
void NRF24L01_Init(void);
uint8_t NRF24L01_Send(void);
uint8_t NRF24L01_Receive(void);
void NRF24L01_UpdateRxAddress(void);

#ifdef __cplusplus
}
#endif

#endif
