/* RT-Spark hardware entropy adapter, replacing the package's rand() stub. */
#include "mbedtls/config.h"
#include "mbedtls/entropy.h"
#include <stddef.h>

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
extern int rtspark_random(unsigned char *output, size_t len);
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;
    *olen = 0;
    if (rtspark_random(output, len))
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    *olen = len;
    return 0;
}
#endif
