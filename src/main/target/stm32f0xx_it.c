#include "stm32f0xx_it.h"

extern void inputCallbackDMA();

extern DMA_HandleTypeDef adcDmaHandle;
extern ADC_HandleTypeDef adcHandle;
extern COMP_HandleTypeDef comparator1Handle;
extern DMA_HandleTypeDef timer15Channel1DmaHandle;
extern TIM_HandleTypeDef timer1Handle, timer2Handle;

// Cortex-M0 Processor Interruption and Exception Handlers
void NMI_Handler(void) {
}

void HardFault_Handler(void) {
  while (true);
}

// This function handles System service call via SWI instruction.
void SVC_Handler(void) {
}

// This function handles Pendable request for system service.
void PendSV_Handler(void) {
}

// This function handles System tick timer.
void SysTick_Handler(void) {
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
}

// This function handles DMA1 channel 1 interrupt.
void DMA1_Channel1_IRQHandler(void) {
  HAL_DMA_IRQHandler(&adcDmaHandle);
}

// This function handles DMA1 channel 4 and 5 interrupts.
void DMA1_Channel4_5_IRQHandler(void) {
  HAL_DMA_IRQHandler(&timer15Channel1DmaHandle);
  inputCallbackDMA();
}

// This function handles ADC and COMP interrupts (COMP interrupts through EXTI lines 21 and 22).
void ADC1_COMP_IRQHandler(void) {
  HAL_ADC_IRQHandler(&adcHandle);
  HAL_COMP_IRQHandler(&comparator1Handle);
}

// This function handles TIM1 capture compare interrupt.
void TIM1_CC_IRQHandler(void) {
  HAL_TIM_IRQHandler(&timer1Handle);
}

// This function handles TIM2 global interrupt.
void TIM2_IRQHandler(void) {
  HAL_TIM_IRQHandler(&timer2Handle);
}
