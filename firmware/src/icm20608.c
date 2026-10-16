#include "icm20608.h"
#include "custom_bus.h"
#include "stm32f4xx_hal.h"

/* ICM20608 registers */
#define ICM20608_REG_SMPLRT_DIV      0x19U
#define ICM20608_REG_CONFIG          0x1AU
#define ICM20608_REG_GYRO_CONFIG     0x1BU
#define ICM20608_REG_ACCEL_CONFIG    0x1CU
#define ICM20608_REG_ACCEL_CONFIG2   0x1DU
#define ICM20608_REG_ACCEL_XOUT_H    0x3BU
#define ICM20608_REG_PWR_MGMT_1      0x6BU
#define ICM20608_REG_PWR_MGMT_2      0x6CU
#define ICM20608_REG_WHO_AM_I        0x75U

/* Selected measurement ranges */
#define ICM20608_ACCEL_SCALE         16384.0f  /* +/-2 g */
#define ICM20608_GYRO_SCALE          131.0f    /* +/-250 dps */

static int32_t ICM20608_WriteRegister(uint8_t reg, uint8_t value)
{
    return BSP_I2C2_WriteReg(
        ICM20608_I2C_ADDRESS,
        reg,
        &value,
        1U
    );
}

static int32_t ICM20608_ReadRegisters(uint8_t reg,
                                      uint8_t *data,
                                      uint16_t length)
{
    return BSP_I2C2_ReadReg(
        ICM20608_I2C_ADDRESS,
        reg,
        data,
        length
    );
}

int32_t ICM20608_ReadID(uint8_t *id)
{
    if (id == NULL)
    {
        return -1;
    }

    return ICM20608_ReadRegisters(
        ICM20608_REG_WHO_AM_I,
        id,
        1U
    );
}

int32_t ICM20608_Init(void)
{
    uint8_t id = 0;

    if (BSP_I2C2_Init() != 0)
    {
        return -1;
    }

    if (ICM20608_ReadID(&id) != 0)
    {
        return -1;
    }

    if (id != ICM20608_WHO_AM_I_VALUE)
    {
        return -1;
    }

    /* Reset sensor */
    if (ICM20608_WriteRegister(ICM20608_REG_PWR_MGMT_1, 0x80U) != 0)
    {
        return -1;
    }

    HAL_Delay(100);

    /* Wake sensor and use PLL clock */
    if (ICM20608_WriteRegister(ICM20608_REG_PWR_MGMT_1, 0x01U) != 0)
    {
        return -1;
    }

    HAL_Delay(10);

    /* Enable accelerometer and gyroscope axes */
    if (ICM20608_WriteRegister(ICM20608_REG_PWR_MGMT_2, 0x00U) != 0)
    {
        return -1;
    }

    /* Sample-rate divider */
    if (ICM20608_WriteRegister(ICM20608_REG_SMPLRT_DIV, 0x00U) != 0)
    {
        return -1;
    }

    /* Digital low-pass filtering */
    if (ICM20608_WriteRegister(ICM20608_REG_CONFIG, 0x03U) != 0)
    {
        return -1;
    }

    /*
     * Gyroscope range:
     * 0x00 = +/-250 degrees/s
     * Sensitivity = 131 LSB/(degree/s)
     */
    if (ICM20608_WriteRegister(ICM20608_REG_GYRO_CONFIG, 0x00U) != 0)
    {
        return -1;
    }

    /*
     * Accelerometer range:
     * 0x00 = +/-2 g
     * Sensitivity = 16384 LSB/g
     */
    if (ICM20608_WriteRegister(ICM20608_REG_ACCEL_CONFIG, 0x00U) != 0)
    {
        return -1;
    }

    if (ICM20608_WriteRegister(ICM20608_REG_ACCEL_CONFIG2, 0x03U) != 0)
    {
        return -1;
    }

    HAL_Delay(20);

    return 0;
}

int32_t ICM20608_ReadData(ICM20608_Data_t *data)
{
    uint8_t raw[14];

    if (data == NULL)
    {
        return -1;
    }

    /*
     * Registers 0x3B onward:
     * Accel X/Y/Z = 6 bytes
     * Temperature = 2 bytes
     * Gyro X/Y/Z  = 6 bytes
     */
    if (ICM20608_ReadRegisters(
            ICM20608_REG_ACCEL_XOUT_H,
            raw,
            sizeof(raw)) != 0)
    {
        return -1;
    }

    data->accel_x_raw =
        (int16_t)(((uint16_t)raw[0] << 8) | raw[1]);

    data->accel_y_raw =
        (int16_t)(((uint16_t)raw[2] << 8) | raw[3]);

    data->accel_z_raw =
        (int16_t)(((uint16_t)raw[4] << 8) | raw[5]);

    data->gyro_x_raw =
        (int16_t)(((uint16_t)raw[8] << 8) | raw[9]);

    data->gyro_y_raw =
        (int16_t)(((uint16_t)raw[10] << 8) | raw[11]);

    data->gyro_z_raw =
        (int16_t)(((uint16_t)raw[12] << 8) | raw[13]);

    data->accel_x_g =
        (float)data->accel_x_raw / ICM20608_ACCEL_SCALE;

    data->accel_y_g =
        (float)data->accel_y_raw / ICM20608_ACCEL_SCALE;

    data->accel_z_g =
        (float)data->accel_z_raw / ICM20608_ACCEL_SCALE;

    data->gyro_x_dps =
        (float)data->gyro_x_raw / ICM20608_GYRO_SCALE;

    data->gyro_y_dps =
        (float)data->gyro_y_raw / ICM20608_GYRO_SCALE;

    data->gyro_z_dps =
        (float)data->gyro_z_raw / ICM20608_GYRO_SCALE;

    return 0;
}