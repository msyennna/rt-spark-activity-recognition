#include <rtthread.h>
#include <rtdevice.h>
#include "icm20608.h"

int main(void)
{
    icm20608_device_t imu;
    rt_int16_t ax, ay, az;
    rt_int16_t gx, gy, gz;
    rt_err_t accel_result;
    rt_err_t gyro_result;

    if (rt_device_find("i2c2") == RT_NULL)
    {
        rt_kprintf("ERROR: i2c2 was not found\n");
        return -1;
    }

    imu = icm20608_init("i2c2");
    if (imu == RT_NULL)
    {
        rt_kprintf("ERROR: ICM20608 initialization failed\n");
        return -1;
    }

    rt_kprintf("ICM20608 ready\n");
    rt_kprintf("ax,ay,az,gx,gy,gz (raw sensor values)\n");

    while (1)
    {
        accel_result = icm20608_get_accel(imu, &ax, &ay, &az);
        gyro_result = icm20608_get_gyro(imu, &gx, &gy, &gz);

        if ((accel_result == RT_EOK) && (gyro_result == RT_EOK))
        {
            rt_kprintf("%d,%d,%d,%d,%d,%d\n", ax, ay, az, gx, gy, gz);
        }
        else
        {
            rt_kprintf("Sensor read failed: accel=%d gyro=%d\n",
                       accel_result, gyro_result);
        }

        rt_thread_mdelay(100);
    }
}
