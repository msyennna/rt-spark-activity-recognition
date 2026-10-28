#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/rtspark_mqtt.c"

char mock_password[64], mock_client[64];
static uint32_t now;
static int network_ok=1, clock_ok=1, rng_ok=1;
static rtspark_tcp_status tcp_status=RTSPARK_TCP_IDLE;
static int operations, close_count, connect_count, handshake_result, handshake_done;
static int seed_result, parse_result, setup_result, hostname_result, tls_freed;
static uint32_t verify_result;
static int auth_mode, min_version, max_version, hostname_checked, ca_configured, rng_configured;
static int write_result=99999, read_chunk=99999, read_error, read_zero;
static unsigned char received_bytes[8], written_bytes[2048];
static size_t received_size, received_offset, written_size;
static const unsigned char *retry_pointer;
static size_t retry_length;
static char diagnostics[4096];
static size_t diagnostics_size;

uint32_t HAL_GetTick(void) { return now; }
int RW007_Network_Ready(void) { return network_ok; }
int rtspark_clock_ready(void) { return clock_ok; }
int rtspark_random_ready(void) { return rng_ok; }
int rtspark_random(void *context, unsigned char *out, size_t length)
{ (void)context; memset(out,0x55,length); return 0; }
void rtspark_tcp_close(void) { ++close_count; tcp_status=RTSPARK_TCP_IDLE; }
int rtspark_tcp_open(const char *host, uint16_t port)
{ assert(strcmp(host,RTSPARK_BROKER)==0 && port==8883); ++connect_count; tcp_status=RTSPARK_TCP_RESOLVING; return 0; }
rtspark_tcp_status rtspark_tcp_state(void) { return tcp_status; }
int rtspark_tcp_send(void *context, const unsigned char *out, size_t length)
{ (void)context; (void)out; (void)length; return -1; }
int rtspark_tcp_recv(void *context, unsigned char *out, size_t length)
{ (void)context; (void)out; (void)length; return -1; }
void LWIP_Port_Diag(const char *format, ...)
{
    va_list args;
    int length;
    va_start(args,format);
    length=vsnprintf(diagnostics+diagnostics_size,sizeof(diagnostics)-diagnostics_size,format,args);
    va_end(args);
    assert(length>=0 && (size_t)length<sizeof(diagnostics)-diagnostics_size);
    diagnostics_size+=(size_t)length;
}
void mbedtls_memory_buffer_alloc_init(unsigned char *buffer, size_t size)
{ assert(buffer!=NULL && size>=48U*1024U); }
void mbedtls_ecp_set_max_ops(unsigned int ops) { assert(ops>0 && ops<=128); }
void mbedtls_ssl_init(mbedtls_ssl_context *ssl) { ssl->live=1; }
void mbedtls_ssl_free(mbedtls_ssl_context *ssl) { ssl->live=0; ++tls_freed; }
void mbedtls_ssl_config_init(mbedtls_ssl_config *config) { config->auth=0; }
void mbedtls_ssl_config_free(mbedtls_ssl_config *config) { (void)config; }
void mbedtls_x509_crt_init(mbedtls_x509_crt *ca) { ca->live=1; }
void mbedtls_x509_crt_free(mbedtls_x509_crt *ca) { ca->live=0; }
void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *drbg) { drbg->live=1; }
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *drbg) { drbg->live=0; }
int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *drbg,
                         int (*entropy)(void *, unsigned char *, size_t),
                         void *context, const unsigned char *personal, size_t size)
{ (void)drbg; (void)context; assert(entropy==rtspark_random && personal && size); return seed_result; }
int mbedtls_ctr_drbg_random(void *context, unsigned char *out, size_t size)
{ (void)context; memset(out,0x55,size); return 0; }
int mbedtls_x509_crt_parse(mbedtls_x509_crt *ca, const unsigned char *pem, size_t length)
{ (void)ca; assert(pem && length>100 && strstr((const char *)pem,"BEGIN CERTIFICATE")); return parse_result; }
int mbedtls_ssl_config_defaults(mbedtls_ssl_config *config, int endpoint, int transport, int preset)
{ (void)config; assert(endpoint==MBEDTLS_SSL_IS_CLIENT && transport==MBEDTLS_SSL_TRANSPORT_STREAM && preset==MBEDTLS_SSL_PRESET_DEFAULT); return 0; }
void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *config, int mode)
{ config->auth=mode; auth_mode=mode; }
void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *config, mbedtls_x509_crt *ca, void *crl)
{ (void)config; assert(ca && crl==NULL); ca_configured=1; }
void mbedtls_ssl_conf_rng(mbedtls_ssl_config *config,
                         int (*random_fn)(void *, unsigned char *, size_t), void *context)
{ (void)config; assert(random_fn==mbedtls_ctr_drbg_random && context); rng_configured=1; }
void mbedtls_ssl_conf_min_tls_version(mbedtls_ssl_config *config, int version)
{ (void)config; min_version=version; }
void mbedtls_ssl_conf_max_tls_version(mbedtls_ssl_config *config, int version)
{ (void)config; max_version=version; }
int mbedtls_ssl_setup(mbedtls_ssl_context *ssl, mbedtls_ssl_config *config)
{ (void)ssl; assert(config->auth==MBEDTLS_SSL_VERIFY_REQUIRED); return setup_result; }
int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *host)
{ (void)ssl; assert(strcmp(host,RTSPARK_BROKER)==0); hostname_checked=1; return hostname_result; }
void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl, void *context,
                         int (*send_fn)(void *, const unsigned char *, size_t),
                         int (*recv_fn)(void *, unsigned char *, size_t), void *timeout)
{ (void)ssl; assert(context==NULL && send_fn==rtspark_tcp_send && recv_fn==rtspark_tcp_recv && timeout==NULL); }
int mbedtls_ssl_handshake_step(mbedtls_ssl_context *ssl)
{ (void)ssl; ++operations; return handshake_result; }
int mbedtls_ssl_is_handshake_over(mbedtls_ssl_context *ssl) { (void)ssl; return handshake_done; }
uint32_t mbedtls_ssl_get_verify_result(mbedtls_ssl_context *ssl) { (void)ssl; return verify_result; }
const char *mbedtls_ssl_get_version(mbedtls_ssl_context *ssl) { (void)ssl; return "TLSv1.2"; }
int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *bytes, size_t length)
{
    int count;
    (void)ssl; ++operations;
    if (retry_pointer) { assert(bytes==retry_pointer && length==retry_length); retry_pointer=NULL; }
    if (write_result<0) { retry_pointer=bytes; retry_length=length; return write_result; }
    count=length<(size_t)write_result ? (int)length : write_result;
    assert(written_size+(size_t)count<=sizeof(written_bytes));
    memcpy(written_bytes+written_size,bytes,(size_t)count); written_size+=(size_t)count;
    return count;
}
int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *out, size_t length)
{
    size_t count;
    (void)ssl; ++operations;
    if (read_error) return read_error;
    if (read_zero) return 0;
    count=received_size-received_offset;
    if (count>length) count=length;
    if (count>(size_t)read_chunk) count=(size_t)read_chunk;
    if (!count) return MBEDTLS_ERR_SSL_WANT_READ;
    memcpy(out,received_bytes+received_offset,count); received_offset+=count;
    return (int)count;
}

static void poll_once(void)
{ operations=0; rtspark_mqtt_poll(); assert(operations<=1); }

static void reply(const unsigned char *bytes, size_t size)
{ memcpy(received_bytes,bytes,size); received_size=size; received_offset=0; }

static void reset(void)
{
    now=100; network_ok=clock_ok=rng_ok=1;
    tcp_status=RTSPARK_TCP_IDLE;
    operations=close_count=connect_count=handshake_result=handshake_done=0;
    seed_result=parse_result=setup_result=hostname_result=tls_freed=0;
    verify_result=0;
    auth_mode=min_version=max_version=hostname_checked=ca_configured=rng_configured=0;
    write_result=read_chunk=99999; read_error=read_zero=0;
    received_size=received_offset=written_size=0;
    retry_pointer=NULL; retry_length=0;
    diagnostics[0]='\0'; diagnostics_size=0;
    strcpy(mock_password,"public-fake-test-value"); strcpy(mock_client,"host-test-client");
    rtspark_mqtt_init();
}

static void start_handshake(void)
{
    poll_once(); assert(mqtt.phase==WAIT_TCP);
    tcp_status=RTSPARK_TCP_CONNECTED;
    poll_once(); assert(mqtt.phase==TLS_START);
    poll_once(); assert(mqtt.phase==TLS_HANDSHAKE);
    assert(auth_mode==MBEDTLS_SSL_VERIFY_REQUIRED && ca_configured && rng_configured && hostname_checked);
    assert(min_version==MBEDTLS_SSL_VERSION_TLS1_2 && max_version==MBEDTLS_SSL_VERSION_TLS1_2);
}

static void online(void)
{
    static const unsigned char connack[]={0x20,2,0,0};
    start_handshake();
    handshake_done=1; poll_once(); assert(mqtt.phase==CONNECT_SEND);
    poll_once(); assert(mqtt.phase==CONNECT_REPLY);
    for (size_t i=0;i<sizeof(mqtt.packet);++i) assert(mqtt.packet[i]==0);
    reply(connack,sizeof(connack));
    poll_once(); assert(mqtt.phase==ONLINE);
}

static void test_prerequisites_and_tls(void)
{
    reset(); mock_password[0]='\0'; rtspark_mqtt_init();
    poll_once(); assert(mqtt.phase==DISABLED && connect_count==0);
    reset(); network_ok=0; poll_once(); assert(mqtt.phase==WAIT_READY && !connect_count);
    network_ok=1; clock_ok=0; poll_once(); assert(!connect_count);
    clock_ok=1; rng_ok=0; poll_once(); assert(!connect_count);
    reset(); start_handshake();
    handshake_result=MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS;
    for (int i=0;i<4;++i) { poll_once(); assert(mqtt.phase==TLS_HANDSHAKE); }
    handshake_result=0; handshake_done=1; verify_result=1;
    poll_once(); assert(mqtt.phase==BACKOFF && tls_freed==1 && written_size==0);
    reset(); start_handshake();
    now+=TLS_TIMEOUT_MS; poll_once(); assert(mqtt.phase==BACKOFF);
    now+=RETRY_MS; poll_once(); assert(mqtt.phase==WAIT_READY);
    reset();
    poll_once(); tcp_status=RTSPARK_TCP_CONNECTED; poll_once();
    seed_result=-123; poll_once();
    assert(mqtt.phase==BACKOFF && tls_freed==1 && written_size==0);
}

static void test_partial_io_and_delivery(void)
{
    unsigned char ack[]={0x40,2,0,1};
    reset(); online();
    assert(written_bytes[0]==0x10);
    assert(rtspark_mqtt_offer("{\"acceleration_x\":1}")==1);
    assert(rtspark_mqtt_offer("{}") == 0); /* One delivery at a time. */
    write_result=MBEDTLS_ERR_SSL_WANT_WRITE;
    poll_once(); assert(mqtt.phase==PUBLISH_SEND && mqtt.offset==0);
    poll_once(); assert(mqtt.phase==PUBLISH_SEND && mqtt.offset==0);
    write_result=3;
    while (mqtt.phase==PUBLISH_SEND) poll_once();
    assert(mqtt.phase==PUBLISH_REPLY);
    reply(ack,sizeof(ack)); read_chunk=1;
    for (int i=0;i<3;++i) { poll_once(); assert(mqtt.phase==PUBLISH_REPLY && mqtt.deliveries==0); }
    poll_once(); assert(mqtt.phase==ONLINE && mqtt.deliveries==1);
    mqtt.packet_id=0xffff;
    assert(rtspark_mqtt_offer("{}")==1 && mqtt.packet_id==1); /* ID zero skipped. */
    write_result=99999; poll_once();
    ack[3]=2; reply(ack,sizeof(ack)); read_chunk=99999;
    poll_once(); assert(mqtt.phase==BACKOFF && mqtt.deliveries==1);
    assert(rtspark_mqtt_offer("{}")==0); /* No offline queue/replay. */
}

static void test_refusal_timeout_and_ping(void)
{
    static const unsigned char refused[]={0x20,2,0,5}, pingresp[]={0xd0,0};
    reset(); start_handshake(); handshake_done=1;
    poll_once(); poll_once(); reply(refused,sizeof(refused)); poll_once();
    assert(mqtt.phase==BACKOFF);
    reset(); online(); assert(rtspark_mqtt_offer("{}")==1);
    poll_once(); now+=MQTT_TIMEOUT_MS; poll_once();
    assert(mqtt.phase==BACKOFF && mqtt.deliveries==0);
    assert(strstr(diagnostics,"state=PUBACK_WAIT age_ms=5000")!=NULL);
    reset(); online(); now+=PING_INTERVAL_MS; poll_once();
    assert(mqtt.phase==PING_SEND); poll_once(); assert(mqtt.phase==PING_REPLY);
    assert(written_bytes[written_size-2]==0xc0 && written_bytes[written_size-1]==0);
    reply(pingresp,sizeof(pingresp)); poll_once(); assert(mqtt.phase==ONLINE);
    network_ok=0; poll_once(); assert(mqtt.phase==WAIT_READY && close_count>0);
    assert(rtspark_mqtt_offer("{}")==0);
}

static void test_terminal_tls_errors_are_reported(void)
{
    reset(); online(); assert(rtspark_mqtt_offer("{}")==1);
    poll_once(); assert(mqtt.phase==PUBLISH_REPLY);
    read_error=MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    poll_once();
    assert(mqtt.phase==BACKOFF && mqtt.deliveries==0 && tls_freed==1);
    assert(strstr(diagnostics,"MQTT acknowledgement read failed: -0x6C00")!=NULL);

    reset(); online(); assert(rtspark_mqtt_offer("{}")==1);
    write_result=MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    poll_once();
    assert(mqtt.phase==BACKOFF && mqtt.deliveries==0);
    assert(strstr(diagnostics,"MQTT write failed: -0x6C00")!=NULL);

    reset(); online(); assert(rtspark_mqtt_offer("{}")==1);
    poll_once(); read_zero=1; poll_once();
    assert(mqtt.phase==BACKOFF && mqtt.deliveries==0);
    assert(strstr(diagnostics,"MQTT acknowledgement read failed: -0x7280")!=NULL);

    reset(); online(); assert(rtspark_mqtt_offer("{}")==1);
    write_result=0; poll_once();
    assert(mqtt.phase==BACKOFF && mqtt.deliveries==0);
    assert(strstr(diagnostics,"MQTT write failed: -0x7280")!=NULL);
    assert(strstr(diagnostics,"public-fake-test-value")==NULL);
}

static void test_busy_sample_report_is_bounded(void)
{
    static const unsigned char ack[]={0x40,2,0,1};
    unsigned char packet_copy[768];
    size_t first_report_size;
    reset(); online(); assert(rtspark_mqtt_offer("{}")==1);
    poll_once(); assert(mqtt.phase==PUBLISH_REPLY);
    memcpy(packet_copy,mqtt.packet,sizeof(packet_copy));
    now+=1000U;
    assert(rtspark_mqtt_offer("{\"acceleration_x\":1}")==0);
    assert(strstr(diagnostics,"sample skipped: state=PUBACK_WAIT age_ms=1000 busy_offers=1")!=NULL);
    first_report_size=diagnostics_size;
    now+=1000U;
    assert(rtspark_mqtt_offer("{\"acceleration_x\":2}")==0);
    assert(diagnostics_size==first_report_size); /* One report per busy interval. */
    assert(memcmp(packet_copy,mqtt.packet,sizeof(packet_copy))==0);
    reply(ack,sizeof(ack)); poll_once(); assert(mqtt.phase==ONLINE && mqtt.deliveries==1);
    assert(rtspark_mqtt_offer("{}")==1); poll_once();
    now+=1000U;
    assert(rtspark_mqtt_offer("{}")==0);
    assert(strstr(diagnostics,"sample skipped: state=PUBACK_WAIT age_ms=1000 busy_offers=3")!=NULL);
    assert(strstr(diagnostics,"acceleration_x")==NULL); /* Samples are not logged by this diagnostic. */
}

int main(void)
{
    test_prerequisites_and_tls();
    test_partial_io_and_delivery();
    test_refusal_timeout_and_ping();
    test_terminal_tls_errors_are_reported();
    test_busy_sample_report_is_bounded();
    puts("PASS: actual nonblocking MQTT state, TLS verification gates, partial I/O, ACK/refusal/timeouts and ping");
    return 0;
}
