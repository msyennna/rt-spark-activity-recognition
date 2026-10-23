#ifndef ICM20608_H
#define ICM20608_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RT-Spark onboard ICM20608 */
#define ICM20608_I2C_ADDRESS       (0x68U << 1)
#define ICM20608_WHO_AM_I_VALUE    0xAFU

typedef struct
{
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;

    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;

    float accel_x_g;
    float accel_y_g;
    float accel_z_g;

    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
} ICM20608_Data_t;

int32_t ICM20608_Init(void);
int32_t ICM20608_ReadID(uint8_t *id);
int32_t ICM20608_ReadData(ICM20608_Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* ICM20608_H */