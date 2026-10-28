#ifndef RTSPARK_PLATFORM_H
#define RTSPARK_PLATFORM_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* All APIs are foreground-only. Keep RW007_Network_Poll() running between calls. */
typedef enum {
    RTSPARK_TCP_IDLE = 0,
    RTSPARK_TCP_RESOLVING,
    RTSPARK_TCP_CONNECTING,
    RTSPARK_TCP_CONNECTED,
    RTSPARK_TCP_FAILED
} rtspark_tcp_status;

void rtspark_platform_init(void);
void rtspark_platform_poll(void);
int rtspark_clock_ready(void);
time_t rtspark_time(time_t *out);

/* Hardware-only entropy callback for mbedtls_ctr_drbg_seed(). Faults latch closed. */
int rtspark_random(void *context, unsigned char *output, size_t length);
int rtspark_random_ready(void);

/* One asynchronous DNS/TCP connection; no sockets, waits or RTOS dependencies. */
int rtspark_tcp_open(const char *hostname, uint16_t port);
rtspark_tcp_status rtspark_tcp_state(void);
void rtspark_tcp_close(void);
int rtspark_tcp_send(void *context, const unsigned char *data, size_t length);
int rtspark_tcp_recv(void *context, unsigned char *data, size_t capacity);

#endif
