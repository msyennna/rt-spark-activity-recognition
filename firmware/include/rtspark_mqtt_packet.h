#ifndef RTSPARK_MQTT_PACKET_H
#define RTSPARK_MQTT_PACKET_H
#include <stddef.h>
#include <stdint.h>
int rtspark_packet_connect(unsigned char *out, size_t capacity,
                          const char *client, const char *user, const char *password);
int rtspark_packet_publish(unsigned char *out, size_t capacity, uint16_t id,
                          const char *topic, const char *json);
int rtspark_packet_connack(const unsigned char reply[4]);
int rtspark_packet_puback(const unsigned char reply[4], uint16_t id);
#endif
