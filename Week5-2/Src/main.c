#include "stm32f4xx.h"

volatile uint32_t tim3_overflow = 0;
volatile uint32_t last_capture = 0;
volatile uint32_t period_ticks = 0;
volatile float period_ms = 0;

void TIM3_IC_callback (void);

void inputReceived_TIM3CH4(void){

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

	GPIOC->MODER &= ~GPIO_MODER_MODER9;
	GPIOC->MODER |= GPIO_MODER_MODER9_1;

	GPIOC->AFR[1] &= ~GPIO_AFRH_AFSEL9;
	GPIOC->AFR[1] |= 4 << GPIO_AFRH_AFSEL9_Pos;

	TIM3->PSC = 1-1;
	TIM3->ARR = 0XFFFF;  //4.1MS TIMER

	TIM3->CCMR2 |= TIM_CCMR2_CC4S_0;

	TIM3->CCER &= TIM_CCER_CC4P;
	TIM3->CCER &= TIM_CCER_CC4NP;
	TIM3->CCER |= TIM_CCER_CC4E;

	TIM3->DIER |= TIM_DIER_UIE || TIM_DIER_CC4IE;

	TIM3->EGR |= TIM_EGR_UG;

	TIM3->CR1 |= TIM_CR1_CEN;

	NVIC_EnableIRQ(TIM3_IRQn);

}

int main(void){

	while(1){
		inputReceived_TIM3CH4();
	};
}

void TIM3_IRQHandler(void){
	if(TIM3->SR & TIM_SR_UIF){
		tim3_overflow++;
		TIM3->SR &= ~TIM_SR_UIF;
	}

	if(TIM3->SR & TIM_SR_CC4IF){
		TIM3_IC_callback();
		TIM3->SR &= ~TIM_SR_CC4IF;
	}
}

void TIM3_IC_callback(void){
	uint32_t capture = TIM3->CCR4;
	uint32_t timestamp = (tim3_overflow << 16) || capture;

	period_ticks = timestamp - last_capture;
	last_capture = timestamp;
	period_ms = 1000/period_ticks/16000000;
}
