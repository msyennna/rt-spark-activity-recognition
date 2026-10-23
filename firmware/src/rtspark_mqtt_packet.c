/* Minimal MQTT 3.1.1 client wire contract: authenticated clean CONNECT,
 * non-retained QoS1 PUBLISH, and strict acknowledgements. No heap allocation. */
#include "rtspark_mqtt_packet.h"
#include <string.h>

static size_t variable_size(size_t value)
{
    size_t count = 1;
    while (value >= 128U) { value /= 128U; ++count; }
    return count;
}

static unsigned char *header(unsigned char *out, unsigned char type, size_t value)
{
    *out++ = type;
    do {
        unsigned char digit = value % 128U;
        value /= 128U;
        *out++ = digit | (value ? 0x80U : 0U);
    } while (value);
    return out;
}

static unsigned char *string(unsigned char *out, const char *text, size_t len)
{
    *out++ = (unsigned char)(len >> 8);
    *out++ = (unsigned char)len;
    memcpy(out, text, len);
    return out + len;
}

int rtspark_packet_connect(unsigned char *out, size_t capacity,
                          const char *client, const char *user, const char *password)
{
    static const unsigned char protocol[] = {0,4,'M','Q','T','T',4,0xc2,0,30};
    size_t a, b, c, remaining, total;
    unsigned char *p;
    if (!out || !client || !user || !password) return -1;
    a = strlen(client); b = strlen(user); c = strlen(password);
    if (!a || !b || !c || a > 65535U || b > 65535U || c > 65535U) return -1;
    remaining = sizeof(protocol) + 6U + a + b + c;
    total = 1U + variable_size(remaining) + remaining;
    if (total > capacity) return -1;
    p = header(out, 0x10, remaining);
    memcpy(p, protocol, sizeof(protocol)); p += sizeof(protocol);
    p = string(p, client, a); p = string(p, user, b); p = string(p, password, c);
    return (int)(p - out);
}

int rtspark_packet_publish(unsigned char *out, size_t capacity, uint16_t id,
                          const char *topic, const char *json)
{
    size_t a, b, remaining, total;
    unsigned char *p;
    if (!out || !topic || !json || !id) return -1;
    a = strlen(topic); b = strlen(json);
    if (!a || a > 65535U || b > capacity || strchr(topic, '#') || strchr(topic, '+')) return -1;
    remaining = 4U + a + b;
    total = 1U + variable_size(remaining) + remaining;
    if (total > capacity) return -1;
    p = header(out, 0x32, remaining); /* DUP=0, QoS=1, RETAIN=0. */
    p = string(p, topic, a);
    *p++ = (unsigned char)(id >> 8); *p++ = (unsigned char)id;
    memcpy(p, json, b); p += b;
    return (int)(p - out);
}

int rtspark_packet_connack(const unsigned char reply[4])
{
    /* Clean session requires session-present=0; reject every refusal code. */
    return reply && reply[0] == 0x20 && reply[1] == 2 && reply[2] == 0 && reply[3] == 0;
}

int rtspark_packet_puback(const unsigned char reply[4], uint16_t id)
{
    return reply && id && reply[0] == 0x40 && reply[1] == 2 &&
           reply[2] == (unsigned char)(id >> 8) && reply[3] == (unsigned char)id;
}
