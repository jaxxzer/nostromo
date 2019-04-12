#include "motor.h"

uint32_t timestamp;
uint16_t step = 1;

uint16_t sine_array[20] = {80, 80, 90, 90, 95, 95,95, 100, 100,100, 100, 100, 100, 95, 95, 95, 90, 90, 80, 80};


extern COMP_HandleTypeDef hcomp1;
extern TIM_HandleTypeDef htim1;


extern uint32_t thiszctime, lastzctime, sensorless, commutation_interval;
// set proportianal to commutation time. with advance divisor
extern uint32_t advance;
extern uint32_t blanktime, waitTime, compit;
extern uint32_t filter_level;
extern uint32_t filter_delay;

extern uint32_t zctimeout;
// depends on speed of main loop
extern uint32_t zc_timeout_threshold;
extern uint32_t tim2_start_arr;
extern uint32_t duty_cycle;

//ToDo enum
extern uint8_t pwm;
extern uint8_t floating;
extern uint8_t lowside;

extern uint32_t bemf_counts;

extern uint8_t forward;
extern uint8_t rising;
extern uint8_t running;
extern uint8_t started;
extern uint8_t armed;
extern uint32_t armedcount;


// for complementary pwm , 0 for diode freewheeling
extern uint16_t slow_decay;
// apply full motor brake on stop
extern uint16_t brake;
extern uint16_t start_power;
extern uint16_t prop_brake, prop_brake_active;
extern uint16_t prop_brake_strength;

// increase divisor to decrease advance
extern uint16_t advancedivisor;

extern uint32_t input, newinput;


// phaseB qfnf051 , phase A qfp32
#ifdef MP6531
void phaseA(uint8_t newPhase)
#endif
#ifdef FD6288
void phaseB(uint8_t newPhase)
#endif
{
  if (newPhase == pwm) {
    if(!slow_decay  || prop_brake_active) {
      LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
      B_FET_LO_GPIO->BRR = B_FET_LO_PIN;
    } else {
      LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_ALTERNATE);
    }
    LL_GPIO_SetPinMode(B_FET_HI_GPIO, B_FET_HI_PIN, LL_GPIO_MODE_ALTERNATE);
  }

  if (newPhase == floating) {
    LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    B_FET_LO_GPIO->BRR = B_FET_LO_PIN;
    LL_GPIO_SetPinMode(B_FET_HI_GPIO, B_FET_HI_PIN, LL_GPIO_MODE_OUTPUT);
    B_FET_HI_GPIO->BRR = B_FET_HI_PIN;
  }

  if (newPhase == lowside) {
    LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    B_FET_LO_GPIO->BSRR = B_FET_LO_PIN;
    LL_GPIO_SetPinMode(B_FET_HI_GPIO, B_FET_HI_PIN, LL_GPIO_MODE_OUTPUT);
    B_FET_HI_GPIO->BRR = B_FET_HI_PIN;
  }

}

// phase c qfn , phase b qfp
#ifdef MP6531
void phaseB(uint8_t newPhase)
#endif
#ifdef FD6288
void phaseC(uint8_t newPhase)
#endif
{
  if (newPhase == pwm) {
    if (!slow_decay || prop_brake_active) {
      LL_GPIO_SetPinMode(C_FET_LO_GPIO, C_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
      C_FET_LO_GPIO->BRR = C_FET_LO_PIN;
    } else {
      LL_GPIO_SetPinMode(C_FET_LO_GPIO, C_FET_LO_PIN, LL_GPIO_MODE_ALTERNATE);
    }
    LL_GPIO_SetPinMode(C_FET_HI_GPIO, C_FET_HI_PIN, LL_GPIO_MODE_ALTERNATE);
  }

  if (newPhase == floating) {
    LL_GPIO_SetPinMode(C_FET_LO_GPIO, C_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    C_FET_LO_GPIO->BRR = C_FET_LO_PIN;
    LL_GPIO_SetPinMode(C_FET_HI_GPIO, C_FET_HI_PIN, LL_GPIO_MODE_OUTPUT);
    C_FET_HI_GPIO->BRR = C_FET_HI_PIN;
  }

  if (newPhase == lowside) {
    LL_GPIO_SetPinMode(C_FET_LO_GPIO, C_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    C_FET_LO_GPIO->BSRR = C_FET_LO_PIN;
    LL_GPIO_SetPinMode(C_FET_HI_GPIO, C_FET_HI_PIN, LL_GPIO_MODE_OUTPUT);
    C_FET_HI_GPIO->BRR = C_FET_HI_PIN;
  }
}

// phaseA qfn , phase C qfp
#ifdef MP6531
void phaseC(uint8_t newPhase)
#endif
#ifdef FD6288
void phaseA(uint8_t newPhase)
#endif
{
  if (newPhase == pwm) {
    if (!slow_decay || prop_brake_active) {
      LL_GPIO_SetPinMode(A_FET_LO_GPIO, A_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
      A_FET_LO_GPIO->BRR = A_FET_LO_PIN;
    } else {
      LL_GPIO_SetPinMode(A_FET_LO_GPIO, A_FET_LO_PIN, LL_GPIO_MODE_ALTERNATE);
    }
    LL_GPIO_SetPinMode(A_FET_HI_GPIO, A_FET_HI_PIN, LL_GPIO_MODE_ALTERNATE);
  }

  if (newPhase == floating) {
    LL_GPIO_SetPinMode(A_FET_LO_GPIO, A_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    A_FET_LO_GPIO->BRR = A_FET_LO_PIN;
    LL_GPIO_SetPinMode(A_FET_HI_GPIO, A_FET_HI_PIN, LL_GPIO_MODE_OUTPUT);
    A_FET_HI_GPIO->BRR = A_FET_HI_PIN;
  }

  if (newPhase == lowside) {
    LL_GPIO_SetPinMode(A_FET_LO_GPIO, A_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    A_FET_LO_GPIO->BSRR = A_FET_LO_PIN;
    LL_GPIO_SetPinMode(A_FET_HI_GPIO, A_FET_HI_PIN, LL_GPIO_MODE_OUTPUT);
    A_FET_HI_GPIO->BRR = A_FET_HI_PIN;
  }

}

void commutationStep(uint8_t newStep) {
  //A-B
  if (newStep == 1) {
    phaseA(pwm);
    phaseB(lowside);
    phaseC(floating);
  }
  // C-B
  if (newStep == 2) {
    phaseA(floating);
    phaseB(lowside);
    phaseC(pwm);
  }
  // C-A
  if (newStep == 3) {
    phaseA(lowside);
    phaseB(floating);
    phaseC(pwm);
  }
  // B-A
  if (newStep == 4) {
    phaseA(lowside);
    phaseB(pwm);
    phaseC(floating);
  }
  // B-C
  if (newStep == 5) {
    phaseA(floating);
    phaseB(pwm);
    phaseC(lowside);
  }
  // A-C
  if (newStep == 6) {
    phaseA(pwm);
    phaseB(floating);
    phaseC(lowside);
  }
}

void allOff() {
  phaseA(floating);
  phaseB(floating);
  phaseC(floating);
}

void fullBrake() {
  phaseA(lowside);
  phaseB(lowside);
  phaseC(lowside);
}

// duty cycle controls braking strength
// will turn off lower fets so only high side is active
void proBrake() {
  phaseA(pwm);
  phaseB(pwm);
  phaseC(pwm);
}


void changeCompInput() {
  // c floating
  if (step == 1 || step == 4) {
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_IO1;
  }
  // a floating
  if (step == 2 || step == 5) {
    // if f051k6  step 2 , 5 is dac 1 ( swap comp input)
    #ifdef MP6531
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC1;
    #endif
    #ifdef FD6288
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC2;
    #endif
  }
  // b floating
  if (step == 3 || step == 6) {
    #ifdef MP6531
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC2;
    #endif
    #ifdef FD6288
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC1;
    #endif
  }
  if (rising) {
    // polarity of comp output reversed
    hcomp1.Init.TriggerMode = COMP_TRIGGERMODE_IT_FALLING;
  }else{
    // falling bemf
    hcomp1.Init.TriggerMode = COMP_TRIGGERMODE_IT_RISING;
  }

  while (HAL_COMP_Init(&hcomp1) != HAL_OK);
}


void commutate() {
  if (forward == 1) {
    step++;
    if (step > 6) {
      step = 1;
    }
    if (step == 1 || step == 3 || step == 5) {
      rising = 1;                                // is back emf rising or falling
    }
    if (step == 2 || step == 4 || step == 6) {
      rising = 0;
    }
  }
  if (forward == 0) {
    step--;
    if (step < 1) {
      step = 6;
    }
    if (step == 1 || step == 3 || step == 5) {
      rising = 0;
    }
    if (step == 2 || step == 4 || step == 6) {
      rising = 1;
    }
  }

  if (input > 47) {
    commutationStep(step);
  }
  changeCompInput();
// TIM2->CNT = 0;
// TIM2->ARR = commutation_interval;
}


// for forced commutation -- open loop
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
  }
}

void startMotor() {
  uint16_t decaystate = slow_decay;
  sensorless = 0;
  if (running == 0) {
    HAL_COMP_Stop_IT(&hcomp1);
    slow_decay = 1;

    commutate();
    commutation_interval = tim2_start_arr- 3000;
    TIM3->CNT = 0;
    running = 1;
    while (HAL_COMP_Start_IT(&hcomp1) != HAL_OK);
  }

  slow_decay = decaystate;    // return to normal
  sensorless = 1;
  bemf_counts = 0;

}

void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp) {
  timestamp = TIM3->CNT;
  LED_ON(LED0);

  if (compit > 200) {
    HAL_COMP_Stop_IT(&hcomp1);
    return;
  }
  compit +=1;
  while (TIM3->CNT - timestamp < filter_delay);

  if (rising) {
    // advancedivisor = advancedivisorup;
    for (int i = 0; i < filter_level; i++) {
      if (HAL_COMP_GetOutputLevel(&hcomp1) == COMP_OUTPUTLEVEL_HIGH) {
        return;
      }
    }

  } else {
    // advancedivisor = advancedivisordown;
    for (int i = 0; i < filter_level; i++) {
      if (HAL_COMP_GetOutputLevel(&hcomp1) == COMP_OUTPUTLEVEL_LOW) {
        return;
      }
    }

  }
  thiszctime = timestamp;
  TIM3->CNT = 0;
  HAL_COMP_Stop_IT(&hcomp1);

  zctimeout = 0;

  // TEST!   divide by two when tracking up down time independant
  commutation_interval = (commutation_interval + thiszctime) / 2;

  advance = commutation_interval / advancedivisor;
  waitTime = commutation_interval /2    - advance;
  blanktime = commutation_interval / 4;

  if (sensorless) {
    while (TIM3->CNT  < waitTime) {
    }

    compit = 0;
    commutate();
    while (TIM3->CNT  < waitTime + blanktime) {
    }
  }

  lastzctime = thiszctime;
  bemf_counts++;

  while (HAL_COMP_Start_IT(&hcomp1) != HAL_OK);
}


void changeDutyCycleWithSin() {
  if (!rising) {
    // last ten elements in sin array
    duty_cycle = (duty_cycle * sine_array[((TIM2->CNT*10)/TIM2->ARR)+9])/100;
  }else{
    // first ten elements in sin array
    duty_cycle = (duty_cycle * sine_array[(TIM2->CNT*10)/TIM2->ARR])/100;
  }

  TIM1->CCR1 = duty_cycle;
  TIM1->CCR2 = duty_cycle;
  TIM1->CCR3 = duty_cycle;
}

void zc_found_routine() {
  zctimeout = 0;

  thiszctime = TIM3->CNT;

  if (thiszctime < lastzctime) {
    lastzctime = lastzctime - 65535;
  }

  if (thiszctime > lastzctime) {
    //if (((thiszctime - lastzctime) > (commutation_interval * 2)) || ((thiszctime - lastzctime < commutation_interval/2))){
    //  commutation_interval = (commutation_interval * 3 + (thiszctime - lastzctime))/4;
    //  commutation_interval = (commutation_interval + (thiszctime - lastzctime))/2;
    //}else{
    commutation_interval = (thiszctime - lastzctime);       // TEST!   divide by two when tracking up down time independant
    //	}
    advance = commutation_interval / advancedivisor;
    waitTime = commutation_interval /2 - advance;
  }
  if (sensorless) {
    while (TIM3->CNT - thiszctime < waitTime) {
    }
    commutate();
    while (TIM3->CNT - thiszctime < waitTime + blanktime) {
    }
  }

  lastzctime = thiszctime;
}

void playStartupTune() {
  TIM1->PSC = 75;
  TIM1->CCR1 = 5;
  TIM1->CCR2 = 5;
  TIM1->CCR3 = 5;
  commutationStep(2);
  HAL_Delay(100);
  TIM1->PSC = 50;
  HAL_Delay(100);
  TIM1->PSC = 25;
  HAL_Delay(100);
  allOff();
  TIM1->PSC = 0;
}

void playInputTune() {
  TIM1->PSC = 100;
  TIM1->CCR1 = 5;
  TIM1->CCR2 = 5;
  TIM1->CCR3 = 5;
  commutationStep(2);
  HAL_Delay(100);
  TIM1->PSC = 50;
  HAL_Delay(100);
  allOff();
  TIM1->PSC = 0;
}