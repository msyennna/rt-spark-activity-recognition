#include "rtspark_platform.h"
#include "main.h"
#include "rw007_netif.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/platform_time.h"
#include "mbedtls/ssl.h"
#include <string.h>

#define TCP_RX_CAPACITY 4096U
#define CONNECT_TIMEOUT_MS 20000U
#define SNTP_RETRY_MS 30000U
#define SNTP_TIMEOUT_MS 10000U
#define SNTP_REFRESH_MS 3600000U
#define CLOCK_MAX_AGE_MS (24ULL * 60ULL * 60ULL * 1000ULL)
#define UNIX_2026 1767225600ULL
#define UNIX_2038 2145916800ULL /* 2038-01-01, before signed 32-bit time_t overflow. */
#define NTP_UNIX_OFFSET 2208988800ULL

/* SNTP bootstraps certificate time, but is not authenticated time. The TLS trust
 * anchor and hostname checks remain mandatory; never use SNTP as a CA substitute. */
static const char time_server[] = "time.cloudflare.com";
static uint32_t tick_previous;
static uint64_t uptime_ms;
static uint64_t clock_set_uptime;
static uint64_t clock_unix;
static uint32_t time_attempt_tick;
static uint32_t time_generation;
static int time_valid;
static int time_phase; /* 0: idle, 1: DNS, 2: UDP reply */
static struct udp_pcb *time_udp;
static ip_addr_t time_address;
static unsigned char time_token[8];
static int network_was_ready;

static int rng_ready;
static int rng_failed;
static int rng_previous_valid;
static uint32_t rng_previous;

static struct tcp_pcb *broker_tcp;
static rtspark_tcp_status broker_status;
static uint32_t broker_generation;
static uint32_t broker_started;
static uint16_t broker_port;
static unsigned char broker_rx[TCP_RX_CAPACITY];
static uint16_t broker_rx_head;
static uint16_t broker_rx_size;
static int broker_eof;

static void tick_update(void)
{
    uint32_t now = HAL_GetTick();
    uptime_ms += (uint32_t)(now - tick_previous);
    tick_previous = now;
}

mbedtls_ms_time_t mbedtls_ms_time(void)
{
    tick_update();
    return (mbedtls_ms_time_t)uptime_ms;
}

static int rng_word(uint32_t *out)
{
    uint32_t start = HAL_GetTick();
    uint32_t status;
    if (!rng_ready || rng_failed) return -1;
    do {
        status = RNG->SR;
        if ((status & (RNG_SR_CECS | RNG_SR_SECS | RNG_SR_CEIS | RNG_SR_SEIS)) != 0U)
            goto failed;
        if ((status & RNG_SR_DRDY) != 0U) {
            uint32_t word = RNG->DR;
            if ((RNG->SR & (RNG_SR_CECS | RNG_SR_SECS | RNG_SR_CEIS | RNG_SR_SEIS)) != 0U)
                goto failed;
            if (rng_previous_valid && word == rng_previous) goto failed;
            rng_previous = word;
            rng_previous_valid = 1;
            *out = word;
            return 0;
        }
    } while ((uint32_t)(HAL_GetTick() - start) < 5U);
failed:
    rng_failed = 1;
    rng_ready = 0;
    RNG->CR &= ~RNG_CR_RNGEN;
    return -1;
}

int rtspark_random(void *context, unsigned char *output, size_t length)
{
    (void)context;
    while (length != 0U) {
        uint32_t word;
        size_t count = length < sizeof(word) ? length : sizeof(word);
        if (rng_word(&word) != 0) return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        memcpy(output, &word, count);
        output += count;
        length -= count;
    }
    return 0;
}

/* lwIP uses this only for ephemeral ports and DNS transaction IDs. All connection
 * attempts are gated on rng_ready; no PRNG/tick/UID entropy fallback is provided. */
uint32_t rtspark_lwip_rand(void)
{
    uint32_t value = 0;
    (void)rng_word(&value);
    return value;
}

int rtspark_random_ready(void) { return rng_ready && !rng_failed; }

static void tcp_dispose(void)
{
    if (broker_tcp != NULL) {
        tcp_arg(broker_tcp, NULL);
        tcp_recv(broker_tcp, NULL);
        tcp_sent(broker_tcp, NULL);
        tcp_err(broker_tcp, NULL);
        tcp_abort(broker_tcp);
        broker_tcp = NULL;
    }
}

void rtspark_tcp_close(void)
{
    ++broker_generation; /* Ignore outstanding DNS callbacks from an old attempt. */
    tcp_dispose();
    broker_status = RTSPARK_TCP_IDLE;
    broker_rx_head = 0;
    broker_rx_size = 0;
    broker_eof = 0;
}

static void tcp_failed(void)
{
    tcp_dispose();
    broker_status = RTSPARK_TCP_FAILED;
}

static void tcp_error(void *context, err_t error)
{
    (void)context;
    (void)error;
    broker_tcp = NULL; /* lwIP already freed the PCB before this callback. */
    broker_status = RTSPARK_TCP_FAILED;
}

static err_t tcp_receive(void *context, struct tcp_pcb *pcb, struct pbuf *p, err_t error)
{
    uint16_t tail, first;
    (void)context;
    (void)pcb;
    if (p == NULL) {
        broker_eof = 1;
        return ERR_OK;
    }
    if (error != ERR_OK || p->tot_len > TCP_RX_CAPACITY - broker_rx_size)
        return ERR_MEM; /* lwIP retains/refeeds the pbuf; apply TCP backpressure. */
    tail = (uint16_t)((broker_rx_head + broker_rx_size) % TCP_RX_CAPACITY);
    first = p->tot_len < TCP_RX_CAPACITY - tail ? p->tot_len : TCP_RX_CAPACITY - tail;
    pbuf_copy_partial(p, broker_rx + tail, first, 0);
    if (first < p->tot_len)
        pbuf_copy_partial(p, broker_rx, (uint16_t)(p->tot_len - first), first);
    broker_rx_size = (uint16_t)(broker_rx_size + p->tot_len);
    pbuf_free(p);
    /* Advertise consumed bytes only when TLS drains the ring in tcp_recv(). */
    return ERR_OK;
}

static err_t tcp_connected(void *context, struct tcp_pcb *pcb, err_t error)
{
    (void)context;
    (void)pcb;
    if (error != ERR_OK) {
        tcp_failed();
        return ERR_ABRT;
    }
    broker_status = RTSPARK_TCP_CONNECTED;
    return ERR_OK;
}

static void tcp_connect_address(const ip_addr_t *address)
{
    err_t error;
    if (!RW007_Network_Ready() || !rtspark_random_ready()) {
        broker_status = RTSPARK_TCP_FAILED;
        return;
    }
    broker_tcp = tcp_new();
    if (broker_tcp == NULL) {
        broker_status = RTSPARK_TCP_FAILED;
        return;
    }
    tcp_err(broker_tcp, tcp_error);
    tcp_recv(broker_tcp, tcp_receive);
    broker_status = RTSPARK_TCP_CONNECTING;
    error = tcp_connect(broker_tcp, address, broker_port, tcp_connected);
    if (error != ERR_OK) tcp_failed();
}

static void tcp_dns_found(const char *hostname, const ip_addr_t *address, void *context)
{
    (void)hostname;
    if ((uint32_t)(uintptr_t)context != broker_generation || broker_status != RTSPARK_TCP_RESOLVING)
        return;
    if (address == NULL) broker_status = RTSPARK_TCP_FAILED;
    else tcp_connect_address(address);
}

int rtspark_tcp_open(const char *hostname, uint16_t port)
{
    ip_addr_t address;
    err_t error;
    rtspark_tcp_close();
    if (!RW007_Network_Ready() || !rtspark_random_ready() || hostname == NULL ||
        hostname[0] == '\0' || strlen(hostname) >= DNS_MAX_NAME_LENGTH || port == 0U)
        return -1;
    broker_port = port;
    broker_started = HAL_GetTick();
    broker_status = RTSPARK_TCP_RESOLVING;
    error = dns_gethostbyname(hostname, &address, tcp_dns_found, (void *)(uintptr_t)broker_generation);
    if (error == ERR_OK) tcp_connect_address(&address);
    else if (error != ERR_INPROGRESS) broker_status = RTSPARK_TCP_FAILED;
    return broker_status == RTSPARK_TCP_FAILED ? -1 : 0;
}

rtspark_tcp_status rtspark_tcp_state(void) { return broker_status; }

int rtspark_tcp_send(void *context, const unsigned char *data, size_t length)
{
    uint16_t count;
    err_t error;
    (void)context;
    if (broker_status != RTSPARK_TCP_CONNECTED || broker_tcp == NULL || broker_eof)
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    if (length == 0U) return 0;
    count = (uint16_t)(length > 1460U ? 1460U : length);
    if (count > tcp_sndbuf(broker_tcp)) count = tcp_sndbuf(broker_tcp);
    if (count == 0U) return MBEDTLS_ERR_SSL_WANT_WRITE;
    error = tcp_write(broker_tcp, data, count, TCP_WRITE_FLAG_COPY);
    if (error == ERR_MEM) return MBEDTLS_ERR_SSL_WANT_WRITE;
    if (error != ERR_OK) { tcp_failed(); return MBEDTLS_ERR_SSL_INTERNAL_ERROR; }
    /* Even on temporary output failure, tcp_write accepted a private copy. Do
     * not make TLS replay those bytes; lwIP retransmits queued data itself. */
    (void)tcp_output(broker_tcp);
    return count;
}

int rtspark_tcp_recv(void *context, unsigned char *data, size_t capacity)
{
    uint16_t count, first;
    (void)context;
    if (broker_status != RTSPARK_TCP_CONNECTED || broker_tcp == NULL)
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    if (capacity == 0U) return 0;
    if (broker_rx_size == 0U) return broker_eof ? 0 : MBEDTLS_ERR_SSL_WANT_READ;
    count = (uint16_t)(capacity < broker_rx_size ? capacity : broker_rx_size);
    first = count < TCP_RX_CAPACITY - broker_rx_head ? count : TCP_RX_CAPACITY - broker_rx_head;
    memcpy(data, broker_rx + broker_rx_head, first);
    if (first < count) memcpy(data + first, broker_rx, (uint16_t)(count - first));
    broker_rx_head = (uint16_t)((broker_rx_head + count) % TCP_RX_CAPACITY);
    broker_rx_size = (uint16_t)(broker_rx_size - count);
    tcp_recved(broker_tcp, count);
    return count;
}

static uint32_t read_be32(const unsigned char *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | bytes[3];
}

static void time_cleanup(void)
{
    if (time_udp != NULL) {
        udp_remove(time_udp);
        time_udp = NULL;
    }
    time_phase = 0;
}

static void time_receive(void *context, struct udp_pcb *pcb, struct pbuf *p,
                         const ip_addr_t *source, u16_t port)
{
    unsigned char reply[48];
    uint64_t ntp_seconds, unix_seconds;
    (void)context;
    (void)pcb;
    if (p == NULL) return;
    if (time_phase != 2 || port != 123U || !ip_addr_cmp(source, &time_address) ||
        p->tot_len < sizeof(reply)) { pbuf_free(p); return; }
    pbuf_copy_partial(p, reply, sizeof(reply), 0);
    pbuf_free(p);
    if ((reply[0] >> 6) == 3U || (reply[0] & 7U) != 4U ||
        ((reply[0] >> 3) & 7U) < 3U || ((reply[0] >> 3) & 7U) > 4U ||
        reply[1] == 0U || reply[1] > 15U || memcmp(reply + 24, time_token, 8) != 0)
        return;
    if ((read_be32(reply + 40) | read_be32(reply + 44)) == 0U) return;
    ntp_seconds = read_be32(reply + 40);
    /* NTP era 1 begins in 2036; support it without a 32-bit rollover. */
    if (ntp_seconds < NTP_UNIX_OFFSET) ntp_seconds += 0x100000000ULL;
    unix_seconds = ntp_seconds - NTP_UNIX_OFFSET;
    if (unix_seconds < UNIX_2026 || unix_seconds >= UNIX_2038) return;
    tick_update();
    clock_unix = unix_seconds;
    clock_set_uptime = uptime_ms;
    time_valid = 1;
    time_attempt_tick = HAL_GetTick();
    time_cleanup();
}

static void time_send(const ip_addr_t *address)
{
    unsigned char request[48] = {0};
    struct pbuf *packet;
    err_t error;
    if (!RW007_Network_Ready() || !rtspark_random_ready()) { time_cleanup(); return; }
    time_address = *address;
    time_udp = udp_new();
    if (time_udp == NULL) { time_cleanup(); return; }
    error = udp_bind(time_udp, IP_ADDR_ANY, 0);
    if (error != ERR_OK || rtspark_random(NULL, time_token, sizeof(time_token)) != 0) {
        time_cleanup();
        return;
    }
    udp_recv(time_udp, time_receive, NULL);
    request[0] = (4U << 3) | 3U; /* NTPv4 client; random transmit nonce. */
    memcpy(request + 40, time_token, sizeof(time_token));
    packet = pbuf_alloc(PBUF_TRANSPORT, sizeof(request), PBUF_RAM);
    if (packet == NULL) { time_cleanup(); return; }
    pbuf_take(packet, request, sizeof(request));
    time_phase = 2;
    error = udp_sendto(time_udp, packet, &time_address, 123);
    pbuf_free(packet);
    if (error != ERR_OK) time_cleanup();
}

static void time_dns_found(const char *hostname, const ip_addr_t *address, void *context)
{
    (void)hostname;
    if ((uint32_t)(uintptr_t)context != time_generation || time_phase != 1) return;
    if (address == NULL) time_cleanup();
    else time_send(address);
}

void rtspark_platform_init(void)
{
    uint32_t discarded;
    uint32_t pll_q = (RCC->PLLCFGR & RCC_PLLCFGR_PLLQ) >> 24;
    uint32_t pll_p = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1U) * 2U;
    tick_previous = HAL_GetTick();
    time_attempt_tick = tick_previous - SNTP_RETRY_MS;
    /* RNG requires an accurate 48MHz PLL domain. Fail closed if CubeMX changes it. */
    if (pll_q == 0U || ((uint64_t)HAL_RCC_GetSysClockFreq() * pll_p / pll_q) != 48000000ULL) {
        rng_failed = 1;
        return;
    }
    __HAL_RCC_RNG_CLK_ENABLE();
    RNG->CR = 0;
    RNG->SR = 0;
    RNG->CR = RNG_CR_RNGEN;
    rng_ready = 1;
    /* RM0090 requires discarding the first word after enabling the peripheral. */
    (void)rng_word(&discarded);
}

int rtspark_clock_ready(void)
{
    tick_update();
    return time_valid && (uptime_ms - clock_set_uptime) < CLOCK_MAX_AGE_MS &&
           clock_unix + (uptime_ms - clock_set_uptime) / 1000U < UNIX_2038;
}

time_t rtspark_time(time_t *out)
{
    time_t value = (time_t)-1;
    if (rtspark_clock_ready()) value = (time_t)(clock_unix + (uptime_ms - clock_set_uptime) / 1000U);
    if (out != NULL) *out = value;
    return value;
}

void rtspark_platform_poll(void)
{
    uint32_t now = HAL_GetTick();
    int ready = RW007_Network_Ready();
    tick_update();
    if (!ready || !rtspark_random_ready()) {
        if (time_phase != 0) { ++time_generation; time_cleanup(); }
        if (broker_status != RTSPARK_TCP_IDLE && broker_status != RTSPARK_TCP_FAILED)
            tcp_failed();
        network_was_ready = 0;
        return;
    }
    if (!network_was_ready) {
        time_attempt_tick = now - SNTP_RETRY_MS;
        network_was_ready = 1;
    }
    if ((broker_status == RTSPARK_TCP_RESOLVING || broker_status == RTSPARK_TCP_CONNECTING) &&
        (uint32_t)(now - broker_started) >= CONNECT_TIMEOUT_MS)
        tcp_failed();
    if (time_phase != 0 && (uint32_t)(now - time_attempt_tick) >= SNTP_TIMEOUT_MS) {
        ++time_generation;
        time_cleanup();
    }
    if (time_phase == 0 && (uint32_t)(now - time_attempt_tick) >=
        (rtspark_clock_ready() ? SNTP_REFRESH_MS : SNTP_RETRY_MS)) {
        ip_addr_t address;
        err_t error;
        time_attempt_tick = now;
        ++time_generation;
        time_phase = 1;
        error = dns_gethostbyname(time_server, &address, time_dns_found, (void *)(uintptr_t)time_generation);
        if (error == ERR_OK) time_send(&address);
        else if (error != ERR_INPROGRESS) time_cleanup();
    }
}
