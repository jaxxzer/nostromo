#include "stm32f0xx_hal.h"
#include "target.h"

extern DMA_HandleTypeDef hdma_adc;
extern DMA_HandleTypeDef hdma_tim15_ch1_up_trig_com;

void HAL_MspInit(void) {
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  HAL_NVIC_SetPriority(SVC_IRQn, 0, 0);
  HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc) {
  GPIO_InitTypeDef GPIO_InitStruct;

  if(hadc->Instance == ADC1) {
    __HAL_RCC_ADC1_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_VOLTAGE | GPIO_PIN_CURRENT;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hdma_adc.Instance = DMA1_Channel1;
    hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_adc.Init.Mode = DMA_CIRCULAR;
    hdma_adc.Init.Priority = DMA_PRIORITY_MEDIUM;
    while (HAL_DMA_Init(&hdma_adc) != HAL_OK);

    __HAL_LINKDMA(hadc,DMA_Handle,hdma_adc);
    HAL_NVIC_SetPriority(ADC1_COMP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC1_COMP_IRQn);
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc) {
  if(hadc->Instance == ADC1) {
    __HAL_RCC_ADC1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_VOLTAGE | GPIO_PIN_CURRENT);
    HAL_DMA_DeInit(hadc->DMA_Handle);
  }
}

void HAL_COMP_MspInit(COMP_HandleTypeDef* hcomp) {
  GPIO_InitTypeDef GPIO_InitStruct;

  if(hcomp->Instance == COMP1) {
    /**COMP1 GPIO Configuration
       PA1     ------> COMP1_INP
       PA5     ------> COMP1_INM
     */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(ADC1_COMP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC1_COMP_IRQn);
  }
}

void HAL_COMP_MspDeInit(COMP_HandleTypeDef* hcomp) {

  if(hcomp->Instance == COMP1) {
    /**COMP1 GPIO Configuration
       PA1     ------> COMP1_INP
       PA5     ------> COMP1_INM
     */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_5);
  }

}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base) {
  GPIO_InitTypeDef GPIO_InitStruct;

  if(htim_base->Instance == TIM1) {
    __HAL_RCC_TIM1_CLK_ENABLE();

    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
  }
  else if(htim_base->Instance == TIM2) {
    __HAL_RCC_TIM2_CLK_ENABLE();

    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
  }
  else if(htim_base->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_ENABLE();
  }
  else if(htim_base->Instance == TIM15) {
    __HAL_RCC_TIM15_CLK_ENABLE();
    /**TIM15 GPIO Configuration
       PA2     ------> TIM15_CH1
     */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_TIM15;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TIM15 DMA Init */
    /* TIM15_CH1_UP_TRIG_COM Init */
    hdma_tim15_ch1_up_trig_com.Instance = DMA1_Channel5;
    hdma_tim15_ch1_up_trig_com.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_tim15_ch1_up_trig_com.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim15_ch1_up_trig_com.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim15_ch1_up_trig_com.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_tim15_ch1_up_trig_com.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_tim15_ch1_up_trig_com.Init.Mode = DMA_NORMAL;
    hdma_tim15_ch1_up_trig_com.Init.Priority = DMA_PRIORITY_HIGH;
    while (HAL_DMA_Init(&hdma_tim15_ch1_up_trig_com) != HAL_OK);

    /* Several peripheral DMA handle pointers point to the same DMA handle.
       Be aware that there is only one channel to perform all the requested DMAs. */
    __HAL_LINKDMA(htim_base,hdma[TIM_DMA_ID_CC1],hdma_tim15_ch1_up_trig_com);
    __HAL_LINKDMA(htim_base,hdma[TIM_DMA_ID_UPDATE],hdma_tim15_ch1_up_trig_com);
    __HAL_LINKDMA(htim_base,hdma[TIM_DMA_ID_TRIGGER],hdma_tim15_ch1_up_trig_com);
    __HAL_LINKDMA(htim_base,hdma[TIM_DMA_ID_COMMUTATION],hdma_tim15_ch1_up_trig_com);
  }

}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim) {
  GPIO_InitTypeDef GPIO_InitStruct;

  if(htim->Instance == TIM1) {
    /**TIM1 GPIO Configuration
       PA7     ------> TIM1_CH1N
       PB0     ------> TIM1_CH2N
       PB1     ------> TIM1_CH3N
       PA8     ------> TIM1_CH1
       PA9     ------> TIM1_CH2
       PA10     ------> TIM1_CH3
       PA11     ------> TIM1_CH4 ???
    */

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;

    GPIO_InitStruct.Pin = C_FET_LO_PIN;
    HAL_GPIO_Init(C_FET_LO_GPIO, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = C_FET_HI_PIN;
    HAL_GPIO_Init(C_FET_HI_GPIO, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = B_FET_HI_PIN;
    HAL_GPIO_Init(B_FET_HI_GPIO, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = A_FET_HI_PIN;
    HAL_GPIO_Init(A_FET_HI_GPIO, &GPIO_InitStruct);
    //GPIO_InitStruct.Pin = GPIO_PIN_11;
    //HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = B_FET_LO_PIN;
    HAL_GPIO_Init(B_FET_LO_GPIO, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = A_FET_LO_PIN;
    HAL_GPIO_Init(A_FET_LO_GPIO, &GPIO_InitStruct);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base) {

  if(htim_base->Instance == TIM1) {
    __HAL_RCC_TIM1_CLK_DISABLE();

    HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
  }
  else if(htim_base->Instance == TIM2) {
    __HAL_RCC_TIM2_CLK_DISABLE();

    HAL_NVIC_DisableIRQ(TIM2_IRQn);
  }
  else if(htim_base->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_DISABLE();
  }
  else if(htim_base->Instance == TIM15) {
    __HAL_RCC_TIM15_CLK_DISABLE();

    /**TIM15 GPIO Configuration
       PA2     ------> TIM15_CH1
     */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2);

    HAL_DMA_DeInit(htim_base->hdma[TIM_DMA_ID_CC1]);
    HAL_DMA_DeInit(htim_base->hdma[TIM_DMA_ID_UPDATE]);
    HAL_DMA_DeInit(htim_base->hdma[TIM_DMA_ID_TRIGGER]);
    HAL_DMA_DeInit(htim_base->hdma[TIM_DMA_ID_COMMUTATION]);
  }
}
