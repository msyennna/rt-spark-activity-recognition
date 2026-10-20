#ifndef RW007_TRANSPORT_H
#define RW007_TRANSPORT_H

#include <stdint.h>

/* Foreground only. Frame storage remains owned by the caller during this call.
 * 0: idle/deferred/completed; -1: transfer failed, acceptance uncertain.
 * accepted is 1 only after a successful SRDY-enabled payload and completion.
 * Received application packets are dispatched before this function returns.
 */
int RW007_Transport_Service(const uint8_t *frame, uint16_t frame_len, int *accepted);

#endif
