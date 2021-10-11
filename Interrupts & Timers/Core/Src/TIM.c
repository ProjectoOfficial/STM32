#include "TIM.h"

size_t timer;

void TIM_INIT(void){
	timer = 0;
}

void HAL_SYSTICK_Callback(void){
	timer++;
}

size_t  Millis(void) { 
	return timer; 
}
