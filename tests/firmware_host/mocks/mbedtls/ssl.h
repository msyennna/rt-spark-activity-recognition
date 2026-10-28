#ifndef MOCK_MBEDTLS_SSL_H
#define MOCK_MBEDTLS_SSL_H
#include <stddef.h>
#include <stdint.h>
#define MBEDTLS_ERR_SSL_WANT_READ -0x6900
#define MBEDTLS_ERR_SSL_WANT_WRITE -0x6880
#define MBEDTLS_ERR_SSL_INTERNAL_ERROR -0x6c00
#define MBEDTLS_ERR_SSL_CONN_EOF -0x7280
#define MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS -0x7000
#define MBEDTLS_SSL_IS_CLIENT 0
#define MBEDTLS_SSL_TRANSPORT_STREAM 0
#define MBEDTLS_SSL_PRESET_DEFAULT 0
#define MBEDTLS_SSL_VERIFY_REQUIRED 2
#define MBEDTLS_SSL_VERSION_TLS1_2 0x0303
typedef struct { int live; } mbedtls_ssl_context;
typedef struct { int auth; } mbedtls_ssl_config;
struct mock_x509;
void mbedtls_ssl_init(mbedtls_ssl_context *);
void mbedtls_ssl_free(mbedtls_ssl_context *);
void mbedtls_ssl_config_init(mbedtls_ssl_config *);
void mbedtls_ssl_config_free(mbedtls_ssl_config *);
int mbedtls_ssl_config_defaults(mbedtls_ssl_config *, int, int, int);
void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *, int);
void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *, struct mock_x509 *, void *);
void mbedtls_ssl_conf_rng(mbedtls_ssl_config *, int (*)(void *, unsigned char *, size_t), void *);
void mbedtls_ssl_conf_min_tls_version(mbedtls_ssl_config *, int);
void mbedtls_ssl_conf_max_tls_version(mbedtls_ssl_config *, int);
int mbedtls_ssl_setup(mbedtls_ssl_context *, mbedtls_ssl_config *);
int mbedtls_ssl_set_hostname(mbedtls_ssl_context *, const char *);
void mbedtls_ssl_set_bio(mbedtls_ssl_context *, void *,
                         int (*)(void *, const unsigned char *, size_t),
                         int (*)(void *, unsigned char *, size_t), void *);
int mbedtls_ssl_handshake_step(mbedtls_ssl_context *);
int mbedtls_ssl_is_handshake_over(mbedtls_ssl_context *);
uint32_t mbedtls_ssl_get_verify_result(mbedtls_ssl_context *);
const char *mbedtls_ssl_get_version(mbedtls_ssl_context *);
int mbedtls_ssl_write(mbedtls_ssl_context *, const unsigned char *, size_t);
int mbedtls_ssl_read(mbedtls_ssl_context *, unsigned char *, size_t);
#endif
