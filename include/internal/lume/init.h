#ifndef LUME_RUNTIME_INIT_H
#define LUME_RUNTIME_INIT_H
#ifdef __cplusplus
extern "C"
{
#endif
    void lume_runtime_construct(void);
    void lume_runtime_destruct(void);
    void lume_runtime_c_initialize(void);
#ifdef __cplusplus
}
#endif
#endif
