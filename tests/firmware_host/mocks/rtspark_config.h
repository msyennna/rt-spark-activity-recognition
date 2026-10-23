#ifndef MOCK_RTSPARK_CONFIG_H
#define MOCK_RTSPARK_CONFIG_H
/* Deliberately public fake test values. Never include a local private header. */
extern char mock_password[64], mock_client[64];
#define RTSPARK_MQTT_PASSWORD mock_password
#define RTSPARK_CLIENT_ID mock_client
#define RTSPARK_USERNAME "host-test-user"
#define RTSPARK_BROKER "test.invalid"
#define RTSPARK_PORT 8883
#define RTSPARK_TOPIC "activity/sensor"
#endif
