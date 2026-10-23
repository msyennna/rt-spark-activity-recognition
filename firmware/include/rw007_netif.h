#ifndef RW007_NETIF_H
#define RW007_NETIF_H

#include <stdint.h>

typedef struct
{
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t rx_drops;
    uint32_t tx_drops;
} RW007_Network_Stats;

/* Foreground only: lwIP and SPI must never be entered from an interrupt. */
int RW007_Network_Init(void);
void RW007_Network_SetLink(int connected);
void RW007_Network_Poll(void);
void RW007_Network_Receive(const uint8_t *frame, uint32_t length);
void RW007_Network_RxDrop(void);
const RW007_Network_Stats *RW007_Network_GetStats(void);

#endif
