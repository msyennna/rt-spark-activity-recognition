/* One worker owns each TLS connection; Paho MQTTPacket encodes MQTT 3.1.1. */
#include "rtspark_app.h"
#include "rtspark_private.h"
#include "rtspark_ca.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include "MQTTPacket.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/x509_crt.h"

#if !defined(MBEDTLS_HAVE_TIME_DATE) || !defined(MBEDTLS_PLATFORM_TIME_ALT)
#error Run install_rtspark.py to enable certificate dates and the board UTC clock.
#endif

static struct
{
    int fd;
    int initialized;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt ca;
    mbedtls_ctr_drbg_context drbg;
    unsigned char packet[768];
    unsigned short packet_id;
} connection;

static int seed_entropy(void *context, unsigned char *out, size_t len)
{
    (void)context;
    return rtspark_random(out, len) == 0 ? 0 : MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
}

static int socket_send(void *ctx, const unsigned char *data, size_t size)
{
    int rc = send(*(int *)ctx, data, size, 0);
    if (rc >= 0)
        return rc;
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

static int socket_recv(void *ctx, unsigned char *data, size_t size)
{
    int rc = recv(*(int *)ctx, data, size, 0);
    if (rc >= 0)
        return rc;
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        return MBEDTLS_ERR_SSL_WANT_READ;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

static int retryable(int rc)
{
    return rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE;
}

static int transfer(int writing, unsigned char *data, size_t size)
{
    size_t done = 0;
    rt_tick_t start = rt_tick_get();
    while (done < size)
    {
        int rc = writing ? mbedtls_ssl_write(&connection.ssl, data + done, size - done) :
                           mbedtls_ssl_read(&connection.ssl, data + done, size - done);
        if (rc > 0)
            done += rc;
        else if (!retryable(rc))
            return -1;
        if ((rt_tick_t)(rt_tick_get() - start) >= 5 * RT_TICK_PER_SECOND)
            return -1;
        if (retryable(rc))
            rt_thread_mdelay(10);
    }
    return 0;
}

void rtspark_mqtt_close(void)
{
    if (!connection.initialized)
        return;
    /* Close the transport first: cleanup never waits on a broken Wi-Fi link. */
    if (connection.fd >= 0)
        closesocket(connection.fd);
    mbedtls_ssl_free(&connection.ssl);
    mbedtls_ssl_config_free(&connection.config);
    mbedtls_x509_crt_free(&connection.ca);
    mbedtls_ctr_drbg_free(&connection.drbg);
    memset(&connection, 0, sizeof(connection));
}

int rtspark_mqtt_open(void)
{
    static const unsigned char personal[] = "rtspark-sensor-tls";
    MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
    unsigned char ack[4];
    unsigned char session_present, return_code;
    int rc, length;
    rt_tick_t start;
    rtspark_mqtt_close();
    if (!rtspark_clock_ready())
        return -1;
    connection.initialized = 1;
    connection.fd = -1;
    mbedtls_ssl_init(&connection.ssl);
    mbedtls_ssl_config_init(&connection.config);
    mbedtls_x509_crt_init(&connection.ca);
    mbedtls_ctr_drbg_init(&connection.drbg);
    mbedtls_platform_set_time(rtspark_clock);

    rc = mbedtls_ctr_drbg_seed(&connection.drbg, seed_entropy,
                              RT_NULL, personal, sizeof(personal) - 1);
    if (rc)
        goto failed;
    rc = mbedtls_x509_crt_parse(&connection.ca, (const unsigned char *)rtspark_ca,
                               sizeof(rtspark_ca));
    if (rc)
        goto failed;
    rc = mbedtls_ssl_config_defaults(&connection.config, MBEDTLS_SSL_IS_CLIENT,
                                     MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (rc)
        goto failed;
    mbedtls_ssl_conf_authmode(&connection.config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&connection.config, &connection.ca, RT_NULL);
    mbedtls_ssl_conf_rng(&connection.config, mbedtls_ctr_drbg_random, &connection.drbg);
    mbedtls_ssl_conf_min_version(&connection.config, MBEDTLS_SSL_MAJOR_VERSION_3,
                                 MBEDTLS_SSL_MINOR_VERSION_3); /* TLS 1.2 */
    rc = mbedtls_ssl_setup(&connection.ssl, &connection.config);
    if (rc)
        goto failed;
    rc = mbedtls_ssl_set_hostname(&connection.ssl, RTSPARK_BROKER);
    if (rc)
        goto failed;
    connection.fd = rtspark_socket(RTSPARK_BROKER, RTSPARK_PORT, 0);
    if (connection.fd < 0)
    {
        rt_kprintf("[RTSpark] Broker TCP connection failed. Check port 8883 and AWS source IP.\n");
        rtspark_mqtt_close();
        return -1;
    }
    mbedtls_ssl_set_bio(&connection.ssl, &connection.fd, socket_send, socket_recv, RT_NULL);
    start = rt_tick_get();
    while ((rc = mbedtls_ssl_handshake(&connection.ssl)) != 0)
    {
        if (!retryable(rc) || (rt_tick_t)(rt_tick_get() - start) >= 30 * RT_TICK_PER_SECOND)
            goto failed;
        rt_thread_mdelay(10);
    }
    if (mbedtls_ssl_get_verify_result(&connection.ssl) != 0)
    {
        rc = MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
        goto failed;
    }
    rt_kprintf("[RTSpark] TLS verified: %s.\n", mbedtls_ssl_get_version(&connection.ssl));
    options.clientID.cstring = RTSPARK_CLIENT_ID;
    options.username.cstring = RTSPARK_USERNAME;
    options.password.cstring = RTSPARK_MQTT_PASSWORD;
    options.keepAliveInterval = 30;
    options.cleansession = 1;
    options.willFlag = 0;
    length = MQTTSerialize_connect(connection.packet, sizeof(connection.packet), &options);
    if (length <= 0 || transfer(1, connection.packet, length) || transfer(0, ack, sizeof(ack)) ||
        ack[0] != 0x20 || ack[1] != 2 ||
        MQTTDeserialize_connack(&session_present, &return_code, ack, sizeof(ack)) != 1)
    {
        rt_kprintf("[RTSpark] MQTT connection reply missing or invalid.\n");
        goto mqtt_failed;
    }
    if (return_code != 0 || session_present != 0)
    {
        rt_kprintf("[RTSpark] MQTT rejected: code=%u (4/5: check username and password).\n", return_code);
        goto mqtt_failed;
    }
    rt_kprintf("[RTSpark] MQTT connected. Publishing %s once per second.\n", RTSPARK_TOPIC);
    return 0;
failed:
    rt_kprintf("[RTSpark] TLS error: -0x%04x. Check CA, UTC time, and free memory.\n", (unsigned)-rc);
mqtt_failed:
    rtspark_mqtt_close();
    return -1;
}

int rtspark_mqtt_send(const char *json)
{
    MQTTString topic = MQTTString_initializer;
    unsigned char ack[4];
    unsigned short packet_id;
    int length;
    if (!connection.initialized || connection.fd < 0)
        return -1;
    if (++connection.packet_id == 0)
        ++connection.packet_id;
    packet_id = connection.packet_id;
    topic.cstring = RTSPARK_TOPIC;
    length = MQTTSerialize_publish(connection.packet, sizeof(connection.packet), 0, 1, 0,
                                    packet_id, topic, (unsigned char *)json, strlen(json));
    if (length <= 0 || transfer(1, connection.packet, length) || transfer(0, ack, sizeof(ack)) ||
        ack[0] != 0x40 || ack[1] != 2 || ack[2] != (packet_id >> 8) ||
        ack[3] != (packet_id & 255))
    {
        rt_kprintf("[RTSpark] Publish was not acknowledged; reconnecting.\n");
        return -1;
    }
    return 0;
}
