#ifndef HM_LIB_UARTI_H_
#define HM_LIB_UARTI_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

int uarti_init();
int uarti_pop(void *msg, k_timeout_t timeout);
int uarti_push(const char *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* HM_LIB_UARTI_H_ */
