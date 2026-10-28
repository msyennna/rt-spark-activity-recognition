/* Actual Mbed TLS integration harness. Localhost test certificates only. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/platform_time.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"

static time_t test_time;
static SOCKET peer;
static unsigned int wants, crypto_yields, foreground_polls;
static union { uint64_t align; unsigned char bytes[56U * 1024U]; } arena;

time_t rtspark_time(time_t *out)
{
    if (out) *out = test_time;
    return test_time;
}

mbedtls_ms_time_t mbedtls_ms_time(void) { return (mbedtls_ms_time_t)GetTickCount64(); }

static int entropy(void *unused, unsigned char *output, size_t length)
{
    (void)unused;
    return BCryptGenRandom(NULL, output, (ULONG)length, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 0 : -1;
}

static int tx(void *unused, const unsigned char *data, size_t length)
{
    int n;
    (void)unused;
    n = send(peer, (const char *)data, (int)length, 0);
    if (n >= 0) return n;
    return WSAGetLastError() == WSAEWOULDBLOCK ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

static int rx(void *unused, unsigned char *data, size_t length)
{
    int n;
    (void)unused;
    n = recv(peer, (char *)data, (int)length, 0);
    if (n >= 0) return n;
    return WSAGetLastError() == WSAEWOULDBLOCK ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

int main(int argc, char **argv)
{
    WSADATA wsa;
    struct sockaddr_in address = {0};
    FILE *input;
    unsigned char ca_bytes[8192];
    size_t ca_length;
    u_long nonblocking = 1;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt ca;
    mbedtls_ctr_drbg_context drbg;
    int rc = -1;
    uint32_t flags;
    ULONGLONG start;
    if (argc != 5) return 2;
    test_time = (time_t)strtoll(argv[4], NULL, 10);
    input = fopen(argv[2], "rb");
    if (!input) return 3;
    ca_length = fread(ca_bytes, 1, sizeof(ca_bytes) - 1, input);
    fclose(input);
    ca_bytes[ca_length++] = 0;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 4;
    peer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons((u_short)atoi(argv[1]));
    if (connect(peer, (struct sockaddr *)&address, sizeof(address)) != 0) return 5;
    ioctlsocket(peer, FIONBIO, &nonblocking);
    mbedtls_memory_buffer_alloc_init(arena.bytes, sizeof(arena.bytes));
    mbedtls_ecp_set_max_ops(128);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&config);
    mbedtls_x509_crt_init(&ca);
    mbedtls_ctr_drbg_init(&drbg);
#define CHECK(call) do { rc = (call); if (rc != 0) goto finished; } while (0)
    CHECK(mbedtls_ctr_drbg_seed(&drbg, entropy, NULL, (const unsigned char *)"native-tls-test", 15));
    CHECK(mbedtls_x509_crt_parse(&ca, ca_bytes, ca_length));
    CHECK(mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));
    mbedtls_ssl_conf_authmode(&config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&config, &ca, NULL);
    mbedtls_ssl_conf_rng(&config, mbedtls_ctr_drbg_random, &drbg);
    mbedtls_ssl_conf_min_tls_version(&config, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&config, MBEDTLS_SSL_VERSION_TLS1_2);
    CHECK(mbedtls_ssl_setup(&ssl, &config));
    CHECK(mbedtls_ssl_set_hostname(&ssl, argv[3]));
    mbedtls_ssl_set_bio(&ssl, NULL, tx, rx, NULL);
    start = GetTickCount64();
    while (!mbedtls_ssl_is_handshake_over(&ssl) && GetTickCount64() - start < 15000U) {
        ++foreground_polls;
        rc = mbedtls_ssl_handshake_step(&ssl);
        if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) ++wants;
        else if (rc == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS) ++crypto_yields;
        else if (rc != 0) break;
        Sleep(1); /* Represents returning to a foreground network/sensor loop. */
    }
    if (mbedtls_ssl_is_handshake_over(&ssl)) rc = 0;
    else if (rc == 0) rc = -1;
finished:
    flags = mbedtls_ssl_get_verify_result(&ssl);
    printf("rc=%d verify=%lu want=%u crypto=%u polls=%u cipher=%s\n", rc,
           (unsigned long)flags, wants, crypto_yields, foreground_polls,
           mbedtls_ssl_get_ciphersuite(&ssl) ? mbedtls_ssl_get_ciphersuite(&ssl) : "none");
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&config);
    mbedtls_x509_crt_free(&ca);
    mbedtls_ctr_drbg_free(&drbg);
    closesocket(peer);
    WSACleanup();
    return rc == 0 ? 0 : 1;
}
