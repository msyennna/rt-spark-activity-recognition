/* RT-Spark STM32F407 platform services. The MQTT worker owns the RNG. */
#include "rtspark_app.h"
#include "board.h"
#include <rthw.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <finsh.h>

static mbedtls_time_t clock_seconds;
static rt_tick_t clock_tick;
static int clock_valid;
static uint32_t last_random;
static int rng_started;
static int rng_has_last;

int rtspark_random(unsigned char *out, size_t len)
{
    size_t done = 0;
    if (!rng_started)
    {
        RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
        (void)RCC->AHB2ENR;
        RNG->CR = 0;
        RNG->SR = 0;
        RNG->CR = RNG_CR_RNGEN;
        rng_started = 1;
    }
    while (done < len)
    {
        uint32_t word;
        size_t take;
        rt_tick_t start = rt_tick_get();
        for (;;)
        {
            uint32_t status = RNG->SR;
            if (status & (RNG_SR_CECS | RNG_SR_SECS | RNG_SR_CEIS | RNG_SR_SEIS))
                goto failed;
            if (status & RNG_SR_DRDY)
                break;
            if ((rt_tick_t)(rt_tick_get() - start) >= RT_TICK_PER_SECOND / 5)
                goto failed;
            rt_thread_mdelay(1);
        }
        word = RNG->DR;
        if (rng_has_last && word == last_random)
            goto failed;
        last_random = word;
        /* Discard the first word after starting the peripheral. */
        if (!rng_has_last)
        {
            rng_has_last = 1;
            continue;
        }
        take = len - done < sizeof(word) ? len - done : sizeof(word);
        memcpy(out + done, &word, take);
        done += take;
    }
    return 0;
failed:
    memset(out, 0, len);
    RNG->CR = 0;
    rng_started = rng_has_last = 0;
    return -1;
}

mbedtls_time_t rtspark_clock(mbedtls_time_t *out)
{
    mbedtls_time_t now;
    rt_base_t level = rt_hw_interrupt_disable();
    rt_tick_t tick = rt_tick_get();
    rt_tick_t elapsed = tick - clock_tick;
    /* Update the anchor often, so a 32-bit tick wrap does not reset time. */
    clock_seconds += elapsed / RT_TICK_PER_SECOND;
    clock_tick = tick - elapsed % RT_TICK_PER_SECOND;
    now = clock_valid ? clock_seconds : (mbedtls_time_t)0;
    rt_hw_interrupt_enable(level);
    if (out)
        *out = now;
    return now;
}

static int set_clock(uint32_t seconds)
{
    rt_base_t level;
    /* UTC 2026-01-01 through 2037-12-31; compatible with 32-bit time_t. */
    if (seconds < 1767225600UL || seconds >= 2145916800UL)
        return -1;
    level = rt_hw_interrupt_disable();
    clock_seconds = (mbedtls_time_t)seconds;
    clock_tick = rt_tick_get();
    clock_valid = 1;
    rt_hw_interrupt_enable(level);
    return 0;
}

int rtspark_clock_ready(void)
{
    return clock_valid;
}

/* Optional fallback for networks that block NTP: rtspark_time <UTC Unix seconds>. */
static void rtspark_time(int argc, char **argv)
{
    char *end;
    unsigned long value;
    if (argc != 2)
    {
        rt_kprintf("Usage: rtspark_time <UTC Unix seconds>\n");
        return;
    }
    value = strtoul(argv[1], &end, 10);
    if (!argv[1][0] || *end || value > 2145916799UL || set_clock((uint32_t)value))
        rt_kprintf("Invalid timestamp. Use the current UTC Unix timestamp.\n");
    else
        rt_kprintf("[RTSpark] UTC clock set.\n");
}
MSH_CMD_EXPORT(rtspark_time, Set UTC Unix seconds if NTP is unavailable);

int rtspark_socket(const char *host, const char *port, int udp)
{
    struct addrinfo hints, *addresses = RT_NULL, *address;
    int fd = -1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &addresses) != 0)
        return -1;
    for (address = addresses; address; address = address->ai_next)
    {
        long nonblocking = 1;
        int result;
        fd_set writable;
        struct timeval timeout;
        int error = -1;
        socklen_t error_size = sizeof(error);
        fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0)
            continue;
        if (ioctlsocket(fd, FIONBIO, &nonblocking) < 0)
            goto next;
        result = connect(fd, address->ai_addr, address->ai_addrlen);
        if (result == 0)
            break;
        if (errno != EINPROGRESS && errno != EWOULDBLOCK && errno != EALREADY)
            goto next;
        FD_ZERO(&writable);
        FD_SET(fd, &writable);
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        if (select(fd + 1, RT_NULL, &writable, RT_NULL, &timeout) > 0 &&
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_size) == 0 && error == 0)
            break;
next:
        closesocket(fd);
        fd = -1;
    }
    freeaddrinfo(addresses);
    return fd;
}

static uint32_t big32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

int rtspark_clock_sync(void)
{
    static const char *servers[] = {"time.cloudflare.com", "time.google.com"};
    unsigned int i;
    for (i = 0; i < sizeof(servers) / sizeof(servers[0]); ++i)
    {
        unsigned char request[48] = {0}, reply[512];
        int fd, length;
        uint32_t stamp;
        fd_set readable;
        struct timeval timeout;
        request[0] = 0x23; /* NTP version 4, client mode. */
        if (rtspark_random(request + 40, 8))
            return -1;
        fd = rtspark_socket(servers[i], "123", 1);
        if (fd < 0)
            continue;
        if (send(fd, request, sizeof(request), 0) != sizeof(request))
        {
            closesocket(fd);
            continue;
        }
        FD_ZERO(&readable);
        FD_SET(fd, &readable);
        timeout.tv_sec = 4;
        timeout.tv_usec = 0;
        length = select(fd + 1, &readable, RT_NULL, RT_NULL, &timeout) > 0 ?
                 recv(fd, reply, sizeof(reply), 0) : -1;
        closesocket(fd);
        if (length < 48 || (reply[0] & 7) != 4 || (reply[0] >> 6) == 3 ||
            ((reply[0] >> 3) & 7) < 3 || ((reply[0] >> 3) & 7) > 4 ||
            reply[1] == 0 || reply[1] > 15 || memcmp(reply + 24, request + 40, 8))
            continue;
        stamp = big32(reply + 40);
        if (stamp < 2208988800UL || set_clock(stamp - 2208988800UL))
            continue;
        rt_kprintf("[RTSpark] UTC synchronized with %s.\n", servers[i]);
        return 0;
    }
    return -1;
}
