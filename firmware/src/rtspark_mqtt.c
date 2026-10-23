#include "rtspark_mqtt.h"
#include "rtspark_mqtt_packet.h"
#include "rtspark_config.h"
#include "rtspark_ca.h"
#include "rtspark_platform.h"
#include "rw007_netif.h"
#include "main.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include <string.h>

void LWIP_Port_Diag(const char *format, ...);

#define MQTT_TIMEOUT_MS 5000U
#define TLS_TIMEOUT_MS 45000U
#define RETRY_MS 5000U
#define PING_INTERVAL_MS 15000U

typedef enum {
    WAIT_READY, WAIT_TCP, TLS_START, TLS_HANDSHAKE,
    CONNECT_SEND, CONNECT_REPLY, ONLINE, PUBLISH_SEND, PUBLISH_REPLY,
    PING_SEND, PING_REPLY, BACKOFF, DISABLED
} mqtt_phase;

static struct {
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt ca;
    mbedtls_ctr_drbg_context drbg;
    unsigned char packet[768];
    unsigned char reply[4];
    size_t length;
    size_t offset;
    uint16_t packet_id;
    uint32_t started;
    uint32_t last_send;
    uint32_t deliveries;
    int tls_initialized;
    mqtt_phase phase;
} mqtt;

/* Includes all TLS heap use in the linker's RAM accounting. No TLS allocation
 * can exhaust the foreground stack or spill into lwIP's separate heap. */
static union {
    uint64_t alignment;
    unsigned char bytes[56U * 1024U];
} tls_heap __attribute__((section(".ccmram")));

static int retryable(int rc)
{
    return rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE ||
           rc == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS;
}

static void phase(mqtt_phase next)
{
    mqtt.phase = next;
    mqtt.started = HAL_GetTick();
    mqtt.offset = 0;
}

static void close_connection(void)
{
    /* No blocking close_notify on a failed link. The peer observes TCP reset. */
    rtspark_tcp_close();
    if (mqtt.tls_initialized) {
        mbedtls_ssl_free(&mqtt.ssl);
        mbedtls_ssl_config_free(&mqtt.config);
        mbedtls_x509_crt_free(&mqtt.ca);
        mbedtls_ctr_drbg_free(&mqtt.drbg);
        mqtt.tls_initialized = 0;
    }
    /* CONNECT contains the password. Retain it only for an in-progress write. */
    memset(mqtt.packet, 0, sizeof(mqtt.packet));
    mqtt.length = 0;
}

static void fail(const char *step, int error)
{
    LWIP_Port_Diag("[RTSpark] %s failed: -0x%04X; retry in 5s\r\n",
                   step, (unsigned int)(error < 0 ? -error : error));
    close_connection();
    phase(BACKOFF);
}

void rtspark_mqtt_init(void)
{
    memset(&mqtt, 0, sizeof(mqtt));
    mbedtls_memory_buffer_alloc_init(tls_heap.bytes, sizeof(tls_heap.bytes));
    mbedtls_ecp_set_max_ops(128);
    if (!RTSPARK_MQTT_PASSWORD[0] || !RTSPARK_CLIENT_ID[0]) {
        phase(DISABLED);
        LWIP_Port_Diag("[RTSpark] MQTT disabled: configure ignored include/rtspark_private.h\r\n");
    } else {
        phase(WAIT_READY);
        LWIP_Port_Diag("[RTSpark] Waiting for DHCP, hardware RNG and UTC before TLS\r\n");
    }
}

static int tls_start(void)
{
    static const unsigned char personal[] = "rtspark-stm32cubemx";
    int rc;
    mqtt.tls_initialized = 1;
    mbedtls_ssl_init(&mqtt.ssl);
    mbedtls_ssl_config_init(&mqtt.config);
    mbedtls_x509_crt_init(&mqtt.ca);
    mbedtls_ctr_drbg_init(&mqtt.drbg);
    rc = mbedtls_ctr_drbg_seed(&mqtt.drbg, rtspark_random, NULL, personal, sizeof(personal) - 1);
    if (rc) return rc;
    rc = mbedtls_x509_crt_parse(&mqtt.ca, (const unsigned char *)rtspark_ca, sizeof(rtspark_ca));
    if (rc) return rc;
    rc = mbedtls_ssl_config_defaults(&mqtt.config, MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc) return rc;
    mbedtls_ssl_conf_authmode(&mqtt.config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&mqtt.config, &mqtt.ca, NULL);
    mbedtls_ssl_conf_rng(&mqtt.config, mbedtls_ctr_drbg_random, &mqtt.drbg);
    mbedtls_ssl_conf_min_tls_version(&mqtt.config, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&mqtt.config, MBEDTLS_SSL_VERSION_TLS1_2);
    rc = mbedtls_ssl_setup(&mqtt.ssl, &mqtt.config);
    if (rc) return rc;
    rc = mbedtls_ssl_set_hostname(&mqtt.ssl, RTSPARK_BROKER);
    if (rc) return rc;
    mbedtls_ssl_set_bio(&mqtt.ssl, NULL, rtspark_tcp_send, rtspark_tcp_recv, NULL);
    return 0;
}

/* A single TLS operation per poll. WANT_* preserves the same buffer/length on
 * retry, as required by ssl_write; the main loop continues servicing SPI/lwIP. */
static int write_step(void)
{
    int rc = mbedtls_ssl_write(&mqtt.ssl, mqtt.packet + mqtt.offset, mqtt.length - mqtt.offset);
    if (rc > 0) mqtt.offset += (size_t)rc;
    else if (!retryable(rc)) return -1;
    if (mqtt.offset == mqtt.length) {
        mqtt.last_send = HAL_GetTick();
        return 1;
    }
    return 0;
}

static int read_step(size_t size)
{
    int rc = mbedtls_ssl_read(&mqtt.ssl, mqtt.reply + mqtt.offset, size - mqtt.offset);
    if (rc > 0) mqtt.offset += (size_t)rc;
    else if (!retryable(rc)) return -1;
    return mqtt.offset == size ? 1 : 0;
}

int rtspark_mqtt_offer(const char *json)
{
    int length;
    uint16_t id;
    if (mqtt.phase != ONLINE || !json) return 0;
    id = (uint16_t)(mqtt.packet_id + 1U);
    if (!id) id = 1U;
    length = rtspark_packet_publish(mqtt.packet, sizeof(mqtt.packet), id, RTSPARK_TOPIC, json);
    if (length < 0) return 0;
    mqtt.packet_id = id;
    mqtt.length = (size_t)length;
    phase(PUBLISH_SEND);
    return 1;
}

void rtspark_mqtt_poll(void)
{
    uint32_t now = HAL_GetTick();
    int rc;
    if (mqtt.phase == DISABLED) return;
    if (mqtt.phase == BACKOFF) {
        if ((uint32_t)(now - mqtt.started) >= RETRY_MS) phase(WAIT_READY);
        return;
    }
    if (!RW007_Network_Ready() || !rtspark_clock_ready() || !rtspark_random_ready()) {
        if (mqtt.phase != WAIT_READY) {
            close_connection();
            phase(WAIT_READY);
        }
        return;
    }
    if (mqtt.phase != WAIT_READY && mqtt.phase != WAIT_TCP &&
        rtspark_tcp_state() != RTSPARK_TCP_CONNECTED) {
        fail("TCP/link", -1);
        return;
    }
    if ((mqtt.phase == TLS_HANDSHAKE && (uint32_t)(now - mqtt.started) >= TLS_TIMEOUT_MS) ||
        (mqtt.phase >= CONNECT_SEND && mqtt.phase <= PING_REPLY && mqtt.phase != ONLINE &&
         (uint32_t)(now - mqtt.started) >= MQTT_TIMEOUT_MS)) {
        fail("TLS/MQTT timeout", -1);
        return;
    }
    switch (mqtt.phase) {
    case WAIT_READY:
        if (rtspark_tcp_open(RTSPARK_BROKER, RTSPARK_PORT) != 0) fail("DNS/TCP start", -1);
        else phase(WAIT_TCP);
        break;
    case WAIT_TCP:
        if (rtspark_tcp_state() == RTSPARK_TCP_CONNECTED) {
            LWIP_Port_Diag("[RTSpark] DNS/TCP connected; UTC ready\r\n");
            phase(TLS_START);
        } else if (rtspark_tcp_state() == RTSPARK_TCP_FAILED) fail("DNS/TCP", -1);
        break;
    case TLS_START:
        rc = tls_start();
        if (rc) fail("TLS setup/entropy", rc);
        else phase(TLS_HANDSHAKE);
        break;
    case TLS_HANDSHAKE:
        rc = mbedtls_ssl_handshake_step(&mqtt.ssl);
        if (rc && !retryable(rc)) { fail("TLS verification/handshake", rc); break; }
        if (mbedtls_ssl_is_handshake_over(&mqtt.ssl)) {
            if (mbedtls_ssl_get_verify_result(&mqtt.ssl) != 0) { fail("TLS certificate", -1); break; }
            LWIP_Port_Diag("[RTSpark] TLS verified: %s\r\n", mbedtls_ssl_get_version(&mqtt.ssl));
            rc = rtspark_packet_connect(mqtt.packet, sizeof(mqtt.packet), RTSPARK_CLIENT_ID,
                                        RTSPARK_USERNAME, RTSPARK_MQTT_PASSWORD);
            if (rc < 0) { fail("MQTT CONNECT size", rc); break; }
            mqtt.length = (size_t)rc;
            phase(CONNECT_SEND);
        }
        break;
    case CONNECT_SEND:
    case PUBLISH_SEND:
    case PING_SEND:
        rc = write_step();
        if (rc < 0) fail("MQTT write", rc);
        else if (rc > 0) {
            mqtt_phase reply = mqtt.phase == CONNECT_SEND ? CONNECT_REPLY :
                               mqtt.phase == PUBLISH_SEND ? PUBLISH_REPLY : PING_REPLY;
            memset(mqtt.packet, 0, sizeof(mqtt.packet));
            phase(reply);
        }
        break;
    case CONNECT_REPLY:
    case PUBLISH_REPLY:
    case PING_REPLY:
        rc = read_step(mqtt.phase == PING_REPLY ? 2U : 4U);
        if (rc < 0) { fail("MQTT acknowledgement read", rc); break; }
        if (rc == 0) break;
        if (mqtt.phase == CONNECT_REPLY) {
            if (!rtspark_packet_connack(mqtt.reply)) { fail("MQTT CONNACK refused/invalid", -1); break; }
            LWIP_Port_Diag("[RTSpark] MQTT CONNACK accepted; topic activity/sensor QoS1\r\n");
        } else if (mqtt.phase == PUBLISH_REPLY) {
            if (!rtspark_packet_puback(mqtt.reply, mqtt.packet_id)) { fail("MQTT PUBACK mismatch", -1); break; }
            ++mqtt.deliveries;
            LWIP_Port_Diag("[RTSpark] MQTT PUBACK id=%u count=%lu\r\n",
                           (unsigned int)mqtt.packet_id, (unsigned long)mqtt.deliveries);
        } else if (mqtt.reply[0] != 0xd0 || mqtt.reply[1] != 0) {
            fail("MQTT PINGRESP invalid", -1);
            break;
        }
        phase(ONLINE);
        break;
    case ONLINE:
        if ((uint32_t)(now - mqtt.last_send) >= PING_INTERVAL_MS) {
            mqtt.packet[0] = 0xc0; mqtt.packet[1] = 0;
            mqtt.length = 2;
            phase(PING_SEND);
        }
        break;
    default: break;
    }
}
