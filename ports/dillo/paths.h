/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_DILLO_PATHS_H
#define LUME_DILLO_PATHS_H
#include <unistd.h>
#define DILLO_SYSCONF                                                                              \
    (access("/data/share/dillo/dillorc", R_OK) ? "/usr/share/dillo/" : "/data/share/dillo/")
#define DILLO_BINDIR (access("/data/bin/dpid", X_OK) ? "/usr/bin/" : "/data/bin/")
#define DPIDRC_SYS                                                                                 \
    (access("/data/share/dillo/dpidrc", R_OK) ? "/usr/share/dillo/dpidrc"                          \
                                              : "/data/share/dillo/dpidrc")
#define CA_CERTS_FILE                                                                              \
    (access("/data/share/lume/ssl/ca-bundle.crt", R_OK) ? "/usr/share/lume/ssl/ca-bundle.crt"      \
                                                        : "/data/share/lume/ssl/ca-bundle.crt")
#endif
