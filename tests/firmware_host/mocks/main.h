#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
typedef struct { uint32_t CR, SR, DR; } MockRng;
typedef struct { uint32_t PLLCFGR; } MockRcc;
extern MockRng mock_rng;
extern MockRcc mock_rcc;
#define RNG (&mock_rng)
#define RCC (&mock_rcc)
#define RNG_SR_DRDY (1U << 0)
#define RNG_SR_CECS (1U << 1)
#define RNG_SR_SECS (1U << 2)
#define RNG_SR_CEIS (1U << 5)
#define RNG_SR_SEIS (1U << 6)
#define RNG_CR_RNGEN (1U << 2)
#define RCC_PLLCFGR_PLLQ (15U << 24)
#define RCC_PLLCFGR_PLLP (3U << 16)
#define __HAL_RCC_RNG_CLK_ENABLE() ((void)0)
uint32_t HAL_GetTick(void);
uint32_t HAL_RCC_GetSysClockFreq(void);
#endif
