#include "main.h"

ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;
COMP_HandleTypeDef hcomp1;
IWDG_HandleTypeDef hiwdg;
TIM_HandleTypeDef htim1, htim2, htim3, htim15;
DMA_HandleTypeDef hdma_tim15_ch1_up_trig_com;

uint16_t VirtAddVarTab[NB_OF_VAR] = {0x5555, 0x6666, 0x7777};
uint16_t VarDataTab[NB_OF_VAR] = {0, 0, 0};
uint16_t VarValue,VarDataTmp;

uint32_t ee_status;

// variables to use for eeprom function,  ie: EE_WriteVariable(VirtAddVarTab[EEvehiclemode],  vehicle_mode ;
enum userVars {
  EEvehiclemode = 0,
  EEdirection = 1,
  EEbidirection = 2,
//	EEbrake_on_stop = 3
};

int dead_time = 60;           // change to 60 for qfn

int dir_reversed = 0;   // global direction reversed set in eeprom
int step = 1;
int pot = 1000;

int smoothedinput = 0;
const int numReadings = 1000;     // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;
char bad_commutation = 0;


char vehicle_mode = 1;                        // 1 = quad mode / eeprom load mode , 2 = crawler / thruster mode,  3 = rc car mode,  4 = like car mode but with auto reverse after stop

int bi_direction = 0;
char slow_decay = 1;                      // for complementary pwm , 0 for diode freewheeling
int brake = 1;                          // apply full motor brake on stop
int start_power = 150;
char prop_brake = 0;
int prop_brake_strength = 300;


char prop_brake_active = 0;
int adjusted_input;

int dshotcommand = 0;
uint8_t calcCRC;
uint8_t checkCRC;
char error = 0;

int quietmode = 0;
int sine_array[20] = {80, 80, 90, 90, 95, 95,95, 100, 100,100, 100,100, 100,95,95,95,90,90,80,80};

int count = 0;
int compCount = 0;
int upcompCount = 0;
int falsecount = 0;
int falseAlarm = 0;
int comp_int_count = 0;
int tempbrake = 0;

int falsethreshold = 2;
int upthiszctime = 0;
int uplastzctime = 0;


char advancedivisor = 8;                    // increase divisor to decrease advance,
char advancedivisorup = 3;
char advancedivisordown = 3;


int thiszctime = 0;
int lastzctime = 0;
int sensorless = 0;
int commutation_interval = 0;
int advance = 0;                       // set proportianal to commutation time. with advance divisor
int blanktime;
int pwm_settle = 50;
int demagtime = 5;
int waitTime = 0;
char filter_level = 1;
char compit = 0;
int filter_delay = 2;

int filter_level_up = 8;
int filter_level_down = 8;



int inputcapture = 0;

int control_loop_count;
int zctimeout = 0;
int zc_timeout_threshold = 2000;   // depends on speed of main loop

int signaltimeout = 0;

int signal_timeout_threshold = 10000;

int temp_step;
int ROC = 1;

int zcfound = 1;
int threshold = 1;
int upthreshold = 1;
int tim2_start_arr = 9000;
int startupcountdown = 0;

int duty_cycle = 100;

int pwm = 1;
int floating = 2;
int lowside = 3;
int bemf_counts;

int forward = 1;
int rising = 1;
int running = 0;
int started = 0;
char armed = 0;
int armedcount = 0;

int input_buffer_size = 64;
int smallestnumber = 20000;
uint32_t dma_buffer[64];
int propulse[4] = {0,0,0,0};
int dpulse[16] =  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int input = 0;
int newinput =0;

int voltageraw = 0;
int currentraw = 0;
uint32_t ADC1ConvertedValues[2] = {0,0};
int timestamp;


char dshot = 0;
char proshot = 0;
char multishot = 0;
char oneshot42 = 0;
char oneshot125 = 0;
char servoPwm = 0;


char inputSet = 0;


// prototypes
void SystemClock_Config(void);

static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_COMP1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM15_Init(void);
static void MX_IWDG_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);


long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  if (x < in_min) {
    x = in_min;
  }
  if (x > in_max) {
    x = in_max;
  }
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

}


void storeEEpromConfig(){
  EE_WriteVariable(VirtAddVarTab[EEvehiclemode], vehicle_mode);
  EE_WriteVariable(VirtAddVarTab[EEdirection], dir_reversed);
  EE_WriteVariable(VirtAddVarTab[EEbidirection], bi_direction);
  // EE_WriteVariable(VirtAddVarTab[EEbrake_on_stop], EEbrake_on_stop);
  // playEEpromSavedTune();
}


void loadEEpromConfig(){
  EE_ReadVariable(VirtAddVarTab[EEvehiclemode], &VarDataTab[EEvehiclemode]);
  EE_ReadVariable(VirtAddVarTab[EEdirection], &VarDataTab[EEdirection]);
  EE_ReadVariable(VirtAddVarTab[EEbidirection], &VarDataTab[EEbidirection]);
//	 EE_ReadVariable(VirtAddVarTab[EEbrake_on_stop], &VarDataTab[EEbrake_on_stop]);

  if (VarDataTab[EEvehiclemode] == 0) {             // nothing in the eeprom
    storeEEpromConfig();             // store default values
  }else{
    vehicle_mode = VarDataTab[EEvehiclemode];
    dir_reversed = VarDataTab[EEdirection];
    bi_direction = VarDataTab[EEbidirection];
//	 brake = VarDataTab[EEbrake_on_stop];
  }
}


// phaseB qfnf051 , phase A qfp32
#ifdef MP6531
void phaseA(int newPhase)
#endif
#ifdef FD6288
void phaseB(int newPhase)
#endif

{
  if (newPhase == pwm) {
    if(!slow_decay  || prop_brake_active) {            // for future
      LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
      GPIOB->BRR = B_FET_LO_PIN;
    }else{
      LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_ALTERNATE); // low
    }
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);  // high

  }

  if (newPhase == floating) {
    LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    GPIOB->BRR = B_FET_LO_PIN;
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_9, LL_GPIO_MODE_OUTPUT);
    GPIOA->BRR = GPIO_PIN_9;
  }

  if (newPhase == lowside) {          // low mosfet on
    LL_GPIO_SetPinMode(B_FET_LO_GPIO, B_FET_LO_PIN, LL_GPIO_MODE_OUTPUT);
    GPIOB->BSRR = B_FET_LO_PIN;
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_9, LL_GPIO_MODE_OUTPUT);
    GPIOA->BRR = GPIO_PIN_9;
  }

}

// phase c qfn , phase b qfp
#ifdef MP6531
void phaseB(int newPhase)
#endif
#ifdef FD6288
void phaseC(int newPhase)
#endif
{
  if (newPhase == pwm) {  // pwm
    if (!slow_decay || prop_brake_active) {
      LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
      GPIOA->BRR = GPIO_PIN_7;
    }else{
      LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    }
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);

  }

  if (newPhase == floating) {            // floating
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
    GPIOA->BRR = GPIO_PIN_7;
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_8, LL_GPIO_MODE_OUTPUT);
    GPIOA->BRR = GPIO_PIN_8;
  }

  if (newPhase == lowside) {              // lowside
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
    GPIOA->BSRR = GPIO_PIN_7;
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_8, LL_GPIO_MODE_OUTPUT);
    GPIOA->BRR = GPIO_PIN_8;
  }
}

// phaseA qfn , phase C qfp
#ifdef MP6531
void phaseC(int newPhase)
#endif
#ifdef FD6288
void phaseA(int newPhase)
#endif
{
  if (newPhase == pwm) {
    if (!slow_decay || prop_brake_active) {
      LL_GPIO_SetPinMode(GPIOB, GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
      GPIOB->BRR = GPIO_PIN_1;
    }else{
      LL_GPIO_SetPinMode(GPIOB, GPIO_PIN_1, LL_GPIO_MODE_ALTERNATE);
    }
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_10, LL_GPIO_MODE_ALTERNATE);

  }

  if (newPhase == floating) {
    LL_GPIO_SetPinMode(GPIOB, GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
    GPIOB->BRR = GPIO_PIN_1;
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_10, LL_GPIO_MODE_OUTPUT);
    GPIOA->BRR = GPIO_PIN_10;
  }

  if (newPhase == lowside) {
    LL_GPIO_SetPinMode(GPIOB, GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
    GPIOB->BSRR = GPIO_PIN_1;
    LL_GPIO_SetPinMode(GPIOA, GPIO_PIN_10, LL_GPIO_MODE_OUTPUT);
    GPIOA->BRR = GPIO_PIN_10;
  }

}

void comStep(int newStep) {
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
//	prop_brake_active = 1;
  phaseA(pwm);
  phaseB(pwm);
  phaseC(pwm);
}


void changeCompInput() {
//	HAL_COMP_Stop_IT(&hcomp1);            // done in comparator routine

  if (step == 1 || step == 4) {   // c floating
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_IO1;
  }

  if (step == 2 || step == 5) {     // a floating
#ifdef MP6531
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC1;                      /// if f051k6  step 2 , 5 is dac 1 ( swap comp input)
#endif
#ifdef FD6288
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC2;
#endif
  }

  if (step == 3 || step == 6) {      // b floating
#ifdef MP6531
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC2;
#endif
#ifdef FD6288
    hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC1;
#endif
  }
  if (rising) {
    hcomp1.Init.TriggerMode = COMP_TRIGGERMODE_IT_FALLING;   // polarity of comp output reversed
  }else{                          // falling bemf
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
    comStep(step);
  }
  changeCompInput();
// zcfound = 0;
// falseAlarm = 0;
// compCount = 0;
// upcompCount = 0;
// TIM2->CNT = 0;
// TIM2->ARR = commutation_interval;
}


// for forced commutation -- open loop
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
  }
}

void startMotor() {
  char decaystate = slow_decay;
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
  startupcountdown =0;
  bemf_counts = 0;

}

void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp) {
  timestamp = TIM3->CNT;
  //LED_ON(LED0);

  if (compit > 200) {
    HAL_COMP_Stop_IT(&hcomp1);
    error = 1;
    return;
  }
  compit +=1;
  while (TIM3->CNT - timestamp < filter_delay) {

  }

  if (rising) {
    //	advancedivisor = advancedivisorup;
    for (int i = 0; i < filter_level; i++) {
      if (HAL_COMP_GetOutputLevel(&hcomp1) == COMP_OUTPUTLEVEL_HIGH) {
        return;
      }
    }


  }else{
    //	advancedivisor = advancedivisordown;
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

  commutation_interval = (commutation_interval + thiszctime) / 2;           // TEST!   divide by two when tracking up down time independant
  bad_commutation = 0;
//			}
  advance = commutation_interval / advancedivisor;
  waitTime = commutation_interval /2    - advance;
  blanktime = commutation_interval / 4;

  if(tempbrake) {
    HAL_COMP_Stop_IT(&hcomp1);
    return;
  }
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

void playStartupTune(){
  TIM1->PSC = 75;
  TIM1->CCR1 = 5;
  TIM1->CCR2 = 5;
  TIM1->CCR3 = 5;
  comStep(2);
  HAL_Delay(100);
  TIM1->PSC = 50;
  HAL_Delay(100);
  TIM1->PSC = 25;
  HAL_Delay(100);
  allOff();
  TIM1->PSC = 0;
}

void playInputTune(){
  TIM1->PSC = 100;
  TIM1->CCR1 = 5;
  TIM1->CCR2 = 5;
  TIM1->CCR3 = 5;
  comStep(2);
  HAL_Delay(100);
  TIM1->PSC = 50;
  HAL_Delay(100);
  allOff();
  TIM1->PSC = 0;
}

void getADCs(){
  voltageraw = ADC1ConvertedValues[0];
  currentraw = ADC1ConvertedValues[1];
  //pot = ADC1ConvertedValues[2];

}




void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  getADCs();
}


void detectInput(){
  smallestnumber = 20000;
  dshot = 0;
  proshot = 0;
  multishot = 0;
  oneshot42 = 0;
  oneshot125 = 0;
  servoPwm = 0;
  int lastnumber = dma_buffer[0];
  for ( int j = 1; j < input_buffer_size; j++) {

    if((dma_buffer[j] - lastnumber) < smallestnumber) { // blank space
      smallestnumber = dma_buffer[j] - lastnumber;

    }
    lastnumber = dma_buffer[j];
  }

  if ((smallestnumber > 3)&&(smallestnumber < 22)) {
    dshot = 1;
  }
  if ((smallestnumber > 40 )&&(smallestnumber < 80)) {
    proshot = 1;
    TIM15->PSC=1;
    TIM15->CNT = 0xffff;

  }
  if ((smallestnumber > 100 )&&(smallestnumber < 400)) {
    multishot = 1;
  }
//	if ((smallestnumber > 2000 )&&(smallestnumber < 3000)){
//		oneshot42 = 1;
//	}
//	if ((smallestnumber > 3000 )&&(smallestnumber < 7000)){
//		oneshot125 = 1;
//	}
  if (smallestnumber > 2000) {
    servoPwm = 1;
    TIM15->PSC = 47;
    TIM15->CNT = 0xffff;
  }

  if (smallestnumber == 0) {
    inputSet = 0;
  }else{

    inputSet = 1;

    HAL_Delay(50);
    //	playInputTune();
  }
  HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 64);
}

void computeProshotDMA(){
  int lastnumber = dma_buffer[0];
  for ( int j = 1; j < 9; j++) {

    if(((dma_buffer[j] - lastnumber) > 1500) && ((dma_buffer[j] - lastnumber) < 50000)) { // blank space
      if ((dma_buffer[j+7] - dma_buffer[j])<10000) {
        //			for ( int i = 0; i < 8; i+= 2){
        //
        //			propulse[i>>1] =map((dma_buffer[j+i+1] - dma_buffer[j+i]),48, 141, 0, 15);
        //			}

        //		for ( int i = 0; i < 8; i+= 2){
        //			 propulse[i>>1] = ((dma_buffer[j+i+1] - dma_buffer[j+i]) - 46)*11>>6;
        //		}
        for (int i = 0; i < 4; i++) {

          propulse[i] = (((dma_buffer[j + i*2 +1] - dma_buffer[j + i*2])) - 23)/3;


        }

        calcCRC = ((propulse[0]^propulse[1]^propulse[2])<<3
                   |(propulse[0]^propulse[1]^propulse[2])<<2
                   |(propulse[0]^propulse[1]^propulse[2])<<1
                   |(propulse[0]^propulse[1]^propulse[2]));
        checkCRC = (propulse[3]<<3 | propulse[3]<<2 | propulse[3]<<1 | propulse[3]);
      }


      if (calcCRC == checkCRC) {
        int tocheck = ((propulse[0]<<7 | propulse[1]<<3 | propulse[2]>>1));
        if (tocheck > 2047 || tocheck < 0) {
          break;
        }else{
          if(tocheck > 47) {
            LED_ON(LED1);
            newinput = tocheck;
            dshotcommand = 0;
          }
          if ((tocheck <= 47)&& (tocheck > 0)) {
            newinput = 0;
            dshotcommand = tocheck;  //  todo
          }
          if (tocheck == 0) {
            newinput = 0;
            dshotcommand = 0;
          }
        }
      }
      break;
    }
    lastnumber = dma_buffer[j];
  }
}

void computeMSInput(){

  int lastnumber = dma_buffer[0];
  for ( int j = 1; j < 2; j++) {

    if(((dma_buffer[j] - lastnumber) < 1500) && ((dma_buffer[j] - lastnumber) > 0)) { // blank space

      newinput = map((dma_buffer[j] - lastnumber),243,1200, 0, 2000);
      break;
    }
    lastnumber = dma_buffer[j];
  }
}

void computeOS125Input(){

  int lastnumber = dma_buffer[0];
  for ( int j = 1; j < 2; j++) {

    if(((dma_buffer[j] - lastnumber) < 12300) && ((dma_buffer[j] - lastnumber) > 0)) { // blank space

      newinput = map((dma_buffer[j] - lastnumber),6500,12000, 0, 2000);
      break;
    }
    lastnumber = dma_buffer[j];
  }
}

void computeOS42Input(){

  int lastnumber = dma_buffer[0];
  for ( int j = 1; j < 2; j++) {

    if(((dma_buffer[j] - lastnumber) < 4500) && ((dma_buffer[j] - lastnumber) > 0)) { // blank space

      newinput = map((dma_buffer[j] - lastnumber),2020, 4032, 0, 2000);
      break;
    }
    lastnumber = dma_buffer[j];
  }
}




void computeServoInput(){

  int lastnumber = dma_buffer[0];
  for ( int j = 1; j < 3; j++) {

    if(((dma_buffer[j] - lastnumber) >1000 ) && ((dma_buffer[j] - lastnumber) < 2010)) { // blank space

      newinput = map((dma_buffer[j] - lastnumber), 1090, 2000, 0, 2000);
      break;
    }
    lastnumber = dma_buffer[j];
  }
}


void computeDshotDMA(){

  int lastnumber = dma_buffer[0];
  for ( int j = 1; j < input_buffer_size; j++) {

    if(((dma_buffer[j] - lastnumber) > 50) && ((dma_buffer[j] - lastnumber) < 65000)) { // blank space

      for (int i = 0; i < 16; i++) {
        dpulse[i] = ((dma_buffer[j + i*2 +1] - dma_buffer[j + i*2]) / 13) - 1;
      }

      uint8_t calcCRC = ((dpulse[0]^dpulse[4]^dpulse[8])<<3
                         |(dpulse[1]^dpulse[5]^dpulse[9])<<2
                         |(dpulse[2]^dpulse[6]^dpulse[10])<<1
                         |(dpulse[3]^dpulse[7]^dpulse[11])
                         );
      uint8_t checkCRC = (dpulse[12]<<3 | dpulse[13]<<2 | dpulse[14]<<1 | dpulse[15]);
      //

      int tocheck = (
        dpulse[0]<<10 | dpulse[1]<<9 | dpulse[2]<<8 | dpulse[3]<<7
          | dpulse[4]<<6 | dpulse[5]<<5 | dpulse[6]<<4 | dpulse[7]<<3
          | dpulse[8]<<2 | dpulse[9]<<1 | dpulse[10]);

      if(calcCRC == checkCRC) {

        if (tocheck > 47) {
          newinput = tocheck;
          dshotcommand = 0;
        }
      }
      if ((tocheck <= 47)&& (tocheck > 0)) {
        newinput = 0;
        dshotcommand = tocheck;    //  todo
      }
      if (tocheck == 0) {
        newinput = 0;
        dshotcommand = 0;
      }



      break;
    }
    lastnumber = dma_buffer[j];
  }
}

void transferComplete(){
  //	TIM15->CNT = 1;
//	compit = 0;
  signaltimeout = 0;
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_3);


  if (inputSet == 1) {
    if (dshot == 1) {
      computeDshotDMA();
      HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 64);
    }
    if (proshot == 1) {
      computeProshotDMA();
      HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 16);
    }

    if  (servoPwm == 1) {
      computeServoInput();
      HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 3);

    }
    if  (multishot) {
      computeMSInput();
      HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 3);

    }
    if  (oneshot125) {
      computeOS125Input();
      HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 3);

    }
    if  (oneshot42) {
      computeOS42Input();
      HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 3);

    }
  }
}


void changeDutyCycleWithSin(){

  if (!rising) {
    duty_cycle = (duty_cycle * sine_array[((TIM2->CNT*10)/TIM2->ARR)+9])/100;        // last ten elements in sin array
  }else{
    duty_cycle = (duty_cycle * sine_array[(TIM2->CNT*10)/TIM2->ARR])/100;         // first ten elements in sin array
  }

  TIM1->CCR1 = duty_cycle;
  TIM1->CCR2 = duty_cycle;
  TIM1->CCR3 = duty_cycle;

}

void zc_found_routine(){
  zcfound = 1;
  zctimeout = 0;
  compCount = 0;
  upcompCount = 0;

  thiszctime = TIM3->CNT;

  if (thiszctime < lastzctime) {
    lastzctime = lastzctime - 65535;
  }

  if (thiszctime > lastzctime) {
//		if (((thiszctime - lastzctime) > (commutation_interval * 2)) || ((thiszctime - lastzctime < commutation_interval/2))){
//		//	commutation_interval = (commutation_interval * 3 + (thiszctime - lastzctime))/4;
//			commutation_interval = (commutation_interval + (thiszctime - lastzctime))/2;
//		}else{
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

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  HAL_FLASH_Unlock();
  EE_Init();
//  HAL_Delay(10);
/* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  ledInit();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_COMP1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM15_Init();
  MX_IWDG_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);


  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start(&htim3);
  //  HAL_Delay(500);
  playStartupTune();


  while (HAL_TIM_OC_Start_IT(&htim1, TIM_CHANNEL_4) != HAL_OK);
  HAL_TIM_IC_Start_DMA(&htim15, TIM_CHANNEL_1, dma_buffer, 64);
	//while (HAL_ADC_Start_DMA(&hadc, (uint32_t*)ADC1ConvertedValues, 2) != HAL_OK);
  while (HAL_COMP_Start_IT(&hcomp1) != HAL_OK);
  while (HAL_IWDG_Init(&hiwdg) != HAL_OK);

  if (vehicle_mode == 1) {                    // quad single direction
    loadEEpromConfig();
  }
  if (vehicle_mode == 2) {                   // crawler or thruster
    bi_direction = 1;
    slow_decay = 1;                       // for complementary pwm , 0 for diode freewheeling
    brake = 1;                           // apply full motor brake on stop
    start_power = 150;

  }
  if (vehicle_mode == 3) {                 // rc car 50 percent brake on reverse.
    bi_direction = 1;
    slow_decay = 0;                       // for complementary pwm , 0 for diode freewheeling
    brake = 0;                           // apply full motor brake on stop
    //	 start_power = 150;
    prop_brake = 1;
    prop_brake_strength = 900;
  }

  if (vehicle_mode == 4) {                 // rc car 50 percent brake on reverse.
    bi_direction = 1;
    slow_decay = 0;                         // for complementary pwm , 0 for diode freewheeling
    brake = 0;                             // apply full motor brake on stop
    //		 start_power = 150;
    prop_brake = 1;
    prop_brake_strength = 800;
  }


  if(bi_direction) {
    newinput = 1001;
    //	start_power = 175;
  }

  TIM1->CCR1 = 1;                       // set duty cycle to 50 out of 768 to start.
  TIM1->CCR2 = 1;
  TIM1->CCR3 = 1;

  TIM1->CCR4 = 800;
  /* USER CODE END 2 */

  /* Infinite loop */
  while (true) {
    LED_OFF(LED0);
    LED_OFF(LED1);

    count++;
    if (count > 100000) {
      count = 0;
    }

    compit = 0;

    while (HAL_IWDG_Refresh(&hiwdg) != HAL_OK);

    control_loop_count++;
    if (control_loop_count > 1) {
      control_loop_count = 0;



//	      1-5: beep (1= low freq. 5 = high freq.)
//	      6: ESC info request (FW Version and SN sent over the tlm wire)
//	      7: rotate in one direction
//	      8: rotate in the other direction
//	      9: 3d mode off
//	      10: 3d mode on
//	      11: ESC settings request (saved settings over the TLM wire) (planed but not there yet)
//	      12: save Settings


      if (dshotcommand > 0) {


        if (dshotcommand == 2) {
          playInputTune();
        }
        if (dshotcommand == 21) {
          forward =  dir_reversed;

        }
        if (dshotcommand == 20) {         // forward = 1 if dir_reversed = 0
          forward = 1 - dir_reversed;
        }
        if (dshotcommand == 7) {
          dir_reversed = 0;

        }
        if (dshotcommand == 8) {
          dir_reversed = 1;

        }
        if (dshotcommand == 9) {
          bi_direction = 0;
          armed = 0;

        }
        if (dshotcommand == 10) {
          bi_direction = 1;
          armed = 0;
        }
        if (dshotcommand == 12) {
          storeEEpromConfig();
          while(1) { // resets esc as iwdg times out

          }
        }
      }


      if (bi_direction == 1 && (proshot == 0 && dshot == 0)) {
        //char oldbrake = brake;

        if ( newinput > 1100 ) {
          if (forward == dir_reversed) {
            adjusted_input = 0;
            prop_brake_active = 1;
            forward = 1 - dir_reversed;
            //	HAL_Delay(1);
          }

          if (prop_brake_active == 0) {

            adjusted_input = (newinput - 1050)*3;
            //	tempbrake = 0;
            //	}
          }
        }


        if (newinput < 800) {
          if (forward == (1 - dir_reversed)) {
            prop_brake_active = 1;
            adjusted_input = 0;
            forward = dir_reversed;
            //	HAL_Delay(1);

          }

          if (prop_brake_active == 0) {
            adjusted_input = (800 - newinput) * 3;

          }
          //	tempbrake = 0;



        }

        if (zctimeout >= zc_timeout_threshold) {
          //	adjusted_input = 0;
          if (vehicle_mode != 3) {                // car mode requires throttle return to center before direction change
            prop_brake_active = 0;
          }

          startupcountdown = 0;
          bemf_counts = 0;
        }

        if (newinput > 800 && newinput < 1100) {
          adjusted_input = 0;
          prop_brake_active = 0;
        }

      }else if((proshot || dshot ) && bi_direction) {
        if ( newinput > 1097 ) {

          if (forward == dir_reversed) {
            forward = 1 - dir_reversed;
            bemf_counts =0;
          }
          adjusted_input = (newinput - 1100) * 2 + 100;
        } if ( newinput <= 1047 &&  newinput > 0) {
          if(forward == (1 - dir_reversed)) {
            bemf_counts =0;
            forward = dir_reversed;
          }
          adjusted_input = (newinput - 90) * 2;
        }
        if ((newinput > 1047 && newinput < 1098 ) || newinput <= 120) {
          adjusted_input = 0;
        }

      }else{
        adjusted_input = newinput;
      }

      //	if ((adjusted_input > 100)&&(startupcountdown>250)){
      if (adjusted_input > 2000) {
        adjusted_input = 2000;
      }


      if (adjusted_input - input > 25) {
        input = input + 5;
      }else{
        input = adjusted_input;
      }

      if (adjusted_input <= input) {

        input = adjusted_input;

      }

    }

    advancedivisor = map((commutation_interval),100,5000, 2, 20);
    if (inputSet == 0) {
      HAL_Delay(10);
      detectInput();

    }

    if (!armed) {
      if ((inputSet == 1)&&(input == 0)) {
        armedcount++;
        HAL_Delay(1);
        if (armedcount > 1000) {
          armed = 1;
          playInputTune();
        }
      }
      if (input > 1) {
        armedcount = 0;
      }
    }


    if ((input > 47)&&(armed == 1)) {
      prop_brake_active = 0;
      started = 1;

      duty_cycle = input / 2 - 10;

      if (bemf_counts < 15) {
        if(duty_cycle < 70) {
          duty_cycle=70;
        }
        if (duty_cycle > 400) {
          duty_cycle=400;
        }
      }

      if (running) {
        if (duty_cycle > 998 ) {                                          // safety!!!
          duty_cycle = 998;
        }
        if (duty_cycle < 44) {
          duty_cycle = 44;
        }


        TIM1->CCR1 = duty_cycle;                      // set duty cycle to 50 out of 768 to start.
        TIM1->CCR2 = duty_cycle;
        TIM1->CCR3 = duty_cycle;
        //	TIM1->CCR4 = duty_cycle;
      }

    }
//


    signaltimeout++;
    if (signaltimeout > signal_timeout_threshold ) {
      input = 0;
      armed = 0;
      armedcount = 0;
      error = 1;

      //	  duty_cycle = 0;          //mid point
    }

    if (input <= 47) {
      //	sensorless = 0;
      started = 0;
      if ( !brake && !prop_brake_active) {
        allOff();
      }
      duty_cycle = 0;
      if(brake || tempbrake) {
        fullBrake();
        duty_cycle = 0;
        //	HAL_COMP_Stop_IT(&hcomp1);
      }




      if(prop_brake && prop_brake_active) {
        //	prop_brake_active = 1;
        duty_cycle = prop_brake_strength;
        proBrake();
      }

      TIM1->CCR1 = duty_cycle;                        // set duty cycle to 50 out of 768 to start.
      TIM1->CCR2 = duty_cycle;
      TIM1->CCR3 = duty_cycle;

      if (commutation_interval > 30000) {
        HAL_COMP_Stop_IT(&hcomp1);
        //	prop_brake_active = 0;
      }

    }

    if (bemf_counts < 100 || commutation_interval > 10000) {
      filter_delay = 15;
      filter_level = 10;
    }else{
      filter_level = 3;
      filter_delay = 3;
    }
    if(commutation_interval < 200 && duty_cycle > 500) {

      filter_delay = 1;
      filter_level = 0;
    }


    if (started == 1) {
      if (running == 0) {
        //		allOff();
        upthreshold = 2;
        threshold = 2;
        zctimeout =0;
        startMotor(); // safety on for input testing   ************************************************
      }
    }

    if (duty_cycle < 300) {
      zc_timeout_threshold = 4000;
    }else{
      zc_timeout_threshold = 2000;
    }

    zctimeout++;                                            // move to started if
    if (zctimeout > zc_timeout_threshold) {
      //		prop_brake_active = 0;
      bad_commutation = 0;
      sensorless = 0;
      HAL_COMP_Stop_IT(&hcomp1);
//			if(tempbrake){
//
//			}else{
//			allOff();
//			}
      running = 0;
      //		commutation_interval = 0;
      zctimeout = zc_timeout_threshold + 1;
      duty_cycle = 0;
    }




    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//			}
//		}
  }
  /* USER CODE END 3 */

}


void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  // Initializes the CPU, AHB and APB busses clocks
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI14 | RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  while (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK);

  // Initializes the CPU, AHB and APB busses clocks
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  while (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK);

  // Configure the Systick interrupt time
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  // Configure the Systick
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  // SysTick_IRQn interrupt configuration
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}


static void MX_ADC_Init(void) {
  ADC_ChannelConfTypeDef sConfig;

  // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = ENABLE;
  hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC4;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc.Init.DMAContinuousRequests = ENABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  while (HAL_ADC_Init(&hadc) != HAL_OK);

  // Configure for the selected ADC regular channel to be converted.
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  while (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK);


  // Configure for the selected ADC regular channel to be converted.
  sConfig.Channel = ADC_CHANNEL_6;
  while (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK);
}


static void MX_COMP1_Init(void) {
  hcomp1.Instance = COMP1;
  hcomp1.Init.InvertingInput = COMP_INVERTINGINPUT_DAC2;
  hcomp1.Init.NonInvertingInput = COMP_NONINVERTINGINPUT_IO1;
  hcomp1.Init.Output = COMP_OUTPUT_NONE;
  hcomp1.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
  hcomp1.Init.Hysteresis = COMP_HYSTERESIS_LOW;
  hcomp1.Init.Mode = COMP_MODE_HIGHSPEED;
  hcomp1.Init.WindowMode = COMP_WINDOWMODE_DISABLE;
  hcomp1.Init.TriggerMode = COMP_TRIGGERMODE_IT_RISING_FALLING;
  while (HAL_COMP_Init(&hcomp1) != HAL_OK);
}


static void MX_IWDG_Init(void) {
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_16;
  hiwdg.Init.Window = IWDG_WINDOW_DISABLE;
  hiwdg.Init.Reload = 2000;
  while (HAL_IWDG_Init(&hiwdg) != HAL_OK);
}


static void MX_TIM1_Init(void) {
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  while (HAL_TIM_Base_Init(&htim1) != HAL_OK);


  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  while (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK);


  while (HAL_TIM_PWM_Init(&htim1) != HAL_OK);


  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC4REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  while (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK);


  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  while (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK);
  while (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK);
  while (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK);
  while (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK);

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = dead_time;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  while (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK);

  HAL_TIM_MspPostInit(&htim1);
}

static void MX_TIM2_Init(void) {

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 100;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 5000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  while (HAL_TIM_Base_Init(&htim2) != HAL_OK);

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  while (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  while (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK);
}

static void MX_TIM3_Init(void) {

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 10;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  while (HAL_TIM_Base_Init(&htim3) != HAL_OK);

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  while (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  while (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK);
}

static void MX_TIM15_Init(void) {
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_IC_InitTypeDef sConfigIC;

  htim15.Instance = TIM15;
  htim15.Init.Prescaler = 0;
  htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim15.Init.Period = 0xffff;
  htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim15.Init.RepetitionCounter = 0;
  htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  while (HAL_TIM_Base_Init(&htim15) != HAL_OK);

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  while (HAL_TIM_ConfigClockSource(&htim15, &sClockSourceConfig) != HAL_OK);
  while (HAL_TIM_IC_Init(&htim15) != HAL_OK);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  while (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK);

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  while (HAL_TIM_IC_ConfigChannel(&htim15, &sConfigIC, TIM_CHANNEL_1) != HAL_OK);
}


static void MX_DMA_Init(void) {
  // DMA controller clock enable
  __HAL_RCC_DMA1_CLK_ENABLE();

  // DMA interrupt init
  // DMA1_Channel1_IRQn interrupt configuration
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  // DMA1_Channel4_5_IRQn interrupt configuration
  HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);
}
