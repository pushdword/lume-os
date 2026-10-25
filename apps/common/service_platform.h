#ifndef LUME_SERVICE_PLATFORM_H
#define LUME_SERVICE_PLATFORM_H
#include "lume/supervision.h"
#include <stdint.h>

int service_call(unsigned, struct lume_supervision_request *);
uint64_t service_now(void);
void service_sleep(unsigned);
#endif
