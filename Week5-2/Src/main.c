#include "stm32f4xx.h"

volatile uint32_t tim3_overflow = 0;   // Counts number of timer overflows
volatile uint32_t last_capture = 0;    // Stores previous capture timestamp
volatile uint32_t period_ticks = 0;    // Stores time difference between capture
volatile float period_ms = 0;          // Stores calculated period in milliseconds

// Function prototype for Input Capture callback
void TIM3_IC_callback(void);

// Function to configure Timer 3 Channel 4 for input capture on PC9
void inputReceived_TIM3CH4(void) {
    // Enable clocks for GPIOC and Timer 3
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    // Set PC9 as alternate function mode
    GPIOC->MODER &= ~GPIO_MODER_MODER9;    
    GPIOC->MODER |= GPIO_MODER_MODER9_1;
    // Select AF2 (TIM3) for PC9
    GPIOC->AFR[1] &= ~GPIO_AFRH_AFSEL9;      
    GPIOC->AFR[1] |= 4 << GPIO_AFRH_AFSEL9_Pos;

    // Configure Timer 3
    TIM3->PSC = 1 - 1;  
    TIM3->ARR = 0xFFFF; // Auto-reload value (max count before overflow)

    // Configure channel 4 as input capture
    TIM3->CCMR2 |= TIM_CCMR2_CC4S_0; // CC4 channel mapped to input (TI4)

    // Configure edge polarity and enable capture
    TIM3->CCER &= ~(TIM_CCER_CC4P | TIM_CCER_CC4NP); // Capture on rising edge
    TIM3->CCER |= TIM_CCER_CC4E;                     // Enable capture on CH4

    // Enable update and capture interrupts
    TIM3->DIER |= TIM_DIER_UIE | TIM_DIER_CC4IE;
    // Generate an update event to load the registers
    TIM3->EGR |= TIM_EGR_UG;

    // Enable Timer 3
    TIM3->CR1 |= TIM_CR1_CEN;

    // Enable Timer 3 interrupt in NVIC
    NVIC_EnableIRQ(TIM3_IRQn);
}

int main(void) {
    // Main loop repeatedly calls configuration function
    while(1) {
        inputReceived_TIM3CH4();
    };
}
// Timer 3 Interrupt Handler
void TIM3_IRQHandler(void) {
    // Check if overflow occurred (update event)
    if (TIM3->SR & TIM_SR_UIF) {
        tim3_overflow++;            // Increment overflow counter
        TIM3->SR &= ~TIM_SR_UIF;    // Clear overflow flag
    }

    // Check if capture event occurred on channel 4
    if (TIM3->SR & TIM_SR_CC4IF) {
        TIM3_IC_callback();         // Call function to handle capture
        TIM3->SR &= ~TIM_SR_CC4IF;  // Clear capture flag
    }
}
// Callback function executed when a capture event occurs
void TIM3_IC_callback(void) {
    uint32_t capture = TIM3->CCR4;                    // Read captured timer value
    uint32_t timestamp = (tim3_overflow << 16) | capture;  // Combine overflow count and capture value

    // Calculate period in timer ticks
    period_ticks = timestamp - last_capture;
    last_capture = timestamp;

    // Convert period to milliseconds (assuming 16 MHz clock)
    period_ms = (float)period_ticks / 16000.0;
}
