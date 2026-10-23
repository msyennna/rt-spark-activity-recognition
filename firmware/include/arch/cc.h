#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdint.h>

/* STM32F407 / GCC; lwIP supplies its C99 types and GCC packing defaults. */
#define BYTE_ORDER LITTLE_ENDIAN

void LWIP_Port_Diag(const char *format, ...);
void LWIP_Port_Assert(const char *message);

#define LWIP_PLATFORM_DIAG(message) do { LWIP_Port_Diag message; } while (0)
#define LWIP_PLATFORM_ASSERT(message) do { LWIP_Port_Assert(message); } while (0)

#endif
