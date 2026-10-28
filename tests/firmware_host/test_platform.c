#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/src/rtspark_platform.c"

MockRng mock_rng;
MockRcc mock_rcc;
static uint32_t mock_tick, tick_increment;
static int network_ready = 1, aborts, freed, received, written;
static uint16_t send_capacity = 1460;
static err_t dns_result = ERR_INPROGRESS, write_result = ERR_OK, output_result = ERR_OK;
static dns_found_callback pending_dns;
static void *pending_dns_context;
static struct tcp_pcb pcb_instance;
static struct udp_pcb udp_instance;

uint32_t HAL_GetTick(void) { uint32_t now = mock_tick; mock_tick += tick_increment; return now; }
uint32_t HAL_RCC_GetSysClockFreq(void) { return 168000000U; }
int RW007_Network_Ready(void) { return network_ready; }
err_t dns_gethostbyname(const char *host, ip_addr_t *out, dns_found_callback callback, void *context)
{ (void)host; out->value=42; pending_dns=callback; pending_dns_context=context; return dns_result; }
struct tcp_pcb *tcp_new(void) { pcb_instance.live=1; return &pcb_instance; }
void tcp_arg(struct tcp_pcb *pcb, void *arg) { (void)pcb; (void)arg; }
void tcp_recv(struct tcp_pcb *pcb, tcp_receive_callback callback) { (void)pcb; (void)callback; }
void tcp_sent(struct tcp_pcb *pcb, void *callback) { (void)pcb; (void)callback; }
void tcp_err(struct tcp_pcb *pcb, tcp_error_callback callback) { (void)pcb; (void)callback; }
void tcp_abort(struct tcp_pcb *pcb) { pcb->live=0; ++aborts; }
err_t tcp_connect(struct tcp_pcb *pcb, const ip_addr_t *address, uint16_t port, tcp_connected_callback callback)
{ (void)pcb; (void)address; (void)port; (void)callback; return ERR_OK; }
uint16_t tcp_sndbuf(struct tcp_pcb *pcb) { (void)pcb; return send_capacity; }
err_t tcp_write(struct tcp_pcb *pcb, const void *bytes, uint16_t length, int flags)
{ (void)pcb; (void)bytes; assert(flags == TCP_WRITE_FLAG_COPY); if (write_result==ERR_OK) written+=length; return write_result; }
err_t tcp_output(struct tcp_pcb *pcb) { (void)pcb; return output_result; }
void tcp_recved(struct tcp_pcb *pcb, uint16_t length) { (void)pcb; received+=length; }
uint16_t pbuf_copy_partial(const struct pbuf *p, void *out, uint16_t length, uint16_t offset)
{ assert(length+offset<=p->tot_len); memcpy(out,(const unsigned char *)p->payload+offset,length); return length; }
uint16_t pbuf_free(struct pbuf *p) { (void)p; ++freed; return 1; }
struct pbuf *pbuf_alloc(int layer, uint16_t length, int type)
{ (void)layer; (void)length; (void)type; return NULL; }
err_t pbuf_take(struct pbuf *p, const void *bytes, uint16_t length)
{ memcpy(p->payload,bytes,length); return ERR_OK; }
struct udp_pcb *udp_new(void) { udp_instance.live=1; return &udp_instance; }
void udp_remove(struct udp_pcb *pcb) { pcb->live=0; }
err_t udp_bind(struct udp_pcb *pcb, const ip_addr_t *address, uint16_t port)
{ (void)pcb; (void)address; (void)port; return ERR_OK; }
void udp_recv(struct udp_pcb *pcb, udp_receive_callback callback, void *context)
{ (void)pcb; (void)callback; (void)context; }
err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *address, uint16_t port)
{ (void)pcb; (void)p; (void)address; (void)port; return ERR_OK; }

static void reset(void)
{
    rtspark_tcp_close();
    mock_tick=100;
    tick_increment=0;
    tick_previous=mock_tick;
    uptime_ms=clock_set_uptime=clock_unix=0;
    time_valid=time_phase=0;
    time_udp=NULL;
    time_attempt_tick=mock_tick;
    network_was_ready=network_ready=1;
    rng_ready=1; rng_failed=rng_previous_valid=0;
    mock_rng.SR=RNG_SR_DRDY; mock_rng.DR=123; mock_rng.CR=RNG_CR_RNGEN;
    aborts=freed=received=written=0;
    send_capacity=1460; write_result=output_result=ERR_OK;
    dns_result=ERR_INPROGRESS;
}

static void connect_now(void)
{
    ip_addr_t address={42};
    assert(rtspark_tcp_open("example.invalid",8883)==0);
    assert(rtspark_tcp_state()==RTSPARK_TCP_RESOLVING);
    pending_dns("example.invalid",&address,pending_dns_context);
    assert(rtspark_tcp_state()==RTSPARK_TCP_CONNECTING);
    assert(tcp_connected(NULL,broker_tcp,ERR_OK)==ERR_OK);
    assert(rtspark_tcp_state()==RTSPARK_TCP_CONNECTED);
}

static void test_transport(void)
{
    unsigned char input[4096], output[4096];
    struct pbuf packet={input,3000};
    dns_found_callback old_callback;
    void *old_context;
    ip_addr_t address={42};
    reset();
    assert(rtspark_tcp_open("example.invalid",8883)==0);
    old_callback=pending_dns; old_context=pending_dns_context;
    rtspark_tcp_close();
    old_callback("example.invalid",&address,old_context);
    assert(rtspark_tcp_state()==RTSPARK_TCP_IDLE && broker_tcp==NULL);
    connect_now();
    for (unsigned int i=0;i<sizeof(input);++i) input[i]=(unsigned char)i;
    assert(tcp_receive(NULL,broker_tcp,&packet,ERR_OK)==ERR_OK);
    assert(freed==1 && received==0); /* Window credit only after TLS consumes. */
    assert(rtspark_tcp_recv(NULL,output,2500)==2500);
    assert(memcmp(input,output,2500)==0 && received==2500);
    packet.tot_len=3000;
    assert(tcp_receive(NULL,broker_tcp,&packet,ERR_OK)==ERR_OK); /* Ring wraps. */
    assert(rtspark_tcp_recv(NULL,output,sizeof(output))==3500);
    assert(memcmp(output,input+2500,500)==0);
    assert(memcmp(output+500,input,3000)==0);
    assert(rtspark_tcp_recv(NULL,output,1)==MBEDTLS_ERR_SSL_WANT_READ);
    packet.tot_len=4096;
    assert(tcp_receive(NULL,broker_tcp,&packet,ERR_OK)==ERR_OK);
    packet.tot_len=1;
    assert(tcp_receive(NULL,broker_tcp,&packet,ERR_OK)==ERR_MEM);
    assert(freed==3); /* Refused pbuf remains owned by lwIP. */
    assert(rtspark_tcp_recv(NULL,output,sizeof(output))==4096);
    output_result=ERR_MEM;
    assert(rtspark_tcp_send(NULL,input,2000)==1460 && written==1460);
    send_capacity=0;
    assert(rtspark_tcp_send(NULL,input,1)==MBEDTLS_ERR_SSL_WANT_WRITE);
    send_capacity=10; write_result=ERR_MEM;
    assert(rtspark_tcp_send(NULL,input,1)==MBEDTLS_ERR_SSL_WANT_WRITE);
    assert(written==1460); /* No replay after accepted tcp_write/output failure. */
    assert(tcp_receive(NULL,broker_tcp,NULL,ERR_OK)==ERR_OK);
    assert(rtspark_tcp_recv(NULL,output,1)==0);
    assert(rtspark_tcp_send(NULL,input,1)==MBEDTLS_ERR_SSL_INTERNAL_ERROR);
    reset(); connect_now();
    network_ready=0; rtspark_platform_poll();
    assert(rtspark_tcp_state()==RTSPARK_TCP_FAILED && aborts==1);
    reset();
    assert(rtspark_tcp_open("example.invalid",8883)==0);
    mock_tick+=CONNECT_TIMEOUT_MS; rtspark_platform_poll();
    assert(rtspark_tcp_state()==RTSPARK_TCP_FAILED);
}

static void ntp_reply(unsigned char reply[48], uint64_t unix_seconds)
{
    uint32_t seconds=(uint32_t)(unix_seconds+NTP_UNIX_OFFSET);
    memset(reply,0,48); reply[0]=(4U<<3)|4U; reply[1]=2;
    memcpy(reply+24,time_token,8);
    reply[40]=(unsigned char)(seconds>>24); reply[41]=(unsigned char)(seconds>>16);
    reply[42]=(unsigned char)(seconds>>8); reply[43]=(unsigned char)seconds;
}

static void test_clock(void)
{
    unsigned char reply[48];
    ip_addr_t source={42}, wrong_source={43};
    struct pbuf packet={reply,48};
    reset(); time_phase=2; time_address=source;
    memcpy(time_token,"NONCE123",8);
    ntp_reply(reply,UNIX_2026+1000);
    time_receive(NULL,NULL,&packet,&wrong_source,123);
    assert(!rtspark_clock_ready());
    time_receive(NULL,NULL,&packet,&source,124);
    assert(!rtspark_clock_ready());
    for (unsigned int i=0;i<4;++i) {
        ntp_reply(reply,UNIX_2026+1000);
        if (i==0) reply[0]|=0xc0; /* Unsynchronized. */
        if (i==1) reply[1]=0; /* Kiss-of-death. */
        if (i==2) reply[24]^=1; /* Wrong request token. */
        if (i==3) ntp_reply(reply,UNIX_2026-1); /* Implausible old date. */
        time_receive(NULL,NULL,&packet,&source,123);
        assert(!rtspark_clock_ready());
    }
    ntp_reply(reply,UNIX_2026+1000);
    time_receive(NULL,NULL,&packet,&source,123);
    assert(rtspark_clock_ready());
    assert(rtspark_time(NULL)==(time_t)(UNIX_2026+1000));
    tick_previous=0xfffffff0U; mock_tick=25;
    assert(rtspark_clock_ready() && uptime_ms==41); /* HAL tick wrap. */
    mock_tick+=959;
    assert(rtspark_time(NULL)==(time_t)(UNIX_2026+1001));
    mock_tick+=(uint32_t)CLOCK_MAX_AGE_MS;
    assert(!rtspark_clock_ready() && rtspark_time(NULL)==(time_t)-1);
    reset(); time_phase=2; time_address=source;
    ntp_reply(reply,2140000000ULL); /* NTP era 1, after February 2036 wrap. */
    time_receive(NULL,NULL,&packet,&source,123);
    assert(rtspark_time(NULL)==(time_t)2140000000ULL);
}

static void test_rng_fail_closed(void)
{
    unsigned char output[4];
    reset();
    assert(rtspark_random(NULL,output,sizeof(output))==0);
    assert(rtspark_random(NULL,output,sizeof(output))==MBEDTLS_ERR_ENTROPY_SOURCE_FAILED);
    assert(!rtspark_random_ready() && !(mock_rng.CR&RNG_CR_RNGEN));
    assert(rtspark_tcp_open("example.invalid",8883)==-1);
    reset(); mock_rng.SR=RNG_SR_CECS|RNG_SR_DRDY;
    assert(rtspark_random(NULL,output,sizeof(output))==MBEDTLS_ERR_ENTROPY_SOURCE_FAILED);
    assert(!rtspark_random_ready());
    reset(); mock_rng.SR=0; tick_increment=1;
    assert(rtspark_random(NULL,output,sizeof(output))==MBEDTLS_ERR_ENTROPY_SOURCE_FAILED);
    assert(!rtspark_random_ready());
    reset(); mock_rcc.PLLCFGR=(4U<<24); /* Old PLLQ gives 84 MHz, not 48 MHz. */
    rtspark_platform_init();
    assert(!rtspark_random_ready());
}

int main(void)
{
    test_transport();
    test_clock();
    test_rng_fail_closed();
    puts("PASS: actual TCP buffering/backpressure, stale DNS/timeout, UTC/NTP validation and RNG faults");
    return 0;
}
