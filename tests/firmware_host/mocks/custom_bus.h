#ifndef MOCK_CUSTOM_BUS_H
#define MOCK_CUSTOM_BUS_H
#include <stddef.h>
#include <stdint.h>
int32_t BSP_I2C2_Init(void);
int32_t BSP_I2C2_WriteReg(uint16_t, uint16_t, uint8_t *, uint16_t);
int32_t BSP_I2C2_ReadReg(uint16_t, uint16_t, uint8_t *, uint16_t);
#endif
