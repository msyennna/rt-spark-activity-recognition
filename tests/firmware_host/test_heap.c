#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "../../firmware/src/rtspark_syscalls.c"

static void fails_without_moving(ptrdiff_t increment)
{
    void *before=_sbrk(0);
    size_t cursor=used;
    errno=0;
    assert(_sbrk(increment)==(void *)-1 && errno==ENOMEM);
    assert(used==cursor && _sbrk(0)==before);
}

int main(void)
{
    unsigned char *start=_sbrk(0);
    assert((uintptr_t)start%8==0 && used==0);
    fails_without_moving(-1);
    fails_without_moving(PTRDIFF_MIN);
    fails_without_moving(PTRDIFF_MAX);
    assert(_sbrk(8191)==start && used==8191);
    assert(_sbrk(1)==start+8191 && used==8192);
    assert(_sbrk(0)==start+8192);
    fails_without_moving(1);
    fails_without_moving(8192);
    fails_without_moving(PTRDIFF_MAX);
    fails_without_moving(PTRDIFF_MIN);
    assert(_sbrk(-1)==start+8192 && used==8191);
    assert(_sbrk(-8191)==start+8191 && used==0);
    fails_without_moving(-1);
    assert(_sbrk(8192)==start && used==8192);
    assert(_sbrk(-8192)==start+8192 && used==0);
    puts("PASS: actual bounded C heap exact limit, shrink, extreme increments and failure cursor preservation");
    return 0;
}
