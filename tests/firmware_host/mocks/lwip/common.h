#ifndef MOCK_LWIP_COMMON_H
#define MOCK_LWIP_COMMON_H
#include <stddef.h>
#include <stdint.h>
typedef int err_t;
typedef uint16_t u16_t;
typedef struct { uint32_t value; } ip_addr_t;
struct tcp_pcb { int live; };
struct udp_pcb { int live; };
struct pbuf { void *payload; uint16_t tot_len; };
#define ERR_OK 0
#define ERR_MEM -1
#define ERR_ABRT -2
#define ERR_INPROGRESS -3
#define TCP_WRITE_FLAG_COPY 1
#define PBUF_TRANSPORT 0
#define PBUF_RAM 0
#define DNS_MAX_NAME_LENGTH 128
#define IP_ADDR_ANY NULL
#define ip_addr_cmp(a,b) ((a)->value == (b)->value)
typedef void (*dns_found_callback)(const char *, const ip_addr_t *, void *);
typedef err_t (*tcp_connected_callback)(void *, struct tcp_pcb *, err_t);
typedef err_t (*tcp_receive_callback)(void *, struct tcp_pcb *, struct pbuf *, err_t);
typedef void (*tcp_error_callback)(void *, err_t);
typedef void (*udp_receive_callback)(void *, struct udp_pcb *, struct pbuf *, const ip_addr_t *, u16_t);
err_t dns_gethostbyname(const char *, ip_addr_t *, dns_found_callback, void *);
struct tcp_pcb *tcp_new(void);
void tcp_arg(struct tcp_pcb *, void *);
void tcp_recv(struct tcp_pcb *, tcp_receive_callback);
void tcp_sent(struct tcp_pcb *, void *);
void tcp_err(struct tcp_pcb *, tcp_error_callback);
void tcp_abort(struct tcp_pcb *);
err_t tcp_connect(struct tcp_pcb *, const ip_addr_t *, uint16_t, tcp_connected_callback);
uint16_t tcp_sndbuf(struct tcp_pcb *);
err_t tcp_write(struct tcp_pcb *, const void *, uint16_t, int);
err_t tcp_output(struct tcp_pcb *);
void tcp_recved(struct tcp_pcb *, uint16_t);
uint16_t pbuf_copy_partial(const struct pbuf *, void *, uint16_t, uint16_t);
uint16_t pbuf_free(struct pbuf *);
struct pbuf *pbuf_alloc(int, uint16_t, int);
err_t pbuf_take(struct pbuf *, const void *, uint16_t);
struct udp_pcb *udp_new(void);
void udp_remove(struct udp_pcb *);
err_t udp_bind(struct udp_pcb *, const ip_addr_t *, uint16_t);
void udp_recv(struct udp_pcb *, udp_receive_callback, void *);
err_t udp_sendto(struct udp_pcb *, struct pbuf *, const ip_addr_t *, uint16_t);
#endif
