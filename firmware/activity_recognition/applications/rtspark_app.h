#ifndef RTSPARK_APP_H
#define RTSPARK_APP_H

#include <stddef.h>
#include <stdint.h>
#include <rtthread.h>
#include "mbedtls/platform_time.h"

#ifndef RTSPARK_BROKER
#define RTSPARK_BROKER "ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com"
#endif
#ifndef RTSPARK_PORT
#define RTSPARK_PORT "8883"
#endif
#define RTSPARK_TOPIC "activity/sensor"
#define RTSPARK_USERNAME "yena_sensor"

int rtspark_random(unsigned char *out, size_t len);
int rtspark_clock_ready(void);
int rtspark_clock_sync(void);
mbedtls_time_t rtspark_clock(mbedtls_time_t *out);
int rtspark_socket(const char *host, const char *port, int udp);
int rtspark_mqtt_open(void);
int rtspark_mqtt_send(const char *json);
void rtspark_mqtt_close(void);
int rtspark_sensor_json(char *out, size_t capacity, const int16_t raw[6]);

#endif
