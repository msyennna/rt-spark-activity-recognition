#include <rtthread.h>
#include <rtdevice.h>
#include <wlan_mgnt.h>
#include "icm20608.h"
#include "rtspark_app.h"
#include "rtspark_private.h"

static void publisher(void *argument)
{
    icm20608_device_t imu;
    unsigned long sent = 0;
    (void)argument;
    imu = icm20608_init("i2c2");
    if (!imu)
    {
        rt_kprintf("[RTSpark] ICM20608 initialization failed.\n");
        return;
    }
    rt_kprintf("[RTSpark] ICM20608 ready.\n");
    for (;;)
    {
        rt_tick_t next_sample;
        (void)rtspark_clock(RT_NULL);
        if (!rt_wlan_is_ready())
        {
            rt_tick_t wifi_start;
            rt_kprintf("[RTSpark] Joining configured Wi-Fi...\n");
            rt_wlan_connect(RTSPARK_WIFI_SSID, RTSPARK_WIFI_PASSWORD);
            wifi_start = rt_tick_get();
            while (rt_wlan_is_connected() && !rt_wlan_is_ready() &&
                   (rt_tick_t)(rt_tick_get() - wifi_start) < 30 * RT_TICK_PER_SECOND)
                rt_thread_mdelay(500);
            if (!rt_wlan_is_ready())
            {
                rt_thread_mdelay(5000);
                continue;
            }
        }
        rt_kprintf("[RTSpark] Wi-Fi has an IP address.\n");
        if (!rtspark_clock_ready() && rtspark_clock_sync())
        {
            rt_kprintf("[RTSpark] Waiting for UTC time. Retrying NTP in 15 seconds.\n");
            rt_thread_mdelay(15000);
            continue;
        }
        if (rtspark_mqtt_open())
        {
            rt_thread_mdelay(5000);
            continue;
        }
        next_sample = rt_tick_get();
        while (rt_wlan_is_ready())
        {
            int16_t raw[6];
            char json[320];
            if (icm20608_get_accel(imu, &raw[0], &raw[1], &raw[2]) != RT_EOK ||
                icm20608_get_gyro(imu, &raw[3], &raw[4], &raw[5]) != RT_EOK)
            {
                rt_kprintf("[RTSpark] Sensor read failed.\n");
                break;
            }
            if (rtspark_sensor_json(json, sizeof(json), raw) < 0 || rtspark_mqtt_send(json))
                break;
            /* Keep each console line below RT_CONSOLEBUF_SIZE (128 bytes). */
            rt_kprintf("[RTSpark] PUBACK %lu: activity/sensor\n", ++sent);
            next_sample += RT_TICK_PER_SECOND;
            /* Skip missed deadlines instead of publishing a burst of stale samples. */
            if ((rt_int32_t)(next_sample - rt_tick_get()) <= 0)
                next_sample = rt_tick_get() + RT_TICK_PER_SECOND;
            {
                rt_int32_t remaining = (rt_int32_t)(next_sample - rt_tick_get());
                if (remaining > 0)
                    rt_thread_delay(remaining);
            }
            (void)rtspark_clock(RT_NULL);
        }
        rtspark_mqtt_close();
        rt_thread_mdelay(5000);
    }
}

int main(void)
{
    rt_thread_t worker;
    rt_kprintf("\n[RTSpark] Direct Wi-Fi MQTT TLS publisher v1.0\n");
    worker = rt_thread_create("sensor", publisher, RT_NULL, 12288, 18, 10);
    if (!worker)
    {
        rt_kprintf("[RTSpark] Could not allocate publisher thread.\n");
        return -1;
    }
    return rt_thread_startup(worker);
}
