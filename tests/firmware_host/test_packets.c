#include "rtspark_mqtt_packet.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static size_t remaining(const unsigned char *packet, size_t *offset)
{
    size_t value = 0, multiplier = 1;
    unsigned char digit;
    *offset = 1;
    do {
        digit = packet[(*offset)++];
        value += (digit & 127U) * multiplier;
        multiplier *= 128U;
    } while (digit & 128U);
    return value;
}

int main(void)
{
    static const unsigned char expected_connect[] = {
        0x10, 19, 0,4,'M','Q','T','T',4,0xc2,0,30,
        0,1,'c', 0,1,'u', 0,1,'p'
    };
    unsigned char out[2048], copy[2048];
    unsigned char connack[] = {0x20,2,0,0};
    unsigned char puback[] = {0x40,2,0x12,0x34};
    char json[300];
    size_t offset, body;
    int length;

    length = rtspark_packet_connect(out, sizeof(out), "c", "u", "p");
    assert(length == (int)sizeof(expected_connect));
    assert(memcmp(out, expected_connect, sizeof(expected_connect)) == 0);
    memset(out, 0xa5, sizeof(out));
    memcpy(copy, out, sizeof(out));
    assert(rtspark_packet_connect(out, sizeof(expected_connect)-1, "c", "u", "p") == -1);
    assert(memcmp(out, copy, sizeof(out)) == 0);
    assert(rtspark_packet_connect(out, sizeof(out), "", "u", "p") == -1);
    assert(rtspark_packet_connect(NULL, sizeof(out), "c", "u", "p") == -1);

    memset(json, 'x', sizeof(json)-1);
    json[sizeof(json)-1] = '\0';
    length = rtspark_packet_publish(out, sizeof(out), 0x1234, "activity/sensor", json);
    assert(length > 0 && out[0] == 0x32); /* QoS 1, no retain, no DUP. */
    body = remaining(out, &offset);
    assert(body + offset == (size_t)length && offset == 3);
    assert(out[offset] == 0 && out[offset+1] == strlen("activity/sensor"));
    offset += 2;
    assert(memcmp(out+offset, "activity/sensor", strlen("activity/sensor")) == 0);
    offset += strlen("activity/sensor");
    assert(out[offset++] == 0x12 && out[offset++] == 0x34);
    assert(memcmp(out+offset, json, strlen(json)) == 0);
    assert(rtspark_packet_publish(out, length, 0xffff, "activity/sensor", json) == length);
    memset(out, 0xa5, sizeof(out));
    memcpy(copy, out, sizeof(out));
    assert(rtspark_packet_publish(out, length-1, 1, "activity/sensor", json) == -1);
    assert(memcmp(out, copy, sizeof(out)) == 0);
    assert(rtspark_packet_publish(out, sizeof(out), 0, "activity/sensor", "{}") == -1);
    assert(rtspark_packet_publish(out, sizeof(out), 1, "activity/+", "{}") == -1);
    assert(rtspark_packet_publish(out, sizeof(out), 1, "activity/#", "{}") == -1);

    assert(rtspark_packet_connack(connack));
    for (unsigned int i = 0; i < sizeof(connack); ++i) {
        connack[i] ^= 1U;
        assert(!rtspark_packet_connack(connack));
        connack[i] ^= 1U;
    }
    assert(!rtspark_packet_connack(NULL));
    assert(rtspark_packet_puback(puback, 0x1234));
    assert(!rtspark_packet_puback(puback, 0x1235));
    assert(!rtspark_packet_puback(puback, 0));
    assert(!rtspark_packet_puback(NULL, 0x1234));
    for (unsigned int i = 0; i < sizeof(puback); ++i) {
        puback[i] ^= 1U;
        assert(!rtspark_packet_puback(puback, 0x1234));
        puback[i] ^= 1U;
    }
    assert(rtspark_packet_ping(out, sizeof(out)) == 2);
    assert(out[0] == 0xc0 && out[1] == 0);
    assert(rtspark_packet_ping(out, 1) == -1);
    assert(rtspark_packet_pingresp((const unsigned char[]){0xd0,0}));
    assert(!rtspark_packet_pingresp((const unsigned char[]){0xd1,0}));
    assert(!rtspark_packet_pingresp((const unsigned char[]){0xd0,1}));
    puts("PASS: actual MQTT CONNECT/PUBLISH encoding, bounds and strict acknowledgements");
    return 0;
}
