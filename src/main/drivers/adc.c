#include "adc.h"

ADC_HandleTypeDef adcHandle;
DMA_HandleTypeDef adcDmaHandle;

uint32_t adcValue[3];
uint32_t adcVoltageRaw, adcCurrentRaw, adcTemperatureRaw;


void adcRead(void){
  adcCurrentRaw = adcValue[0];
  adcVoltageRaw = adcValue[1];
  adcTemperatureRaw = adcValue[2];
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* adcHandle) {
  adcRead();
}
