#include "main.h"
#include "lwip/sys.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

u32_t sys_now(void)
{
    return HAL_GetTick();
}

void LWIP_Port_Diag(const char *format, ...)
{
    char message[192];
    va_list args;

    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart1, (uint8_t *)message, (uint16_t)strlen(message), HAL_MAX_DELAY);
}

void LWIP_Port_Assert(const char *message)
{
    LWIP_Port_Diag("LWIP: ASSERT FAILED: %s\r\n", message);
    Error_Handler();
}
