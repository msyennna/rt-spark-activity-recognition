#ifndef LWIPOPTS_H
#define LWIPOPTS_H

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
#define LWIP_TCP                        0
#define LWIP_IPV6                       0
#define LWIP_RAW                        0
#define LWIP_DNS                        0
#define LWIP_AUTOIP                     0
#define LWIP_IGMP                       0
#define LWIP_HAVE_LOOPIF                0
#define LWIP_NETIF_LOOPBACK             0
#define LWIP_NETIF_STATUS_CALLBACK      1

/* Bounded storage within the F407's RAM. */
#define MEM_ALIGNMENT                  4
#define MEM_SIZE                       8192
#define PBUF_POOL_SIZE                 4
#define PBUF_POOL_BUFSIZE               1536
#define MEMP_NUM_UDP_PCB                4
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
