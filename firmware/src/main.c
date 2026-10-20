/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "custom_bus.h"
#include "icm20608.h"
#include "rw007_netif.h"
#include "rw007_transport.h"
#include <stdio.h>
#include <string.h>
#if defined(__has_include)
#if __has_include("rw007_wifi_config.h")
#include "rw007_wifi_config.h"
#endif
#endif
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* RT-Thread v4.1.1 wlan_dev.h: preserve the enum ABI used by wlan_mode(). */
typedef enum
{
    RT_WLAN_NONE,
    RT_WLAN_STATION,
    RT_WLAN_AP,
    RT_WLAN_MODE_MAX
} rt_wlan_mode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RW007_CS_SETUP_DELAY_MS 2U
#define RW007_MAX_PACKET_SIZE (8U + 1520U)
/* rw007 v2.1.0 and RT-Thread v4.1.1 wlan_dev.h protocol constants. */
#define RW00x_CMD_SET_MODE 1U
#define RW00x_CMD_SCAN 6U
#define RW00x_CMD_JOIN 7U
#define DATA_TYPE_CB 6U
#define DATA_TYPE_STA_ETH_DATA 1U
#define RT_WLAN_DEV_EVT_CONNECT 1U
#define RT_WLAN_DEV_EVT_CONNECT_FAIL 2U
#define RT_WLAN_DEV_EVT_DISCONNECT 3U
#define RT_WLAN_DEV_EVT_SCAN_REPORT 9U
#define RT_WLAN_DEV_EVT_SCAN_DONE 10U
#define RT_WLAN_SSID_MAX_LENGTH 32U
#define RW007_WLAN_INFO_WIRE_SIZE 60U
#define RW007_SCAN_WINDOW_MS 15000U
/* v4.1.1 default: rw007_ap_info_value = rt_wlan_info + passwd[32]. */
#define RT_WLAN_PASSWORD_MAX_LENGTH 32U
#define RW007_JOIN_VALUE_SIZE (RW007_WLAN_INFO_WIRE_SIZE + RT_WLAN_PASSWORD_MAX_LENGTH)
#define RW007_JOIN_WINDOW_MS 15000U
/* Set these in the optional, ignored include/rw007_wifi_config.h. */
#ifndef RW007_TARGET_SSID
#define RW007_TARGET_SSID ""
#endif
#ifndef RW007_TARGET_PASSWORD
#define RW007_TARGET_PASSWORD ""
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static ICM20608_Data_t imu_data;
static uint8_t rw007_join_ap[RW007_WLAN_INFO_WIRE_SIZE];
static int rw007_join_ap_found;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
static void UART_Print(const char *text);
static int RW007_HardwareReset(void);
static int RW007_SPI_Handshake(void);
static int RW007_CMD_INIT_Test(void);
static int RW007_STATION_MODE_Test(void);
static int RW007_VERSION_Test(void);
static int RW007_MAC_Test(void);
static int RW007_SCAN_Test(void);
static int RW007_JOIN_Test(void);
static int SPI2_Flash_JEDEC_Test(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void UART_Print(const char *text)
{
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)text,
        strlen(text),
        HAL_MAX_DELAY
    );
}


/* ============================================================
 * RW007 HARDWARE RESET
 * ============================================================ */
static int RW007_HardwareReset(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint32_t start;

    UART_Print("RW007: resetting module...\r\n");

    /* Enable required GPIO clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /*
     * PB12 = onboard SPI flash CS.
     * Keep HIGH so the flash does not interfere with SPI2.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);


    /*
     * PB13 = RW007 BOOT0 / SPI2 SCK
     *
     * Hold BOOT0 HIGH during reset for SPI boot.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);


    /*
     * PF10 = RW007 BOOT1 / CS
     *
     * Hold BOOT1 LOW during reset; CS idles HIGH after boot.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);


    /* PG15 = RW007 RESET */
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);


    /* PG11 = RW007 INT/BUSY */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);


    /* Hold RW007 in reset */
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET);

    HAL_Delay(100);

    /* Release reset */
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_SET);


    /*
     * Wait until INT/BUSY becomes HIGH.
     * This tells us the module has completed startup.
     */
    start = HAL_GetTick();

    while (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_11) == GPIO_PIN_RESET)
    {
        if ((HAL_GetTick() - start) > 3000)
        {
            UART_Print("RW007: READY TIMEOUT\r\n");
            return -1;
        }

        HAL_Delay(5);
    }

    /*
     * Give RW007 time to finish its internal reset.
     */
    HAL_Delay(200);


    /*
     * Restore PB13 to SPI2 SCK.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


    /*
     * PF10 remains software-controlled chip select.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);


    /*
     * Latch falling INT/BUSY edges after boot. NVIC stays disabled;
     * polling the EXTI pending bit also catches short ready pulses.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);

    return 0;
}


/* ============================================================
 * RW007 TWO-STAGE SPI HANDSHAKE TEST
 * ============================================================ */
static int RW007_SPI_Handshake(void)
{
    uint8_t tx[16];
    uint8_t rx[16];
    char msg[220];

    HAL_StatusTypeDef status;

    uint8_t type;
    uint16_t seq;
    uint16_t s2m_len;
    uint32_t magic1;
    uint32_t magic2;
    uint16_t remaining;
    uint8_t discard[16];
    int header_valid;

    /*
     * ============================================================
     * STAGE 1
     * ============================================================
     */
    memset(tx, 0, sizeof(tx));
    memset(rx, 0, sizeof(rx));

    tx[0] = 0x00;

    /* flag = MRDY(1), type = MASTER CMD(1) */
    tx[1] = 0x11;

    tx[2] = 0x00;
    tx[3] = 0x00;

    /* Sequence = 1 */
    tx[4] = 0x01;
    tx[5] = 0x00;

    /* M2S_len = 0 for this test */
    tx[6] = 0x00;
    tx[7] = 0x00;

    /* MASTER_MAGIC1 = 0x67452301 */
    tx[8]  = 0x01;
    tx[9]  = 0x23;
    tx[10] = 0x45;
    tx[11] = 0x67;

    /* MASTER_MAGIC2 = 0xEFCDAB89 */
    tx[12] = 0x89;
    tx[13] = 0xAB;
    tx[14] = 0xCD;
    tx[15] = 0xEF;

    UART_Print("RW007: STAGE 1 START\r\n");

    /* Deselect the other device on the shared SPI2 bus. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(2);

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    /* Empirical setup delay required by this board before SPI clocks. */
    HAL_Delay(RW007_CS_SETUP_DELAY_MS);

    status =
        HAL_SPI_TransmitReceive(
            &hspi2,
            tx,
            rx,
            sizeof(tx),
            1000
        );

    /* Discard stale edges while selected; the next edge requests Stage 2. */
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);

    snprintf(
        msg,
        sizeof(msg),
        "RW007 STAGE1 RX: "
        "%02X %02X %02X %02X "
        "%02X %02X %02X %02X "
        "%02X %02X %02X %02X "
        "%02X %02X %02X %02X\r\n",
        rx[0], rx[1], rx[2], rx[3],
        rx[4], rx[5], rx[6], rx[7],
        rx[8], rx[9], rx[10], rx[11],
        rx[12], rx[13], rx[14], rx[15]
    );

    UART_Print(msg);

    if (status != HAL_OK)
    {
        UART_Print("RW007: STAGE 1 SPI ERROR\r\n");
        return -1;
    }

    type = (rx[1] >> 4) & 0x0F;

    magic1 =
        ((uint32_t)rx[8]) |
        ((uint32_t)rx[9] << 8) |
        ((uint32_t)rx[10] << 16) |
        ((uint32_t)rx[11] << 24);

    magic2 =
        ((uint32_t)rx[12]) |
        ((uint32_t)rx[13] << 8) |
        ((uint32_t)rx[14] << 16) |
        ((uint32_t)rx[15] << 24);

    if ((type != 3) ||
        (magic1 != 0x98BADCFE) ||
        (magic2 != 0x10325476))
    {
        UART_Print("RW007: STAGE 1 INVALID\r\n");
        return -1;
    }

    UART_Print("RW007: STAGE 1 OK!\r\n");


    /*
     * ============================================================
     * WAIT FOR RW007 TO REQUEST STAGE 2
     * Accept a falling edge latched during UART output, or a held LOW.
     * ============================================================
     */

    UART_Print("RW007: waiting for INT/BUSY LOW...\r\n");

    uint32_t start = HAL_GetTick();

    while ((__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) == RESET) &&
           (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_11) == GPIO_PIN_SET))
    {
        if ((HAL_GetTick() - start) > 100)
        {
            snprintf(
                msg,
                sizeof(msg),
                "RW007: INT/BUSY TIMEOUT, INT=%d\r\n",
                HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_11)
            );

            UART_Print(msg);

            return -1;
        }
    }

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
    UART_Print("RW007: INT/BUSY READY EVENT - STAGE 2 READY\r\n");


    /*
     * ============================================================
     * STAGE 2
     * ============================================================
     */
    memset(tx, 0, sizeof(tx));
    memset(rx, 0, sizeof(rx));

    tx[0] = 0x00;

    /*
     * flag = MRDY(1)
     * type = MASTER DATA PHASE(2)
     */
    tx[1] = 0x21;

    tx[2] = 0x00;
    tx[3] = 0x00;

    /* Same sequence number = 1 */
    tx[4] = 0x01;
    tx[5] = 0x00;

    /* M2S_len = 0 */
    tx[6] = 0x00;
    tx[7] = 0x00;

    tx[8]  = 0x01;
    tx[9]  = 0x23;
    tx[10] = 0x45;
    tx[11] = 0x67;

    tx[12] = 0x89;
    tx[13] = 0xAB;
    tx[14] = 0xCD;
    tx[15] = 0xEF;

    UART_Print("RW007: STAGE 2 START\r\n");

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_Delay(RW007_CS_SETUP_DELAY_MS);

    status =
        HAL_SPI_TransmitReceive(
            &hspi2,
            tx,
            rx,
            sizeof(tx),
            1000
        );

    if (status != HAL_OK)
    {
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
        UART_Print("RW007: STAGE 2 SPI ERROR\r\n");
        return -1;
    }

    type = (rx[1] >> 4) & 0x0F;

    seq =
        ((uint16_t)rx[4]) |
        ((uint16_t)rx[5] << 8);

    s2m_len =
        ((uint16_t)rx[6]) |
        ((uint16_t)rx[7] << 8);

    magic1 =
        ((uint32_t)rx[8]) |
        ((uint32_t)rx[9] << 8) |
        ((uint32_t)rx[10] << 16) |
        ((uint32_t)rx[11] << 24);

    magic2 =
        ((uint32_t)rx[12]) |
        ((uint32_t)rx[13] << 8) |
        ((uint32_t)rx[14] << 16) |
        ((uint32_t)rx[15] << 24);

    header_valid = (type == 4) && (seq == 1) &&
                   (magic1 == 0x98BADCFE) && (magic2 == 0x10325476);

    /* M2S is zero. Clock ALIGN4(S2M_len) before releasing Stage-2 CS.
     * MRDY advertises receive capacity; discard payload for this bus test.
     * Never trust the length from an invalid or oversized response.
     */
    if (header_valid && (s2m_len <= RW007_MAX_PACKET_SIZE))
    {
        remaining = (uint16_t)((s2m_len + 3U) & ~3U);
        memset(tx, 0, sizeof(tx));

        while (remaining != 0U)
        {
            uint16_t count = (remaining > sizeof(tx)) ? sizeof(tx) : remaining;

            status = HAL_SPI_TransmitReceive(&hspi2, tx, discard, count, 1000);
            if (status != HAL_OK)
            {
                break;
            }
            remaining -= count;
        }
    }

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);

    snprintf(
        msg,
        sizeof(msg),
        "RW007 STAGE2 RX: "
        "%02X %02X %02X %02X "
        "%02X %02X %02X %02X "
        "%02X %02X %02X %02X "
        "%02X %02X %02X %02X\r\n",
        rx[0], rx[1], rx[2], rx[3],
        rx[4], rx[5], rx[6], rx[7],
        rx[8], rx[9], rx[10], rx[11],
        rx[12], rx[13], rx[14], rx[15]
    );

    UART_Print(msg);

    snprintf(
        msg,
        sizeof(msg),
        "RW007 STAGE2 decoded: "
        "type=%u seq=%u S2M=%u "
        "magic1=0x%08lX magic2=0x%08lX\r\n",
        type,
        seq,
        s2m_len,
        (unsigned long)magic1,
        (unsigned long)magic2
    );

    UART_Print(msg);

    /*
     * Expected:
     * type   = 4
     * seq    = 1
     * magic1 = 0x98BADCFE
     * magic2 = 0x10325476
     */
    if (status != HAL_OK)
    {
        UART_Print("RW007: STAGE 2 PAYLOAD SPI ERROR\r\n");
        return -1;
    }

    if (header_valid && (s2m_len > RW007_MAX_PACKET_SIZE))
    {
        UART_Print("RW007: STAGE 2 PAYLOAD LENGTH INVALID\r\n");
        return -1;
    }

    if (header_valid)
    {
        /* The upstream protocol signals data completion with another edge. */
        start = HAL_GetTick();
        while (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) == RESET)
        {
            if ((HAL_GetTick() - start) > 100)
            {
                UART_Print("RW007: STAGE 2 COMPLETION TIMEOUT\r\n");
                return -1;
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);

        UART_Print("RW007: STAGE 2 OK!\r\n");
        UART_Print("RW007: TWO-STAGE SPI HANDSHAKE PASSED!\r\n");

        return 0;
    }

    UART_Print("RW007: STAGE 2 RESPONSE INVALID\r\n");

    return -1;
}

/* CMD_INIT wire layout and exchange follow RT-Thread-packages/rw007:
 * inc/spi_wifi_rw007.h and src/spi_wifi_rw007.c (spi_send_cmd/wifi_data_transfer).
 * Keep the board-confirmed reset and handshake test above unchanged.
 */
static uint16_t RW007_InitRead16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t RW007_InitRead32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void RW007_InitDump(const char *label, const uint8_t *data, uint16_t len)
{
    char line[64];

    UART_Print(label);
    UART_Print("\r\n");
    for (uint16_t offset = 0; offset < len; offset += 16U)
    {
        int used = snprintf(line, sizeof(line), "%04X:", (unsigned int)offset);
        for (uint16_t i = offset; (i < len) && (i < offset + 16U); ++i)
        {
            used += snprintf(line + used, sizeof(line) - used,
                             " %02X", (unsigned int)data[i]);
        }
        snprintf(line + used, sizeof(line) - used, "\r\n");
        UART_Print(line);
    }
}

static void RW007_InitDumpHeader(const char *label, const uint8_t *header)
{
    char msg[180];

    RW007_InitDump(label, header, 16);
    snprintf(msg, sizeof(msg),
             "type=%u flags=0x%X seq=%u S2M=%u rx_buf=%u tx_buf=%u "
             "magic1=0x%08lX magic2=0x%08lX\r\n",
             (unsigned int)(header[1] >> 4), (unsigned int)(header[1] & 0x0F),
             (unsigned int)RW007_InitRead16(header + 4),
             (unsigned int)RW007_InitRead16(header + 6),
             (unsigned int)(header[2] & 0x0F), (unsigned int)(header[2] >> 4),
             (unsigned long)RW007_InitRead32(header + 8),
             (unsigned long)RW007_InitRead32(header + 12));
    UART_Print(msg);
}

static int RW007_InitWaitEvent(int allow_low)
{
    uint32_t start = HAL_GetTick();

    while ((__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) == RESET) &&
           (!allow_low || (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_11) == GPIO_PIN_SET)))
    {
        if ((HAL_GetTick() - start) > 100U)
        {
            return -1;
        }
    }
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
    return 0;
}

/* RT-Thread v4.1.1 components/drivers/wlan/wlan_dev.h:
 * rt_wlan_ssid = u8 len + u8 val[33]. In the 60-byte ARM rt_wlan_info,
 * security/band/datarate occupy bytes 0..11; channel/rssi are signed LE16
 * at 12/14, SSID len/value at 16/17, BSSID at 50, hidden at 56.
 * rw007's packed callback wrapper is cmd/len/result followed by this value.
 */
static int RW007_ScanCallback(const uint8_t *response, uint32_t data_len,
                              uint32_t *ap_count, int *done)
{
    uint32_t event, len;
    const uint8_t *info;
    uint8_t ssid_len;
    char ssid[RT_WLAN_SSID_MAX_LENGTH + 1U];
    char msg[160];

    if (data_len < 12U)
    {
        return -1;
    }
    event = RW007_InitRead32(response);
    len = RW007_InitRead32(response + 4);
    if (len > data_len - 12U)
    {
        return -1;
    }
    snprintf(msg, sizeof(msg), "RW007 DATA_TYPE_CB: event=%lu len=%lu result=%ld\r\n",
             (unsigned long)event, (unsigned long)len,
             (long)(int32_t)RW007_InitRead32(response + 8));
    UART_Print(msg);
    /* Upstream callback dispatch selects by event and does not interpret result. */
    if (event == RT_WLAN_DEV_EVT_SCAN_DONE)
    {
        *done = 1;
        return 0;
    }
    if (event != RT_WLAN_DEV_EVT_SCAN_REPORT)
    {
        UART_Print("RW007 SCAN: unknown callback event ignored\r\n");
        return 0;
    }
    if (len < RW007_WLAN_INFO_WIRE_SIZE)
    {
        return -1;
    }
    info = response + 12U;
    ssid_len = info[16];
    if (ssid_len > RT_WLAN_SSID_MAX_LENGTH)
    {
        return -1;
    }
    memcpy(ssid, info + 17U, ssid_len);
    ssid[ssid_len] = '\0';
    snprintf(msg, sizeof(msg),
             "WIFI AP:\r\nSSID: %s\r\nRSSI: %d\r\nCHANNEL: %d\r\n"
             "BSSID: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
             ssid, (int)(int16_t)RW007_InitRead16(info + 14U),
             (int)(int16_t)RW007_InitRead16(info + 12U),
             (unsigned int)info[50], (unsigned int)info[51], (unsigned int)info[52],
             (unsigned int)info[53], (unsigned int)info[54], (unsigned int)info[55]);
    UART_Print(msg);
    ++*ap_count;
    return 0;
}

/* Capture a validated scan report separately, leaving scan decoding unchanged. */
static void RW007_JoinRememberAP(const uint8_t *response, uint32_t data_len)
{
    size_t target_len = strlen(RW007_TARGET_SSID);
    const uint8_t *info;
    uint32_t len;
    uint8_t ssid_len;
    int exact_match;
    char msg[180];

    if ((data_len < 12U) ||
        (RW007_InitRead32(response) != RT_WLAN_DEV_EVT_SCAN_REPORT))
    {
        return;
    }
    len = RW007_InitRead32(response + 4U);
    if ((len < RW007_WLAN_INFO_WIRE_SIZE) || (len > data_len - 12U))
    {
        return;
    }
    info = response + 12U;
    ssid_len = info[16];
    if (ssid_len > RT_WLAN_SSID_MAX_LENGTH)
    {
        return;
    }
    exact_match = (target_len > 0U) && (target_len <= RT_WLAN_SSID_MAX_LENGTH) &&
                  (ssid_len == target_len) &&
                  (memcmp(info + 17U, RW007_TARGET_SSID, target_len) == 0);
    snprintf(msg, sizeof(msg),
             "RW007 TARGET COMPARE: target_len=%lu scan_len=%u exact=%s\r\n",
             (unsigned long)target_len, (unsigned int)ssid_len,
             exact_match ? "YES" : "NO");
    UART_Print(msg);
    RW007_InitDump("RW007 SCAN SSID BYTES:", info + 17U, ssid_len);
    if (exact_match)
    {
        snprintf(msg, sizeof(msg),
                 "RW007 TARGET MATCH FOUND\r\nTARGET CHANNEL: %d\r\n"
                 "TARGET BSSID: %02X:%02X:%02X:%02X:%02X:%02X\r\n"
                 "TARGET SECURITY: %lu\r\n",
                 (int)(int16_t)RW007_InitRead16(info + 12U),
                 (unsigned int)info[50], (unsigned int)info[51], (unsigned int)info[52],
                 (unsigned int)info[53], (unsigned int)info[54], (unsigned int)info[55],
                 (unsigned long)RW007_InitRead32(info));
        UART_Print(msg);
        /* Keep selecting the first exact match, while diagnosing every report. */
        if (!rw007_join_ap_found)
        {
            memcpy(rw007_join_ap, info, sizeof(rw007_join_ap));
            rw007_join_ap_found = 1;
        }
    }
}

static int RW007_JoinCallback(const uint8_t *response, uint32_t data_len, int *outcome)
{
    uint32_t event, len;
    char msg[100];

    if (data_len < 12U)
    {
        return -1;
    }
    event = RW007_InitRead32(response);
    len = RW007_InitRead32(response + 4U);
    if (len > data_len - 12U)
    {
        return -1;
    }
    snprintf(msg, sizeof(msg), "RW007 DATA_TYPE_CB: event=%lu len=%lu result=%ld\r\n",
             (unsigned long)event, (unsigned long)len,
             (long)(int32_t)RW007_InitRead32(response + 8U));
    UART_Print(msg);
    /* As upstream, dispatch callbacks by event, independently of cmd result. */
    if (event == RT_WLAN_DEV_EVT_CONNECT)
    {
        *outcome = 1;
    }
    else if ((event == RT_WLAN_DEV_EVT_CONNECT_FAIL) ||
             (event == RT_WLAN_DEV_EVT_DISCONNECT))
    {
        *outcome = -1;
    }
    else
    {
        UART_Print("RW007 JOIN: unknown callback event ignored\r\n");
    }
    return 0;
}

typedef struct
{
    uint16_t s2m;
    uint16_t effective_m2s;
    int queued;
    HAL_StatusTypeDef status;
    const char *failure;
} RW007_ExchangeResult;

/* The command runner and Ethernet service share exactly one sequence stream. */
static uint16_t rw007_spi_seq = 2; /* Handshake used 1. */
static int rw007_spi_busy;

static int RW007_SPI_Exchange(const uint8_t *init_packet, uint16_t m2s,
                              uint8_t *packet, int command, const char *name,
                              int pending, RW007_ExchangeResult *result)
{
    uint8_t header[16] =
    {
        0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
    };
    uint8_t stage1[16], stage2[16], tx_chunk[16];
    const uint16_t seq = rw007_spi_seq;
    uint16_t effective_m2s = 0U, s2m = 0U, clocks = 0U;
    HAL_StatusTypeDef status = HAL_OK;
    const char *failure = NULL;
    char msg[180];

    memset(result, 0, sizeof(*result));
    if (rw007_spi_busy || (m2s > RW007_MAX_PACKET_SIZE) ||
        ((m2s != 0U) && (init_packet == NULL)) || (packet == NULL))
    {
        result->failure = "invalid or overlapping SPI exchange";
        return -1;
    }
    rw007_spi_busy = 1;
    header[1] = 0x11; /* master_cmd_phase + MRDY (receive buffer available). */
    header[4] = (uint8_t)seq;
    header[5] = (uint8_t)(seq >> 8);
    header[6] = (uint8_t)m2s;
    header[7] = (uint8_t)(m2s >> 8);
    if (name != NULL)
    {
        snprintf(msg, sizeof(msg), "RW007 %s %s: seq=%u M2S=%u\r\n",
                 name, pending ? "SEND" : "RESPONSE POLL", (unsigned int)seq, (unsigned int)m2s);
        UART_Print(msg);
        RW007_InitDump(command == RW00x_CMD_JOIN ? "RW007 JOIN STAGE1 TX:" :
                     (command == RW00x_CMD_SET_MODE ? "RW007 STATION MODE STAGE1 TX:" :
                      (command == 0U ? "RW007 CMD_INIT STAGE1 TX:" :
                       (command == 2U ? "RW007 MAC STAGE1 TX:" :
                        (command == RW00x_CMD_SCAN ? "RW007 SCAN STAGE1 TX:" : "RW007 VERSION STAGE1 TX:")))),
                       header, sizeof(header));
    }
    memset(stage1, 0, sizeof(stage1));

    /* Reuse the confirmed CS-high gap and post-assert setup delay. */
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_Delay(RW007_CS_SETUP_DELAY_MS);
    status = HAL_SPI_TransmitReceive(&hspi2, header, stage1, sizeof(stage1), 1000);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    if (name != NULL)
    {
        RW007_InitDumpHeader(command == RW00x_CMD_JOIN ? "RW007 JOIN STAGE1 RX:" :
                          (command == RW00x_CMD_SET_MODE ? "RW007 STATION MODE STAGE1 RX:" :
                           (command == 0U ? "RW007 CMD_INIT STAGE1 RX:" :
                            (command == 2U ? "RW007 MAC STAGE1 RX:" :
                             (command == RW00x_CMD_SCAN ? "RW007 SCAN STAGE1 RX:" : "RW007 VERSION STAGE1 RX:")))), stage1);
    }
    if (status != HAL_OK)
    {
        failure = "Stage 1 SPI error";
        goto exchange_fail;
    }
    if (((stage1[1] >> 4) != 3U) ||
        (RW007_InitRead32(stage1 + 8) != 0x98BADCFEU) ||
        (RW007_InitRead32(stage1 + 12) != 0x10325476U))
    {
        failure = "Stage 1 invalid header";
        goto exchange_fail;
    }
    if (RW007_InitWaitEvent(1) != 0)
    {
        failure = "Stage 2 ready timeout";
        goto exchange_fail;
    }

    header[1] = 0x21; /* master_data_phase; same sequence, MRDY, and M2S. */
    memset(stage2, 0, sizeof(stage2));
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_Delay(RW007_CS_SETUP_DELAY_MS);
    status = HAL_SPI_TransmitReceive(&hspi2, header, stage2, sizeof(stage2), 1000);
    s2m = RW007_InitRead16(stage2 + 6);
    if (status != HAL_OK)
    {
        failure = "Stage 2 SPI error";
    }
    else if (((stage2[1] >> 4) != 4U) || (RW007_InitRead16(stage2 + 4) != seq) ||
             (RW007_InitRead32(stage2 + 8) != 0x98BADCFEU) ||
             (RW007_InitRead32(stage2 + 12) != 0x10325476U))
    {
        failure = "Stage 2 invalid header";
    }
    else if (s2m > RW007_MAX_PACKET_SIZE)
    {
        failure = "Stage 2 S2M length exceeds 1528";
    }
    else
    {
        /* SRDY gates sending. Keep INIT pending if the slave cannot receive. */
        effective_m2s = (stage2[1] & 0x01U) ? m2s : 0U;
        clocks = (uint16_t)(((s2m > effective_m2s ? s2m : effective_m2s) + 3U) & ~3U);
        for (uint16_t offset = 0; offset < clocks; offset += sizeof(tx_chunk))
        {
            uint16_t count = clocks - offset;
            if (count > sizeof(tx_chunk))
            {
                count = sizeof(tx_chunk);
            }
            memset(tx_chunk, 0, sizeof(tx_chunk));
            if (offset < effective_m2s)
            {
                uint16_t copy_len = effective_m2s - offset;
                if (copy_len > count)
                {
                    copy_len = count;
                }
                memcpy(tx_chunk, init_packet + offset, copy_len);
            }
            status = HAL_SPI_TransmitReceive(&hspi2, tx_chunk, packet + offset, count, 1000);
            if (status != HAL_OK)
            {
                failure = "payload SPI error (command acceptance uncertain)";
                break;
            }
        }
    }
    /* No UART inside the continuous Stage-2 header + payload selection. */
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    if (name != NULL)
    {
        RW007_InitDumpHeader(command == RW00x_CMD_JOIN ? "RW007 JOIN STAGE2 RX:" :
                          (command == RW00x_CMD_SET_MODE ? "RW007 STATION MODE STAGE2 RX:" :
                           (command == 0U ? "RW007 CMD_INIT STAGE2 RX:" :
                            (command == 2U ? "RW007 MAC STAGE2 RX:" :
                             (command == RW00x_CMD_SCAN ? "RW007 SCAN STAGE2 RX:" : "RW007 VERSION STAGE2 RX:")))), stage2);
        snprintf(msg, sizeof(msg),
                 "RW007 %s lengths: M2S=%u SRDY=%u effective_M2S=%u S2M=%u aligned_clocks=%u\r\n",
                 name, (unsigned int)m2s, (unsigned int)(stage2[1] & 1U),
                 (unsigned int)effective_m2s, (unsigned int)s2m, (unsigned int)clocks);
        UART_Print(msg);
    }
    if (failure != NULL)
    {
        goto exchange_fail;
    }
    if ((s2m != 0U) && (name != NULL))
    {
        RW007_InitDump(command == RW00x_CMD_JOIN ? "RW007 JOIN inbound packet:" :
                     (command == RW00x_CMD_SET_MODE ? "RW007 STATION MODE inbound packet:" :
                      (command == 0U ? "RW007 CMD_INIT inbound packet:" :
                       (command == 2U ? "RW007 MAC inbound packet:" :
                        (command == RW00x_CMD_SCAN ? "RW007 SCAN inbound packet:" : "RW007 VERSION inbound packet:")))), packet, s2m);
    }
    if (RW007_InitWaitEvent(0) != 0)
    {
        failure = "data completion timeout (command acceptance uncertain)";
        goto exchange_fail;
    }
    result->s2m = s2m;
    result->effective_m2s = effective_m2s;
    result->queued = ((stage2[2] >> 4) != 0U);
    ++rw007_spi_seq; /* Same point: completion succeeded, before application parsing. */
    rw007_spi_busy = 0;
    return 0;

exchange_fail:
    /* Both stages already release CS before reaching these failure paths. */
    result->status = status;
    result->failure = failure;
    rw007_spi_busy = 0;
    return -1;
}

static int RW007_Command_Test(uint8_t command, const uint8_t **value, uint32_t *value_len)
{
    /* No-parameter commands retain their 16-byte packet. SET_MODE appends
     * sizeof(rt_wlan_mode_t), exactly as upstream wlan_mode() does.
     * Send the used bytes, not sizeof the upstream variable-value union.
     */
    uint8_t init_packet[16U + RW007_JOIN_VALUE_SIZE] =
    {
        0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static uint8_t packet[RW007_MAX_PACKET_SIZE];
    uint16_t request_len = 16U;
    int pending = 1;
    int poll_now = 1;
    int empty_read = 1;
    uint32_t start = HAL_GetTick();
    uint32_t timeout_ms = 10000U;
    uint32_t scan_ap_count = 0;
    int scan_accepted = 0;
    int scan_done = 0;
    int join_accepted = 0;
    int join_outcome = 0;
    int join_timed_out = 0;
    HAL_StatusTypeDef status = HAL_OK;
    const char *failure = NULL;
    const char *name = command == RW00x_CMD_JOIN ? "JOIN" :
                      (command == RW00x_CMD_SET_MODE ? "STATION MODE" :
                       (command == 0U ? "CMD_INIT" :
                        (command == 2U ? "MAC" : (command == RW00x_CMD_SCAN ? "SCAN" : "VERSION"))));
    char msg[180];

    init_packet[8] = command;
    if (command == RW00x_CMD_SET_MODE)
    {
        rt_wlan_mode_t mode = RT_WLAN_STATION;

        init_packet[0] = (uint8_t)(8U + sizeof(mode)); /* spi_data_packet.data_len */
        init_packet[12] = (uint8_t)sizeof(mode);      /* rw007_cmd.len */
        memcpy(init_packet + 16U, &mode, sizeof(mode));
        request_len = 16U + sizeof(mode);
    }
    else if (command == RW00x_CMD_JOIN)
    {
        uint8_t *info = init_packet + 16U;
        size_t password_len = strlen(RW007_TARGET_PASSWORD);

        if (!rw007_join_ap_found || (password_len >= RT_WLAN_PASSWORD_MAX_LENGTH))
        {
            failure = "invalid JOIN target or password length";
            goto fail;
        }
        init_packet[0] = (uint8_t)(8U + RW007_JOIN_VALUE_SIZE);
        init_packet[12] = (uint8_t)RW007_JOIN_VALUE_SIZE;
        /* Mirror upstream wlan_join(): security, 2.4GHz band=1, channel,
         * SSID and BSSID. datarate/rssi/hidden and padding remain zero.
         */
        memcpy(info, rw007_join_ap, 4U);
        info[4] = 1U;
        memcpy(info + 12U, rw007_join_ap + 12U, 2U);
        memcpy(info + 16U, rw007_join_ap + 16U, 34U);
        memcpy(info + 50U, rw007_join_ap + 50U, 6U);
        /* Leave the final passwd byte available for upstream's terminator. */
        memcpy(info + RW007_WLAN_INFO_WIRE_SIZE, RW007_TARGET_PASSWORD, password_len);
        request_len = 16U + RW007_JOIN_VALUE_SIZE;
    }
    snprintf(msg, sizeof(msg), "RW007: %s test, DATA_TYPE_CMD=4 cmd=%u len=%u M2S=%u\r\n",
             name, (unsigned int)command, (unsigned int)init_packet[12], (unsigned int)request_len);
    UART_Print(msg);
    if (command == RW00x_CMD_JOIN)
    {
        UART_Print("RW007 JOIN: outbound payload prepared (password bytes omitted)\r\n");
    }
    else
    {
        RW007_InitDump(command == RW00x_CMD_SET_MODE ? "RW007 STATION MODE outbound packet:" :
                  (command == 0U ? "RW007 CMD_INIT outbound packet:" :
                   (command == 2U ? "RW007 MAC outbound packet:" :
                    (command == RW00x_CMD_SCAN ? "RW007 SCAN outbound packet:" : "RW007 VERSION outbound packet:"))),
                   init_packet, request_len);
    }

    /* Upstream waits up to 10 seconds for a command response. Read on INT,
     * queued slave packets, and once immediately after accepting the command.
     */
    while ((HAL_GetTick() - start) < timeout_ms)
    {
        uint16_t m2s = pending ? request_len : 0U;
        uint16_t effective_m2s = 0U;
        uint16_t s2m = 0U;

        if (!pending && !poll_now &&
            (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) == RESET) &&
            (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_11) == GPIO_PIN_SET))
        {
            HAL_Delay(1);
            continue;
        }

        RW007_ExchangeResult exchange;
        if (RW007_SPI_Exchange(init_packet, m2s, packet, command, name, pending, &exchange) != 0)
        {
            status = exchange.status;
            failure = exchange.failure;
            goto fail;
        }
        effective_m2s = exchange.effective_m2s;
        s2m = exchange.s2m;
        if (effective_m2s != 0U)
        {
            pending = 0;
            snprintf(msg, sizeof(msg), "RW007 %s: %u payload bytes sent with SRDY; awaiting DATA_TYPE_RESP\r\n",
                     name, (unsigned int)request_len);
            UART_Print(msg);
        }
        else if (pending)
        {
            UART_Print(command == RW00x_CMD_JOIN ? "RW007 JOIN: SRDY=0; command deferred, no JOIN bytes sent\r\n" :
                      (command == RW00x_CMD_SET_MODE ? "RW007 STATION MODE: SRDY=0; command deferred, no SET_MODE bytes sent\r\n" :
                      (command == 0U ? "RW007 CMD_INIT: SRDY=0; command deferred, no INIT bytes sent\r\n" :
                       (command == 2U ? "RW007 MAC: SRDY=0; command deferred, no MAC_GET bytes sent\r\n" :
                        (command == RW00x_CMD_SCAN ? "RW007 SCAN: SRDY=0; command deferred, no SCAN bytes sent\r\n" :
                                                    "RW007 VERSION: SRDY=0; command deferred, no GET_VSR bytes sent\r\n")))));
        }
        /* Match upstream's queued-data hint and one extra empty-read toggle. */
        poll_now = exchange.queued;
        if (poll_now)
        {
            empty_read = 1;
        }
        else if (empty_read)
        {
            poll_now = 1;
            empty_read = 0;
        }
        else
        {
            empty_read = 1;
        }
        /* A deferred attempt must not suppress the first post-send read. */
        if (effective_m2s != 0U)
        {
            poll_now = 1;
        }

        if (s2m != 0U)
        {
            uint32_t data_len, data_type, response_cmd, response_len;
            int32_t result;

            if (s2m < 8U)
            {
                failure = "truncated spi_data_packet header";
                goto fail;
            }
            data_len = RW007_InitRead32(packet);
            data_type = RW007_InitRead32(packet + 4);
            snprintf(msg, sizeof(msg), "RW007 packet: data_len=%lu data_type=%lu (DATA_TYPE_RESP=5)\r\n",
                     (unsigned long)data_len, (unsigned long)data_type);
            UART_Print(msg);
            if ((data_len > 1520U) || (data_len > (uint32_t)s2m - 8U))
            {
                failure = "invalid spi_data_packet data_len";
                goto fail;
            }
            if (data_type == DATA_TYPE_STA_ETH_DATA)
            {
                RW007_Network_Receive(packet + 8U, data_len);
                continue;
            }
            if ((command == RW00x_CMD_SCAN) && (data_type == DATA_TYPE_CB))
            {
                if (pending)
                {
                    UART_Print("RW007 SCAN: callback before SCAN was sent; ignored\r\n");
                    continue;
                }
                if (RW007_ScanCallback(packet + 8U, data_len, &scan_ap_count, &scan_done) != 0)
                {
                    failure = "invalid scan callback length or SSID";
                    goto fail;
                }
                RW007_JoinRememberAP(packet + 8U, data_len);
                if (scan_accepted && scan_done)
                {
                    goto scan_complete;
                }
                poll_now = 1; /* Drain a queued reply/event even if its IRQ coalesced. */
                continue;
            }
            if ((command == RW00x_CMD_JOIN) && (data_type == DATA_TYPE_CB))
            {
                if (pending)
                {
                    UART_Print("RW007 JOIN: callback before JOIN was sent; ignored\r\n");
                    continue;
                }
                if (RW007_JoinCallback(packet + 8U, data_len, &join_outcome) != 0)
                {
                    failure = "invalid JOIN callback length";
                    goto fail;
                }
                if (join_accepted && (join_outcome != 0))
                {
                    goto join_complete;
                }
                poll_now = 1;
                continue;
            }
            if (data_type != 5U)
            {
                snprintf(msg, sizeof(msg), "RW007 %s: non-response packet consumed; keep waiting\r\n", name);
                UART_Print(msg);
                continue;
            }
            if (data_len < 12U)
            {
                failure = "truncated rw007_resp header";
                goto fail;
            }
            response_cmd = RW007_InitRead32(packet + 8);
            response_len = RW007_InitRead32(packet + 12);
            result = (int32_t)RW007_InitRead32(packet + 16);
            snprintf(msg, sizeof(msg),
                     "RW007 DATA_TYPE_RESP: cmd=%lu len=%lu result=%ld\r\n",
                     (unsigned long)response_cmd, (unsigned long)response_len, (long)result);
            UART_Print(msg);
            if (response_len > data_len - 12U)
            {
                failure = "rw007_resp len exceeds received data";
                goto fail;
            }
            if (response_cmd == command)
            {
                if (pending)
                {
                    failure = command == RW00x_CMD_JOIN ? "JOIN response before command was sent" :
                             (command == RW00x_CMD_SET_MODE ? "SET_MODE response before command was sent" :
                             (command == 0U ? "INIT response before command was sent" :
                              (command == 2U ? "MAC_GET response before command was sent" :
                               (command == RW00x_CMD_SCAN ? "SCAN response before command was sent" :
                                                           "GET_VSR response before command was sent"))));
                    goto fail;
                }
                if ((result == 0) && (value != NULL) && (value_len != NULL))
                {
                    *value = packet + 20U;
                    *value_len = response_len;
                }
                if (command == RW00x_CMD_JOIN)
                {
                    snprintf(msg, sizeof(msg), "RW007 JOIN COMMAND RESULT: %ld\r\n", (long)result);
                }
                else if (command == RW00x_CMD_SET_MODE)
                {
                    snprintf(msg, sizeof(msg), "RW007 STATION MODE: response result=%ld\r\n", (long)result);
                }
                else
                {
                    snprintf(msg, sizeof(msg), "RW007 %s: %s\r\n", name,
                             result == 0 ? "response result=0" : "module returned an error");
                }
                UART_Print(msg);
                if ((command == RW00x_CMD_SCAN) && (result == 0))
                {
                    if (!scan_accepted)
                    {
                        scan_accepted = 1;
                        start = HAL_GetTick();
                        timeout_ms = RW007_SCAN_WINDOW_MS;
                        poll_now = 1;
                        UART_Print("RW007 SCAN: accepted; waiting for DATA_TYPE_CB scan events\r\n");
                    }
                    if (scan_done)
                    {
                        goto scan_complete;
                    }
                    continue;
                }
                if (command == RW00x_CMD_JOIN)
                {
                    if (result != 0)
                    {
                        join_outcome = -1;
                        goto join_complete;
                    }
                    if (!join_accepted)
                    {
                        join_accepted = 1;
                        start = HAL_GetTick();
                        timeout_ms = RW007_JOIN_WINDOW_MS;
                        poll_now = 1;
                        UART_Print("RW007 JOIN: accepted; waiting for connection callback\r\n");
                    }
                    if (join_outcome != 0)
                    {
                        goto join_complete;
                    }
                    continue;
                }
                return result == 0 ? 0 : -1;
            }
            snprintf(msg, sizeof(msg), "RW007 %s: response for another command; keep waiting\r\n", name);
            UART_Print(msg);
        }
    }
    join_timed_out = (command == RW00x_CMD_JOIN);
    failure = join_accepted ? "connection callback timeout" :
             (command == RW00x_CMD_JOIN ? "JOIN command/send timeout" :
              (scan_accepted ? "SCAN_DONE callback timeout" :
              (pending ? "send timeout (SRDY never set)" :
              (command == RW00x_CMD_SET_MODE ? "DATA_TYPE_RESP SET_MODE timeout" :
              (command == 0U ? "DATA_TYPE_RESP INIT timeout" :
               (command == 2U ? "DATA_TYPE_RESP MAC_GET timeout" :
                (command == RW00x_CMD_SCAN ? "DATA_TYPE_RESP SCAN timeout" : "DATA_TYPE_RESP GET_VSR timeout")))))));

fail:
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    snprintf(msg, sizeof(msg), "RW007 %s FAILED: %s; SPI status=%d error=0x%08lX\r\n",
             name, failure, (int)status, (unsigned long)HAL_SPI_GetError(&hspi2));
    UART_Print(msg);
    if (command == RW00x_CMD_JOIN)
    {
        UART_Print(join_timed_out ? "RW007 WIFI CONNECT TIMEOUT\r\n" :
                                  "RW007 WIFI CONNECT FAILED\r\n");
    }
    return -1;

join_complete:
    UART_Print(join_outcome > 0 ? "RW007 WIFI CONNECTED\r\n" :
                                 "RW007 WIFI CONNECT FAILED\r\n");
    if (join_outcome > 0)
    {
        /* This path requires the existing CONNECT callback AND JOIN response. */
        RW007_Network_SetLink(1);
    }
    return join_outcome > 0 ? 0 : -1;

scan_complete:
    snprintf(msg, sizeof(msg), "RW007 WIFI SCAN DONE\r\nTOTAL APS: %lu\r\n",
             (unsigned long)scan_ap_count);
    UART_Print(msg);
    return 0;
}

/* All runtime frames and callbacks are handled after the shared exchange has
 * released CS. lwIP output only queues frames, so it cannot reenter SPI here.
 */
static void RW007_DispatchPacket(const uint8_t *packet, uint16_t received)
{
    uint32_t data_len, data_type;
    char msg[140];

    if (received < 8U)
    {
        RW007_Network_RxDrop();
        return;
    }
    data_len = RW007_InitRead32(packet);
    data_type = RW007_InitRead32(packet + 4U);
    if ((data_len > 1520U) || (data_len > (uint32_t)received - 8U))
    {
        RW007_Network_RxDrop();
        return;
    }
    if (data_type == DATA_TYPE_STA_ETH_DATA)
    {
        RW007_Network_Receive(packet + 8U, data_len);
    }
    else if (data_type == DATA_TYPE_CB)
    {
        int outcome = 0;
        if (RW007_JoinCallback(packet + 8U, data_len, &outcome) != 0)
        {
            RW007_Network_RxDrop();
            return;
        }
        if (outcome > 0)
        {
            UART_Print("RW007 WIFI CONNECTED\r\n");
            RW007_Network_SetLink(1);
        }
        else if (outcome < 0)
        {
            RW007_Network_SetLink(0);
        }
    }
    else if (data_type == 5U) /* Preserve response diagnostics; Ethernet has no RESP. */
    {
        if ((data_len < 12U) || (RW007_InitRead32(packet + 12U) > data_len - 12U))
        {
            RW007_Network_RxDrop();
            return;
        }
        snprintf(msg, sizeof(msg), "RW007 DATA_TYPE_RESP: cmd=%lu len=%lu result=%ld\r\n",
                 (unsigned long)RW007_InitRead32(packet + 8U),
                 (unsigned long)RW007_InitRead32(packet + 12U),
                 (long)(int32_t)RW007_InitRead32(packet + 16U));
        UART_Print(msg);
    }
    else
    {
        snprintf(msg, sizeof(msg), "RW007: runtime packet type=%lu len=%lu ignored\r\n",
                 (unsigned long)data_type, (unsigned long)data_len);
        UART_Print(msg);
    }
}

int RW007_Transport_Service(const uint8_t *frame, uint16_t frame_len, int *accepted)
{
    static uint8_t tx_packet[RW007_MAX_PACKET_SIZE];
    static uint8_t rx_packet[RW007_MAX_PACKET_SIZE];
    static uint32_t last_poll;
    static uint32_t minimum_gap = 2U;
    static int poll_now = 1;
    static int empty_read = 1;
    RW007_ExchangeResult exchange;
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - last_poll;
    uint16_t tx_len = 0U;
    char msg[160];

    if (accepted == NULL)
    {
        return -1;
    }
    *accepted = 0;
    if (((frame == NULL) && (frame_len != 0U)) ||
        ((frame != NULL) && ((frame_len == 0U) || (frame_len > 1520U))))
    {
        return -1;
    }
    if (rw007_spi_busy || (elapsed < minimum_gap))
    {
        return 0;
    }
    /* Preserve PG11 event/level handling; a 20ms fallback catches coalesced IRQs.
     * Even queued/INT-driven exchanges have a bounded foreground minimum gap.
     */
    if ((frame == NULL) && !poll_now &&
        (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11) == RESET) &&
        (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_11) == GPIO_PIN_SET) && (elapsed < 20U))
    {
        return 0;
    }
    if (frame != NULL)
    {
        memset(tx_packet, 0, 8U);
        tx_packet[0] = (uint8_t)frame_len;
        tx_packet[1] = (uint8_t)(frame_len >> 8);
        tx_packet[4] = DATA_TYPE_STA_ETH_DATA;
        memcpy(tx_packet + 8U, frame, frame_len);
        tx_len = 8U + frame_len;
    }
    if (RW007_SPI_Exchange(tx_packet, tx_len, rx_packet, -1, NULL,
                           frame != NULL, &exchange) != 0)
    {
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
        last_poll = HAL_GetTick();
        minimum_gap = 100U; /* Back off; never blindly replay uncertain payloads. */
        poll_now = 1;
        snprintf(msg, sizeof(msg), "RW007 ETH: %s; SPI status=%d error=0x%08lX\r\n",
                 exchange.failure, (int)exchange.status, (unsigned long)HAL_SPI_GetError(&hspi2));
        UART_Print(msg);
        return -1;
    }
    last_poll = HAL_GetTick();
    minimum_gap = 2U;
    *accepted = exchange.effective_m2s != 0U;
    /* Keep the proven queued-data hint plus extra-empty-read behavior. */
    poll_now = exchange.queued;
    if (poll_now)
    {
        empty_read = 1;
    }
    else if (empty_read)
    {
        poll_now = 1;
        empty_read = 0;
    }
    else
    {
        empty_read = 1;
    }
    if (*accepted)
    {
        poll_now = 1;
    }
    if (exchange.s2m != 0U)
    {
        /* Bad RX is a separate drop; an accepted TX must never be replayed. */
        RW007_DispatchPacket(rx_packet, exchange.s2m);
    }
    return 0;
}

static int RW007_CMD_INIT_Test(void)
{
    return RW007_Command_Test(0U, NULL, NULL);
}

static int RW007_STATION_MODE_Test(void)
{
    UART_Print("RW007 STATION MODE START\r\n");
    if (RW007_Command_Test(RW00x_CMD_SET_MODE, NULL, NULL) != 0)
    {
        return -1;
    }
    UART_Print("RW007 STATION MODE OK\r\n");
    return 0;
}

static int RW007_VERSION_Test(void)
{
    const uint8_t *value = NULL;
    uint32_t len = 0;
    /* Bound by the full packet minus its 8-byte header and 12-byte response.
     * Upstream rw007_version_get(char[16]) does not enforce a wire length.
     */
    char version[RW007_MAX_PACKET_SIZE - 20U + 1U];

    UART_Print("RW007 VERSION QUERY START\r\n");
    if (RW007_Command_Test(5U, &value, &len) != 0) /* RW00x_CMD_GET_VSR = 5 */
    {
        return -1;
    }
    if ((value == NULL) || (len >= sizeof(version)))
    {
        UART_Print("RW007 VERSION: response length invalid\r\n");
        return -1;
    }
    memcpy(version, value, len);
    version[len] = '\0';
    UART_Print("RW007 VERSION: ");
    UART_Print(version);
    UART_Print("\r\n");
    return 0;
}

static int RW007_MAC_Test(void)
{
    const uint8_t *value = NULL;
    uint32_t len = 0;
    char msg[64];

    UART_Print("RW007 MAC QUERY START\r\n");
    /* Upstream wlan_get_mac selects RW00x_CMD_MAC_GET=2 for the station.
     * The shared runner sends no parameter and validates DATA_TYPE_RESP,
     * matching cmd, result=0, and the packet/response length bounds.
     */
    if (RW007_Command_Test(2U, &value, &len) != 0)
    {
        return -1;
    }
    if ((value == NULL) || (len < 6U) || (len > RW007_MAX_PACKET_SIZE - 20U))
    {
        UART_Print("RW007 MAC: response length invalid\r\n");
        return -1;
    }
    /* mac_value may include padding; print only the first six address bytes. */
    snprintf(msg, sizeof(msg), "RW007 MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
             (unsigned int)value[0], (unsigned int)value[1], (unsigned int)value[2],
             (unsigned int)value[3], (unsigned int)value[4], (unsigned int)value[5]);
    UART_Print(msg);
    return 0;
}

static int RW007_SCAN_Test(void)
{
    size_t target_len = strlen(RW007_TARGET_SSID);
    char msg[80];

    rw007_join_ap_found = 0;
    memset(rw007_join_ap, 0, sizeof(rw007_join_ap));
    UART_Print("RW007 TARGET SSID: ");
    UART_Print(RW007_TARGET_SSID);
    UART_Print("\r\n");
    snprintf(msg, sizeof(msg), "RW007 TARGET SSID LENGTH: %lu (maximum %u)\r\n",
             (unsigned long)target_len, (unsigned int)RT_WLAN_SSID_MAX_LENGTH);
    UART_Print(msg);
    RW007_InitDump("RW007 TARGET SSID BYTES:", (const uint8_t *)RW007_TARGET_SSID,
                  (uint32_t)(target_len > RT_WLAN_SSID_MAX_LENGTH ?
                             RT_WLAN_SSID_MAX_LENGTH : target_len));
    UART_Print("RW007 WIFI SCAN START\r\n");
    return RW007_Command_Test(RW00x_CMD_SCAN, NULL, NULL);
}

static int RW007_JOIN_Test(void)
{
    char ssid[RT_WLAN_SSID_MAX_LENGTH + 1U];
    char msg[160];

    if (strlen(RW007_TARGET_SSID) == 0U)
    {
        UART_Print("RW007 JOIN: target SSID not configured in rw007_wifi_config.h\r\n");
    }
    if (!rw007_join_ap_found)
    {
        UART_Print("RW007 JOIN: TARGET SSID NOT FOUND\r\n");
        return -1;
    }
    if (rw007_join_ap[16] > RT_WLAN_SSID_MAX_LENGTH)
    {
        UART_Print("RW007 JOIN: invalid saved SSID length\r\n");
        return -1;
    }
    memcpy(ssid, rw007_join_ap + 17U, rw007_join_ap[16]);
    ssid[rw007_join_ap[16]] = '\0';
    UART_Print("RW007 JOIN START\r\n");
    snprintf(msg, sizeof(msg), "RW007 JOIN TARGET: %s\r\nRW007 JOIN CHANNEL: %d\r\n"
             "RW007 JOIN BSSID: %02X:%02X:%02X:%02X:%02X:%02X\r\n", ssid,
             (int)(int16_t)RW007_InitRead16(rw007_join_ap + 12U),
             (unsigned int)rw007_join_ap[50], (unsigned int)rw007_join_ap[51],
             (unsigned int)rw007_join_ap[52], (unsigned int)rw007_join_ap[53],
             (unsigned int)rw007_join_ap[54], (unsigned int)rw007_join_ap[55]);
    UART_Print(msg);
    return RW007_Command_Test(RW00x_CMD_JOIN, NULL, NULL);
}

/* ============================================================
 * ONBOARD SPI FLASH TEST
 *
 * The onboard flash shares SPI2 with RW007:
 *
 * PB13 = SCK
 * PC2  = MISO
 * PC3  = MOSI
 *
 * PB12 = Flash CS
 * PF10 = RW007 CS
 * ============================================================ */
static int SPI2_Flash_JEDEC_Test(void)
{
    uint8_t tx[4] =
    {
        0x9F,
        0xFF,
        0xFF,
        0xFF
    };

    uint8_t rx[4] =
    {
        0,
        0,
        0,
        0
    };

    char msg[160];

    UART_Print("SPI2: testing onboard flash...\r\n");


    /*
     * Make absolutely sure RW007 is not selected.
     */
    HAL_GPIO_WritePin(
        GPIOF,
        GPIO_PIN_10,
        GPIO_PIN_SET
    );


    /*
     * Flash CS idle HIGH.
     */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_12,
        GPIO_PIN_SET
    );

    HAL_Delay(1);


    /*
     * Select flash.
     */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_12,
        GPIO_PIN_RESET
    );


    HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(
            &hspi2,
            tx,
            rx,
            sizeof(tx),
            1000
        );


    /*
     * Release flash.
     */
    HAL_GPIO_WritePin(
        GPIOB,
        GPIO_PIN_12,
        GPIO_PIN_SET
    );


    snprintf(
        msg,
        sizeof(msg),
        "FLASH SPI status=%d error=0x%08lX\r\n",
        (int)status,
        (unsigned long)HAL_SPI_GetError(&hspi2)
    );

    UART_Print(msg);


    snprintf(
        msg,
        sizeof(msg),
        "FLASH JEDEC RX: %02X %02X %02X %02X\r\n",
        rx[0],
        rx[1],
        rx[2],
        rx[3]
    );

    UART_Print(msg);


    if (status != HAL_OK)
    {
        UART_Print("FLASH: SPI TRANSFER ERROR\r\n");
        return -1;
    }


    /*
     * Winbond manufacturer ID = 0xEF.
     *
     * Because the first received byte occurs while the
     * 0x9F command itself is transmitted, manufacturer
     * ID normally appears in rx[1].
     */
    if (rx[1] == 0xEF)
    {
        UART_Print(
            "FLASH: JEDEC ID OK - SPI2 BUS WORKS!\r\n"
        );

        return 0;
    }


    UART_Print("FLASH: JEDEC ID INVALID\r\n");

    return -1;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI3_Init();
  MX_USART1_UART_Init();
  MX_SPI2_Init();

  /* USER CODE BEGIN 2 */
 char msg[160];
  uint8_t device_id = 0;

  if (RW007_Network_Init() != 0)
  {
    UART_Print("LWIP: INIT FAILED; continuing existing tests\r\n");
  }

  UART_Print("RW007: resetting module...\r\n");

if (RW007_HardwareReset() == 0)
{
    UART_Print("RW007: MODULE READY\r\n");

    SPI2_Flash_JEDEC_Test();

    UART_Print("RW007: STARTING SPI TEST\r\n");

    if (RW007_SPI_Handshake() == 0)
    {
        UART_Print("RW007: SPI TEST PASSED\r\n");
        if (RW007_CMD_INIT_Test() != 0)
        {
            UART_Print("RW007: CMD_INIT TEST FAILED; continuing IMU test\r\n");
        }
        else if (RW007_STATION_MODE_Test() != 0)
        {
            UART_Print("RW007: STATION MODE FAILED; continuing IMU test\r\n");
        }
        else if (RW007_VERSION_Test() != 0)
        {
            UART_Print("RW007: VERSION QUERY FAILED; continuing IMU test\r\n");
        }
        else if (RW007_MAC_Test() != 0)
        {
            UART_Print("RW007: MAC QUERY FAILED; continuing IMU test\r\n");
        }
        else if (RW007_SCAN_Test() != 0)
        {
            UART_Print("RW007: WIFI SCAN FAILED; continuing IMU test\r\n");
        }
        else if (RW007_JOIN_Test() != 0)
        {
            UART_Print("RW007: JOIN TEST FAILED; continuing IMU test\r\n");
        }
    }
    else
    {
        UART_Print("RW007: SPI TEST FAILED\r\n");
    }

    UART_Print("RW007: SPI TEST FINISHED\r\n");
}
else
{
    UART_Print("RW007: READY TIMEOUT\r\n");
}

  UART_Print("\r\n====================================\r\n");
  UART_Print("RT-SPARK ACTIVITY RECOGNITION\r\n");
  UART_Print("ICM20608 SENSOR TEST\r\n");
  UART_Print("====================================\r\n");

  if (ICM20608_Init() == 0)
  {
    if (ICM20608_ReadID(&device_id) == 0)
    {
      snprintf(
          msg,
          sizeof(msg),
          "WHO_AM_I: 0x%02X\r\n",
          device_id
      );

      UART_Print(msg);
    }

    UART_Print("ICM20608 INITIALIZED SUCCESSFULLY!\r\n");
    UART_Print("Reading accelerometer + gyroscope...\r\n");
  }
  else
  {
    UART_Print("ICM20608 INITIALIZATION FAILED!\r\n");
  }

  UART_Print("====================================\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
 uint32_t imu_last_report = HAL_GetTick() - 1000U;
 while (1)
{
  RW007_Network_Poll();
  if ((HAL_GetTick() - imu_last_report) < 1000U)
  {
    HAL_Delay(1);
    continue;
  }
  imu_last_report = HAL_GetTick();
  if (ICM20608_ReadData(&imu_data) == 0)
{
  float gyro_x_rad = imu_data.gyro_x_dps * 0.01745329252f;
  float gyro_y_rad = imu_data.gyro_y_dps * 0.01745329252f;
  float gyro_z_rad = imu_data.gyro_z_dps * 0.01745329252f;

  char msg[256];

  snprintf(
      msg,
      sizeof(msg),
      "{\"acceleration_x\":%.6f,"
      "\"acceleration_y\":%.6f,"
      "\"acceleration_z\":%.6f,"
      "\"gyro_x\":%.6f,"
      "\"gyro_y\":%.6f,"
      "\"gyro_z\":%.6f}\r\n",
      (double)imu_data.accel_x_g,
      (double)imu_data.accel_y_g,
      (double)imu_data.accel_z_g,
      (double)gyro_x_rad,
      (double)gyro_y_rad,
      (double)gyro_z_rad
  );

  UART_Print(msg);
}
  else
  {
    UART_Print("ERROR: Unable to read ICM20608\r\n");
  }


  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
}

  /* USER CODE BEGIN 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */

static void MX_SPI2_Init(void)
{
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;

  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
 __HAL_RCC_GPIOF_CLK_ENABLE();
__HAL_RCC_GPIOH_CLK_ENABLE();
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_GPIOC_CLK_ENABLE();
__HAL_RCC_GPIOB_CLK_ENABLE();
__HAL_RCC_GPIOG_CLK_ENABLE();

/* Deselect onboard SPI flash on shared SPI2 bus: PB12 = HIGH */
GPIO_InitStruct.Pin = GPIO_PIN_12;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

/* RW007 CS idle HIGH */
HAL_GPIO_WritePin(RW007_CS_GPIO_Port, RW007_CS_Pin, GPIO_PIN_SET);

/* RW007 RESET idle HIGH */
HAL_GPIO_WritePin(RW007_RST_GPIO_Port, RW007_RST_Pin, GPIO_PIN_SET);

/* Configure RW007 CS pin: PF10 */
GPIO_InitStruct.Pin = RW007_CS_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(RW007_CS_GPIO_Port, &GPIO_InitStruct);

/* Configure RW007 RESET pin: PG15 */
GPIO_InitStruct.Pin = RW007_RST_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(RW007_RST_GPIO_Port, &GPIO_InitStruct);

/* Configure RW007 INT/BUSY pin: PG11 */
  GPIO_InitStruct.Pin = RW007_INT_BUSY_Pin;
  GPIO_InitStruct.Pin = RW007_INT_BUSY_Pin;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(RW007_INT_BUSY_GPIO_Port, &GPIO_InitStruct);

  /* Enable EXTI11 interrupt */
  //HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
 // HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
