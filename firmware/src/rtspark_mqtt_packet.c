/* AWS coreMQTT serializer adapter for the foreground TLS transport. */
#include "rtspark_mqtt_packet.h"
#include "core_mqtt_serializer.h"
#include <limits.h>
#include <string.h>

int rtspark_packet_connect(unsigned char *out, size_t capacity,
                          const char *client, const char *user, const char *password)
{
    MQTTConnectInfo_t info = {0};
    MQTTFixedBuffer_t buffer;
    size_t a, b, c, remaining, total;
    if (!out || !client || !user || !password) return -1;
    a = strlen(client); b = strlen(user); c = strlen(password);
    if (!a || a > 65535U || b > 65535U || c > 65535U) return -1;
    info.cleanSession = true;
    info.keepAliveSeconds = 30;
    info.pClientIdentifier = client;
    info.clientIdentifierLength = (uint16_t)a;
    info.pUserName = user;
    info.userNameLength = (uint16_t)b;
    info.pPassword = password;
    info.passwordLength = (uint16_t)c;
    if (MQTT_GetConnectPacketSize(&info, NULL, &remaining, &total) != MQTTSuccess ||
        total > capacity || total > INT_MAX) return -1;
    buffer.pBuffer = out; buffer.size = capacity;
    if (MQTT_SerializeConnect(&info, NULL, remaining, &buffer) != MQTTSuccess) return -1;
    return (int)total;
}

int rtspark_packet_publish(unsigned char *out, size_t capacity, uint16_t id,
                          const char *topic, const char *json)
{
    MQTTPublishInfo_t info = {0};
    MQTTFixedBuffer_t buffer;
    size_t topic_size, remaining, total;
    if (!out || !topic || !json || !id) return -1;
    topic_size = strlen(topic);
    if (!topic_size || topic_size > 65535U || strchr(topic, '#') || strchr(topic, '+')) return -1;
    info.qos = MQTTQoS1;
    info.pTopicName = topic;
    info.topicNameLength = (uint16_t)topic_size;
    info.pPayload = json;
    info.payloadLength = strlen(json);
    if (MQTT_GetPublishPacketSize(&info, &remaining, &total) != MQTTSuccess ||
        total > capacity || total > INT_MAX) return -1;
    buffer.pBuffer = out; buffer.size = capacity;
    if (MQTT_SerializePublish(&info, id, remaining, &buffer) != MQTTSuccess) return -1;
    return (int)total;
}

int rtspark_packet_connack(const unsigned char reply[4])
{
    MQTTPacketInfo_t packet = {0};
    uint8_t remaining[2];
    uint16_t id = 0;
    bool present = false;
    if (!reply || reply[0] != MQTT_PACKET_TYPE_CONNACK || reply[1] != 2) return 0;
    remaining[0] = reply[2]; remaining[1] = reply[3];
    packet.type = reply[0]; packet.remainingLength = 2;
    packet.pRemainingData = remaining; packet.headerLength = 2;
    return MQTT_DeserializeAck(&packet, &id, &present) == MQTTSuccess && !present;
}

int rtspark_packet_puback(const unsigned char reply[4], uint16_t expected)
{
    MQTTPacketInfo_t packet = {0};
    uint8_t remaining[2];
    uint16_t received = 0;
    bool present = false;
    if (!reply || !expected || reply[0] != MQTT_PACKET_TYPE_PUBACK || reply[1] != 2) return 0;
    remaining[0] = reply[2]; remaining[1] = reply[3];
    packet.type = reply[0]; packet.remainingLength = 2;
    packet.pRemainingData = remaining; packet.headerLength = 2;
    return MQTT_DeserializeAck(&packet, &received, &present) == MQTTSuccess &&
           received == expected;
}

int rtspark_packet_ping(unsigned char *out, size_t capacity)
{
    MQTTFixedBuffer_t buffer;
    if (!out || capacity < 2) return -1;
    buffer.pBuffer = out; buffer.size = capacity;
    return MQTT_SerializePingreq(&buffer) == MQTTSuccess ? 2 : -1;
}

int rtspark_packet_pingresp(const unsigned char reply[2])
{
    MQTTPacketInfo_t packet = {0};
    uint16_t id = 0;
    bool present = false;
    if (!reply || reply[0] != MQTT_PACKET_TYPE_PINGRESP || reply[1] != 0) return 0;
    packet.type = reply[0]; packet.remainingLength = 0; packet.headerLength = 2;
    return MQTT_DeserializeAck(&packet, &id, &present) == MQTTSuccess;
}
