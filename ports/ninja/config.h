#ifndef LUME_NINJA_CONFIG_H
#define LUME_NINJA_CONFIG_H
#define USE_PPOLL 1
#ifdef __cplusplus
extern "C"
{
#endif
    int getloadavg(double *, int);
#ifdef __cplusplus
}
#endif
#endif
