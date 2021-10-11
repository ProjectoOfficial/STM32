#ifndef __TIM_H
#define __TIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern void TIM_INIT(void);
extern void HAL_SYSTICK_Callback(void);
extern size_t Millis(void);

#ifdef __cplusplus
}
#endif
#endif //__TIM_H
