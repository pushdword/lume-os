#include "native.h"
#include "lume/process.h"
#include <stdio.h>
extern int __real_main(int, char **, char **);

int __wrap_main(int argc, char **argv, char **envp)
{
    const uint64_t required = LUME_FEATURE_FORK | LUME_FEATURE_EXEC | LUME_FEATURE_RANDOM |
                              LUME_FEATURE_SOCKETS | LUME_FEATURE_CREDENTIALS |
                              LUME_FEATURE_SIGNALS | LUME_FEATURE_FILE_MODES;
    struct lume_system_features features;
    if (native_call(LUME_SYS_FEATURES, (uintptr_t)&features, sizeof(features),
                    LUME_FEATURE_VERSION) ||
        (features.features & required) != required)
    {
        fputs(
            "OpenSSH requires a kernel with process, socket, credential, signal and file mode support.\n",
            stderr);
        return 1;
    }
    return __real_main(argc, argv, envp);
}
