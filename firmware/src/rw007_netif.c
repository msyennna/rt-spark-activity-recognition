#include "rw007_netif.h"
#include "rw007_transport.h"
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include <string.h>

#define RW007_ETH_FRAME_MAX 1520U
#define RW007_TX_QUEUE_DEPTH 4U
#define RW007_ETH_HEADER_SIZE 14U

typedef struct
{
    uint8_t frame[RW007_ETH_FRAME_MAX];
    uint16_t length;
} RW007_TxSlot;

static struct netif rw007_netif;
static RW007_TxSlot tx_queue[RW007_TX_QUEUE_DEPTH];
static RW007_Network_Stats network_stats;
static unsigned int tx_head;
static unsigned int tx_count;
static uint32_t tx_generation;
static uint32_t in_flight_generation;
static int tx_in_flight;
static int initialized;
static int service_enabled;
static int dhcp_attempted;
static int bound_reported;

static err_t rw007_linkoutput(struct netif *netif, struct pbuf *packet)
{
    const struct pbuf *part;
    RW007_TxSlot *slot;
    uint32_t remaining;
    uint32_t segments = 0U;
    uint16_t length;
    uint16_t offset = 0U;

    if ((packet == NULL) || (packet->tot_len < RW007_ETH_HEADER_SIZE) ||
        (packet->tot_len > RW007_ETH_FRAME_MAX))
    {
        network_stats.tx_drops++;
        return ERR_BUF;
    }
    length = packet->tot_len;
    remaining = length;
    for (part = packet; part != NULL; part = part->next)
    {
        /* Also bounds cyclic chains containing zero-length pbufs. */
        if ((++segments > RW007_ETH_FRAME_MAX + 1U) ||
            (part->tot_len != remaining) || (part->len > remaining) ||
            ((part->len != 0U) && (part->payload == NULL)))
        {
            network_stats.tx_drops++;
            return ERR_BUF;
        }
        remaining -= part->len;
        if (remaining == 0U)
        {
            if (part->next != NULL)
            {
                network_stats.tx_drops++;
                return ERR_BUF;
            }
            break;
        }
    }
    if (remaining != 0U)
    {
        network_stats.tx_drops++;
        return ERR_BUF;
    }
    if (!initialized || (netif != &rw007_netif) || !netif_is_link_up(netif))
    {
        network_stats.tx_drops++;
        return ERR_IF;
    }
    if (tx_count == RW007_TX_QUEUE_DEPTH)
    {
        network_stats.tx_drops++;
        return ERR_MEM;
    }
    slot = &tx_queue[(tx_head + tx_count) % RW007_TX_QUEUE_DEPTH];
    for (part = packet; part != NULL; part = part->next)
    {
        if (part->len != 0U)
        {
            memcpy(slot->frame + offset, part->payload, part->len);
            offset = (uint16_t)(offset + part->len);
        }
    }
    slot->length = length;
    tx_count++;
    /* The SPI service owns transmission; no pbuf pointers leave this call. */
    return ERR_OK;
}

static void rw007_status_callback(struct netif *netif)
{
    char ip[16];
    char mask[16];
    char gateway[16];

    if (bound_reported || !dhcp_supplied_address(netif))
    {
        return;
    }
    if ((ip4addr_ntoa_r(netif_ip4_addr(netif), ip, sizeof(ip)) == NULL) ||
        (ip4addr_ntoa_r(netif_ip4_netmask(netif), mask, sizeof(mask)) == NULL) ||
        (ip4addr_ntoa_r(netif_ip4_gw(netif), gateway, sizeof(gateway)) == NULL))
    {
        return;
    }
    bound_reported = 1;
    LWIP_PLATFORM_DIAG(("LWIP: DHCP BOUND\r\n"));
    LWIP_PLATFORM_DIAG(("LWIP: IP      %s\r\n", ip));
    LWIP_PLATFORM_DIAG(("LWIP: NETMASK %s\r\n", mask));
    LWIP_PLATFORM_DIAG(("LWIP: GATEWAY %s\r\n", gateway));
}

static err_t rw007_netif_init(struct netif *netif)
{
    static const uint8_t mac[6] = {0xFC, 0x58, 0x4A, 0xA6, 0xE5, 0x68};

    netif->name[0] = 'r';
    netif->name[1] = 'w';
    memcpy(netif->hwaddr, mac, sizeof(mac));
    netif->hwaddr_len = sizeof(mac);
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    netif->output = etharp_output;
    netif->linkoutput = rw007_linkoutput;
    return ERR_OK;
}

int RW007_Network_Init(void)
{
    ip4_addr_t zero;

    if (initialized)
    {
        return 0;
    }
    LWIP_PLATFORM_DIAG(("LWIP: INIT START\r\n"));
    lwip_init();
    LWIP_PLATFORM_DIAG(("LWIP: INIT OK\r\n"));

    ip4_addr_set_zero(&zero);
    if (netif_add(&rw007_netif, &zero, &zero, &zero, NULL,
                  rw007_netif_init, ethernet_input) == NULL)
    {
        return -1;
    }
    LWIP_PLATFORM_DIAG(("LWIP: NETIF CREATED\r\n"));
    netif_set_status_callback(&rw007_netif, rw007_status_callback);
    netif_set_default(&rw007_netif);
    /* Administrative up only; the real CONNECT callback enables the link. */
    netif_set_up(&rw007_netif);
    LWIP_PLATFORM_DIAG(("LWIP: MAC FC:58:4A:A6:E5:68\r\n"));
    LWIP_PLATFORM_DIAG(("LWIP: LINK DOWN - WAITING FOR WIFI CONNECTED\r\n"));
    initialized = 1;
    return 0;
}

void RW007_Network_SetLink(int connected)
{
    err_t result;
    ip4_addr_t zero;

    if (!initialized)
    {
        return;
    }
    if (connected)
    {
        if (netif_is_link_up(&rw007_netif))
        {
            return;
        }
        service_enabled = 1;
        bound_reported = 0;
        netif_set_link_up(&rw007_netif);
        LWIP_PLATFORM_DIAG(("LWIP: LINK UP\r\n"));
        if (!dhcp_attempted)
        {
            dhcp_attempted = 1;
            LWIP_PLATFORM_DIAG(("LWIP: DHCP START\r\n"));
            result = dhcp_start(&rw007_netif);
            LWIP_PLATFORM_DIAG(("LWIP: DHCP START result=%d\r\n", (int)result));
            if (result != ERR_OK)
            {
                LWIP_PLATFORM_DIAG(("LWIP: DHCP START FAILED\r\n"));
            }
        }
        return;
    }
    if (!netif_is_link_up(&rw007_netif) && !dhcp_attempted)
    {
        return;
    }
    netif_set_link_down(&rw007_netif);
    dhcp_stop(&rw007_netif);
    ip4_addr_set_zero(&zero);
    netif_set_addr(&rw007_netif, &zero, &zero, &zero);
    dhcp_attempted = 0;
    bound_reported = 0;
    /* Resolve an in-flight head in Poll, after its actual SRDY outcome. */
    network_stats.tx_drops += tx_count;
    if (tx_in_flight && (tx_count != 0U) &&
        (in_flight_generation == tx_generation))
    {
        network_stats.tx_drops--;
    }
    tx_head = 0U;
    tx_count = 0U;
    tx_generation++;
    LWIP_PLATFORM_DIAG(("LWIP: LINK DOWN\r\n"));
}

void RW007_Network_Receive(const uint8_t *frame, uint32_t length)
{
    struct pbuf *packet;
    err_t result;

    if (!initialized || !netif_is_link_up(&rw007_netif) || (frame == NULL) ||
        (length < RW007_ETH_HEADER_SIZE) || (length > RW007_ETH_FRAME_MAX))
    {
        network_stats.rx_drops++;
        return;
    }
    packet = pbuf_alloc(PBUF_RAW, (u16_t)length, PBUF_POOL);
    if (packet == NULL)
    {
        network_stats.rx_drops++;
        return;
    }
    result = pbuf_take(packet, frame, (u16_t)length);
    if (result != ERR_OK)
    {
        pbuf_free(packet);
        network_stats.rx_drops++;
        return;
    }
    /* Copy first: SPI receive storage is reused by the next transaction. */
    result = rw007_netif.input(packet, &rw007_netif);
    if (result != ERR_OK)
    {
        pbuf_free(packet);
        network_stats.rx_drops++;
        return;
    }
    /* Input owns the pbuf on success, including frames it chooses to discard. */
    network_stats.rx_frames++;
    LWIP_PLATFORM_DIAG(("LWIP RX: ETH len=%lu\r\n", (unsigned long)length));
}

void RW007_Network_RxDrop(void)
{
    network_stats.rx_drops++;
}

const RW007_Network_Stats *RW007_Network_GetStats(void)
{
    return &network_stats;
}

void RW007_Network_Poll(void)
{
    const uint8_t *frame = NULL;
    uint16_t length = 0U;
    uint32_t generation;
    int accepted = 0;
    int result;

    if (!initialized)
    {
        return;
    }
    sys_check_timeouts();
    if (!service_enabled)
    {
        return;
    }
    generation = tx_generation;
    if (netif_is_link_up(&rw007_netif) && (tx_count != 0U))
    {
        frame = tx_queue[tx_head].frame;
        length = tx_queue[tx_head].length;
        tx_in_flight = 1;
        in_flight_generation = generation;
    }
    result = RW007_Transport_Service(frame, length, &accepted);
    if (frame != NULL)
    {
        if (accepted)
        {
            network_stats.tx_frames++;
            LWIP_PLATFORM_DIAG(("LWIP TX: ETH len=%u\r\n", (unsigned int)length));
        }
        else if ((result != 0) || (generation != tx_generation))
        {
            network_stats.tx_drops++;
        }
        if ((generation == tx_generation) && (accepted || (result != 0)))
        {
            tx_head = (tx_head + 1U) % RW007_TX_QUEUE_DEPTH;
            tx_count--;
        }
    }
    tx_in_flight = 0;
}
