/* Same units as mqtt_sensor_publisher.py: acceleration in g, gyro in rad/s. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

static void decimal6(char out[20], int32_t value)
{
    uint32_t magnitude = value < 0 ? (uint32_t)(-(int64_t)value) : (uint32_t)value;
    snprintf(out, 20, "%s%lu.%06lu", value < 0 ? "-" : "",
             (unsigned long)(magnitude / 1000000),
             (unsigned long)(magnitude % 1000000));
}

int rtspark_sensor_json(char *out, size_t capacity, const int16_t raw[6])
{
    char values[6][20];
    int i;
    int length;
    for (i = 0; i < 6; ++i)
    {
        /* Signed rounding to the nearest micro-unit; no floating printf needed. */
        int64_t numerator = i < 3 ? (int64_t)raw[i] * 1000000 :
                                   (int64_t)raw[i] * 133231240610LL;
        int64_t denominator = i < 3 ? 16384 : 1000000000LL;
        numerator += numerator < 0 ? -(denominator / 2) : denominator / 2;
        decimal6(values[i], (int32_t)(numerator / denominator));
    }
    length = snprintf(out, capacity,
        "{\"acceleration_x\":%s,\"acceleration_y\":%s,\"acceleration_z\":%s,"
        "\"gyro_x\":%s,\"gyro_y\":%s,\"gyro_z\":%s}",
        values[0], values[1], values[2], values[3], values[4], values[5]);
    return length >= 0 && (size_t)length < capacity ? length : -1;
}
