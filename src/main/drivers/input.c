#include "input.h"

bool inputArmed, inputDataValid;
uint8_t inputProtocol;
uint32_t inputData;
uint32_t inputNormed, outputPwm;
uint32_t inputArmCounter, inputTimeoutCounter;
uint32_t inputBufferDMA[INPUT_BUFFER_DMA_SIZE];

extern TIM_HandleTypeDef timer15Handle;
extern uint8_t motorDirection;

void inputArmCheck(void) {
  if (!inputArmed) {
    if ((inputProtocol != AUTODETECT) && (inputData < DSHOT_CMD_MAX) && inputDataValid) {
      inputArmCounter++;
      HAL_Delay(1);
      if (inputArmCounter > INPUT_ARM_COUNTER_THRESHOLD) {
        inputArmed = true;
        //debug
        //LED_ON(RED);
        motorInputTune();
      }
    }
  }
}

void inputDisarm(void) {
  inputData = 0;
  inputDataValid = true;

  inputNormed = 0;
  outputPwm = 0;

  inputArmed = false;
  inputArmCounter = 0;
  inputTimeoutCounter = 0;

  //ToDo
  while (HAL_TIM_IC_Stop_DMA(&timer15Handle, TIM_CHANNEL_1) != HAL_OK);
  inputProtocol = AUTODETECT;
  TIM15->PSC = 1;
  TIM15->CNT = 0;
  while (HAL_TIM_IC_Start_DMA(&timer15Handle, TIM_CHANNEL_1, inputBufferDMA, INPUT_BUFFER_DMA_SIZE_AUTODETECT) != HAL_OK);
}

void inputDisarmCheck(void) {
  inputTimeoutCounter++;
  if (inputTimeoutCounter > INPUT_TIMEOUT_COUNTER_THRESHOLD ) {
    inputDisarm();
    //debug
    //LED_OFF(RED);
    //LED_OFF(GREEN);
  }
}

void inputDshotCommandRun(void) {
  if (inputProtocol == PROSHOT) {
    switch (inputData) {
    case DSHOT_CMD_MOTOR_STOP:
      break;
    case DSHOT_CMD_BEACON1:
      motorStartupTune();
      break;
    case DSHOT_CMD_BEACON2:
      motorInputTune();
      break;
    case DSHOT_CMD_SETTING_SPIN_DIRECTION_NORMAL:
      escConfig()->motorDirection = 1;
      inputArmed = false;
      break;
    case DSHOT_CMD_SETTING_SPIN_DIRECTION_REVERSED:
      escConfig()->motorDirection = 0;
      inputArmed = false;
      break;
    case DSHOT_CMD_SPIN_DIRECTION_NORMAL:
      motorDirection = escConfig()->motorDirection;
      break;
    case DSHOT_CMD_SPIN_DIRECTION_REVERSED:
      motorDirection = !escConfig()->motorDirection;
      break;
    case DSHOT_CMD_SETTING_3D_MODE_OFF:
      escConfig()->motor3Dmode = 0;
      inputArmed = false;
      break;
    case DSHOT_CMD_SETTING_3D_MODE_ON:
      escConfig()->motor3Dmode = 1;
      inputArmed = false;
      break;
    case DSHOT_CMD_SETTING_SAVE:
      configWrite();
      // reset esc, iwdg timeout
      while(true);
    default:
      break;
    }
  }
}

void inputCallbackDMA() {
  switch (inputProtocol) {
     case AUTODETECT:
      inputDetectProtocol();
      break;
    case PROSHOT:
      while (HAL_TIM_IC_Stop_DMA(&timer15Handle, TIM_CHANNEL_1) != HAL_OK);
      inputProshot();
      while (HAL_TIM_IC_Start_DMA(&timer15Handle, TIM_CHANNEL_1, inputBufferDMA, INPUT_BUFFER_DMA_SIZE_PROSHOT) != HAL_OK);
      break;
    case SERVOPWM:
      while (HAL_TIM_IC_Stop_DMA(&timer15Handle, TIM_CHANNEL_1) != HAL_OK);
      inputServoPwm();
      while (HAL_TIM_IC_Start_DMA(&timer15Handle, TIM_CHANNEL_1, inputBufferDMA, INPUT_BUFFER_DMA_SIZE_PWM) != HAL_OK);
      break;
  }

}

void inputDetectProtocol() {
  uint32_t telegramPulseWidthBuff;
  uint32_t telegramPulseWidthMin = 20000;

  while (HAL_TIM_IC_Stop_DMA(&timer15Handle, TIM_CHANNEL_1) != HAL_OK);

  for (int i = 0; i < (INPUT_BUFFER_DMA_SIZE_AUTODETECT - 1); i++) {
    telegramPulseWidthBuff = inputBufferDMA[i + 1] - inputBufferDMA[i];
    if(telegramPulseWidthBuff < telegramPulseWidthMin) {
      telegramPulseWidthMin = telegramPulseWidthBuff;
    }
  }

  if ((telegramPulseWidthMin > INPUT_PROSHOT_WIDTH_MIN_SYSTICKS ) && (telegramPulseWidthMin < INPUT_PROSHOT_WIDTH_MAX_SYSTICKS)) {
    inputProtocol = PROSHOT;
    TIM15->PSC = INPUT_PROSHOT_PRESCALER;
    TIM15->CNT = 0;
    while (HAL_TIM_IC_Start_DMA(&timer15Handle, TIM_CHANNEL_1, inputBufferDMA, INPUT_BUFFER_DMA_SIZE_PROSHOT) != HAL_OK);
    return;
  }

  if (telegramPulseWidthMin > 900) {
    inputProtocol = SERVOPWM;
    TIM15->PSC = INPUT_PWM_PRESCALER;
    TIM15->CNT = 0;
    while (HAL_TIM_IC_Start_DMA(&timer15Handle, TIM_CHANNEL_1, inputBufferDMA, INPUT_BUFFER_DMA_SIZE_PWM) != HAL_OK);
    return;
  }

  // default
  if (inputProtocol == AUTODETECT) {
    TIM15->PSC = INPUT_PROSHOT_PRESCALER;
    TIM15->CNT = 0;
    while (HAL_TIM_IC_Start_DMA(&timer15Handle, TIM_CHANNEL_1, inputBufferDMA, INPUT_BUFFER_DMA_SIZE_AUTODETECT) != HAL_OK);
  }
}

void inputProshot() {
  uint8_t telegramCalculatedCRC = 0, telegramReceivedCRC = 0;
  uint16_t telegramData = 0;
  uint32_t telegramPulseValue[4] = {0, 0, 0, 0};

  //debug
  //LED_OFF(GREEN);

  for (int i = 0; i < 4; i++) {
    telegramPulseValue[i] = ( (inputBufferDMA[i*2 + 1] - inputBufferDMA[i*2]) - 23)/3;
  }

  for (int i = 0; i < 4; i++) {
    telegramCalculatedCRC = telegramCalculatedCRC | ((telegramPulseValue[0]^telegramPulseValue[1]^telegramPulseValue[2]) << i);
    telegramReceivedCRC = telegramReceivedCRC | (telegramPulseValue[3] << i);
  }

  telegramData = ((telegramPulseValue[0] << 7 | telegramPulseValue[1] << 3 | telegramPulseValue[2] >> 1));

  if ((telegramCalculatedCRC == telegramReceivedCRC) && (telegramData >= INPUT_VALUE_MIN) && (telegramData <= INPUT_VALUE_MAX)) {
    inputDataValid = true;
    inputTimeoutCounter = 0;
    inputData = telegramData;
    //debug
    //LED_ON(GREEN);
    return;
  } else {
    inputDataValid = false;
    return;
  }
}

void inputServoPwm() {
  uint32_t telegramPulseWidthBuff = 0;

  for (int i = 0; i < (INPUT_BUFFER_DMA_SIZE_PWM - 1); i++) {
    telegramPulseWidthBuff = inputBufferDMA[i + 1] - inputBufferDMA[i];

    if ((telegramPulseWidthBuff >= INPUT_PWM_WIDTH_MIN_US) && (telegramPulseWidthBuff <= INPUT_PWM_WIDTH_MAX_US)) {
      inputDataValid = true;
      inputTimeoutCounter = 0;
      inputData = map(telegramPulseWidthBuff, INPUT_PWM_WIDTH_MIN_US, INPUT_PWM_WIDTH_MAX_US, OUTPUT_PWM_MIN, OUTPUT_PWM_MAX);
      return;
    } else {
      inputDataValid = false;
    }
  }

}
