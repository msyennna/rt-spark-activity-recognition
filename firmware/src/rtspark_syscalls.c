/* newlib's float formatter allocates memory. Replace libnosys' unbounded
 * _sbrk so serial formatting cannot grow the C heap into the stack. TLS uses
 * its separate fixed CCM arena; lwIP uses its own bounded SRAM heap. */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

static union {
    uint64_t alignment;
    unsigned char bytes[8192];
} c_heap;
static size_t used;

void *_sbrk(ptrdiff_t increment)
{
    size_t previous = used;
    if (increment >= 0) {
        if ((size_t)increment > sizeof(c_heap.bytes) - used) goto exhausted;
        used += (size_t)increment;
    } else {
        /* This form also handles PTRDIFF_MIN without signed overflow. */
        size_t shrink = (size_t)(-(increment + 1)) + 1U;
        if (shrink > used) goto exhausted;
        used -= shrink;
    }
    return c_heap.bytes + previous;
exhausted:
    errno = ENOMEM;
    return (void *)-1;
}
