#ifndef RTSPARK_CONFIG_H
#define RTSPARK_CONFIG_H

/* Public endpoints and contract from the working cloud/Android baseline. */
#define RTSPARK_BROKER "ec2-15-134-214-242.ap-southeast-2.compute.amazonaws.com"
#define RTSPARK_PORT 8883U
#define RTSPARK_TOPIC "activity/sensor"
#define RTSPARK_USERNAME "yena_sensor"

#if defined(__has_include)
#if __has_include("rtspark_private.h")
#include "rtspark_private.h"
#endif
#endif
#ifndef RTSPARK_MQTT_PASSWORD
#define RTSPARK_MQTT_PASSWORD ""
#endif
#ifndef RTSPARK_CLIENT_ID
#define RTSPARK_CLIENT_ID ""
#endif

#endif
