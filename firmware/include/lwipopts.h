#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#include <stdint.h>
uint32_t rtspark_lwip_rand(void);
#define LWIP_RAND()                     rtspark_lwip_rand()

/* Bare-metal foreground-only stack; no RTOS objects or interrupt callers. */
#define NO_SYS                          1
#define SYS_LIGHTWEIGHT_PROT            0
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0

#define LWIP_IPV4                       1
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_UDP                        1
#define LWIP_DHCP                       1 /* Started only after the CONNECT callback. */
#define LWIP_TCP                        1
#define LWIP_IPV6                       0
#define LWIP_RAW                        0
#define LWIP_DNS                        1
#define LWIP_AUTOIP                     0
#define LWIP_IGMP                       0
#define LWIP_HAVE_LOOPIF                0
#define LWIP_NETIF_LOOPBACK             0
#define LWIP_NETIF_STATUS_CALLBACK      1

/* Bounded storage within the F407's RAM. */
#define MEM_ALIGNMENT                  4
#define MEM_SIZE                       16384
#define PBUF_POOL_SIZE                 8
#define PBUF_POOL_BUFSIZE               1536
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                2
#define MEMP_NUM_TCP_PCB_LISTEN         1
#define MEMP_NUM_TCP_SEG                16
#define TCP_MSS                        1460
#define TCP_WND                        4096
#define TCP_SND_BUF                    (3 * TCP_MSS)
#define TCP_SND_QUEUELEN                12
#define TCP_QUEUE_OOSEQ                 0
#define DNS_TABLE_SIZE                 4
#define DNS_MAX_NAME_LENGTH            128
#define LWIP_DNS_SECURE                7 /* Random ID + source port, reject duplicate queries. */
#define DNS_MAX_RETRIES                3
#define ETH_PAD_SIZE                   0
#define LWIP_STATS                     0

/* The SPI module provides no STM32 Ethernet checksum offload. */
#define LWIP_CHECKSUM_CTRL_PER_NETIF    0
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1

#endif
