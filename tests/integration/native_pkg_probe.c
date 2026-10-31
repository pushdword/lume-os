#include <stdio.h>

#ifndef PACKAGE_PROBE_REVISION
#define PACKAGE_PROBE_REVISION 1
#endif

int main(void)
{
    printf("NATIVE_PACKAGE_PAYLOAD_%u\n", PACKAGE_PROBE_REVISION);
    return 0;
}
