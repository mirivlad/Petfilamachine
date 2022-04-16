#if !defined(ESP8266)
  #error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif
#include <Wire.h>
//// Not work with Gyver Lib - EncButton
//const int sda=D1, scl=D2;
// #include <SoftwareWire.h>	//make sure to not use beyond version 1.5.0
// // Check for "new" SoftwareWire that breaks things
// #if defined(TwoWire_h)
//  #error incompatible version of SoftwareWire library (use version 1.5.0)
// #endif
// SoftwareWire Wire(D1,D2); // Create Wire object using desired Arduino pins
//////
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd; // declare lcd object and let it auto-configure everything.


#define L_BTN D4    // left button
#define R_BTN D3    // right button
#define E_BTN D5    // enter button

#define T_PIN A0    // thermistor pin

#define STEPS 400    // thermistor pin
#define STEP_PIN D7    // thermistor pin
#define DIR_PIN D8    // thermistor pin
#define EN_PIN D6    // thermistor pin

#define MOS_PIN D0
// Make custom characters:
byte motor_char_1[] = {
  B00111,
  B01000,
  B10110,
  B10101,
  B10100,
  B10100,
  B01000,
  B00111
};
byte motor_char_2[] = {
  B11100,
  B00010,
  B01101,
  B10101,
  B00101,
  B00101,
  B00010,
  B11100
};

//enable NTC thermistor
#include <GyverNTC.h>
// термистор на пине А0
// сопротивление резистора 10к
// тепловой коэффициент 3950
GyverNTC therm(T_PIN, 100000, 3950, 25, 8890);

//enable button lib and define buttons array
#define BTN_AMOUNT 5
#define EB_HOLD 1000
#include <EncButton2.h>
EncButton2<EB_BTN> btn[BTN_AMOUNT];

//enable stepper
//#define GS_NO_ACCEL                         
// отключить модуль движения с ускорением (уменьшить вес кода)
#include <GyverStepper2.h>
GStepper2<STEPPER2WIRE> stepper(STEPS, STEP_PIN, DIR_PIN, EN_PIN);

// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
// Select a Timer Clock
#define USING_TIM_DIV1                false           // for shortest and most accurate timer
#define USING_TIM_DIV16               false           // for medium time and medium accurate timer
#define USING_TIM_DIV256              true            // for longest timer but least accurate. Default
#include "ESP8266TimerInterrupt.h"
volatile uint32_t lastMillis = 0;
#define TIMER_INTERVAL_MS   1000
// Init ESP8266 timer 1
ESP8266Timer ITimer;
#define P 16.67
#define I 0.75
#define D 91.91
#define DT 10
//PID regulator
#include "GyverPID.h"
GyverPID regulator(P, I, D, DT);  // коэф. П, коэф. И, коэф. Д, период дискретизации dt (мс)
//Autotune PID
#include "PIDtuner.h"
PIDtuner tuner;

//define vars
int max_speed = STEPS;
int cursor=1; //позиция курсора
int t_current=25;
// int t_current_temp=230;
int t_set=25;
// int t_set_temp=230;
int motor_speed=100;
// int motor_speed_temp=100;
int motor_state = 0;//0 - off, 1 - on
String motor_state_text = "OFF";
// int motor_state_temp = 0;
// String motor_state_temp_text = "OFF";
int motor_dir = 0;//0 - forward, 1 - backward (FWD, BWD)
String motor_dir_text = "FWD";
// int motor_dir_temp = 0;
// String motor_dir_temp_text = "FWD";

int save=100; //режим работы меню. 100- режим выбора. 1,2,3 - выбранное значение

long previousMillis = 0;        // храним время последнего переключения светодиода
long interval = 600;           // интервал между включение/выключением светодиода (1 секунда)
int autotune=0;
//===========Stepping in timer interrupts==================================
void IRAM_ATTR TimerHandler()
{
  if (motor_state==1)
  {
    stepper.tick();
  }
}


//=====================================================================
void setup(){
  Serial.begin(115200);

  // Start interrupts timer. Interval in microsecs
  if (ITimer.attachInterruptInterval(100, TimerHandler))
  {
    lastMillis = millis();
    Serial.print(F("Starting  ITimer OK, millis() = ")); Serial.println(lastMillis);
  }
  else{
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval"));
  }
  //initial screen
  init_screen();
  //init buttons
  btn[0].setPins(INPUT_PULLUP, L_BTN);
  btn[1].setPins(INPUT_PULLUP, R_BTN);
  btn[2].setPins(INPUT_PULLUP, E_BTN);


  regulator.setDirection(NORMAL); // направление регулирования (NORMAL/REVERSE). ПО УМОЛЧАНИЮ СТОИТ NORMAL
  regulator.setLimits(0, 255);    // пределы (ставим для 8 битного ШИМ). ПО УМОЛЧАНИЮ СТОЯТ 0 И 255
  regulator.setpoint = t_set;        // сообщаем регулятору температуру, которую он должен поддерживать
}

void init_screen(){

  int istatus;
  istatus = lcd.begin(20,4);
  if(istatus)
  {
	  Serial.println("-----Err init screen\n");
	  lcd.fatalError(istatus); // blinks error code on built in LED
  }
  //lcd.init();                      // initialize the lcd 
  // Print a message to the LCD.
  //lcd.backlight();
  lcd.createChar(0, motor_char_1);
  lcd.createChar(1, motor_char_2);

  lcd.setCursor(5,0);
  lcd.print("Welcome to");
  lcd.setCursor(0,1);
  lcd.print("PetFilament Machine");
  lcd.setCursor(2,2);
  lcd.print("Firmware ver 0.1");
  lcd.setCursor(0,3);
  lcd.print("Powered By Mirivlad");
  Serial.println("+++++Welcome screen ok");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Tc: ");//4,0 - set Temperature current
  lcd.setCursor(4,0);
  lcd.print(t_current);

  lcd.setCursor(0,1);
  lcd.print("Ts: ");//4,1 - set Temperature setting
  lcd.setCursor(4,1);
  lcd.print(t_set);
  
  lcd.setCursor(0,2);
  lcd.print(" PID TUNE");//0,2 - set Temperature setting
  // lcd.setCursor(4,1);
  // lcd.print(t_set);

  lcd.setCursor(9,0);
  lcd.write(0);
  lcd.write(1);
  lcd.setCursor(11,0);
  lcd.print("spd: 100");//16,0 - Motor speed
  lcd.setCursor(16,0);
  lcd.print(motor_speed);

  if(motor_state==0){
    motor_state_text="OFF";
  }else{
    motor_state_text=" ON";
  }
  lcd.setCursor(9,1);
  lcd.write(0);
  lcd.write(1);
  lcd.setCursor(11,1);
  lcd.print("act: OFF");//16,1 - Motor speed
  lcd.setCursor(16,1);
  lcd.print(motor_state_text);

  if(motor_dir==0){
    motor_dir_text="FWD";
  }else{
    motor_dir_text=" BWD";
  }
  lcd.setCursor(9,2);
  lcd.write(0);
  lcd.write(1);
  lcd.setCursor(11,2);
  lcd.print("dir: FWD");//16,2 - Motor speed
  lcd.setCursor(16,2);
  lcd.print(motor_dir_text);
}
void change_params(int save, int plus, int step_val){
    //change current temperature
    //save - value change params. 
    // 0 - current temperature
    // 1 - needed temperature
    // 2 - motor speed
    // 3 - motor start|stop
    // 4 - motor direction
    if (save==0){
    
      if (t_current>=300 || t_current<=0){
        //stop heating
      }
      lcd.noBlink();
      lcd.noCursor();
      lcd.setCursor(4,0);
      lcd.print("   ");
      lcd.setCursor(4,0);
      lcd.print(t_current);
      Serial.println(t_current);
    }
    //change needed temperature
    if (save==1){
      
      if (plus==1){
        t_set+=step_val;
      }
      if (plus==0){
        t_set-=step_val;
      }
      if (t_set>=300){
        t_set=300;
      }
      if (t_set<=0){
        t_set=0;
      }

      lcd.setCursor(4,1);
      lcd.print("   ");
      lcd.setCursor(4,1);
      lcd.print(t_set);
      Serial.println("+++++Set needing temp ok");
    }
    //change motor speed
    if (save==2){
      if (plus==1){
        motor_speed+=step_val;
      }
      if (plus==0){
        motor_speed-=step_val;
      } 
      if (motor_speed>=10000){
        motor_speed=10000;
      }
      if (motor_speed<=0){
        motor_speed=0;
      }
      if (motor_state==1){
        if (motor_dir==0){
          stepper.setSpeed(motor_speed);
        }else{
          stepper.setSpeed(-motor_speed);
        }
      }
      lcd.setCursor(16,0);
      lcd.print("   ");
      lcd.setCursor(16,0);
      lcd.print(motor_speed);
      Serial.println("+++++Set motor speed ok");
    }
    //change motor state
    if (save==3){

      if (motor_state>=1){
        motor_state=0;
        motor_state_text="OFF";
        // motor_state=0;
        // motor_state_text="OFF";
        stepper.disable();        
      }else{
        // motor_state_temp=1;
        // motor_state_temp_text="ON";
        motor_state=1;
        motor_state_text="ON";
        stepper.enable();
      }

      lcd.setCursor(16,1);
      lcd.print("   ");
      lcd.setCursor(16,1);
      lcd.print(motor_state_text);
      Serial.println("+++++Set motor state ok");
    }
    if (save==4){

      if (motor_dir>=1){
        // motor_dir_temp=0;
        // motor_dir_temp_text="FWD";
        motor_dir=0;
        motor_dir_text="FWD";
      }else{
        // motor_dir_temp=1;
        // motor_dir_temp_text="BWD";
        motor_dir=1;
        motor_dir_text="BWD";
      }

      lcd.setCursor(16,2);
      lcd.print("   ");
      lcd.setCursor(16,2);
      lcd.print(motor_dir_text);
      Serial.println("+++++Set motor direction ok");
    }
}
void loop()
{ 
  if (autotune==1){
    tuner.setParameters(NORMAL, t_set, 15, 5000, 0.08, 15000, 500);
    tuner.setInput(therm.getTempAverage());
    tuner.compute();
    analogWrite(MOS_PIN, tuner.getOutput());
    if (tuner.getAccuracy() > 95){
      autotune=0;
      // выводит в порт текстовые отладочные данные, включая коэффициенты
      tuner.debugText();
      lcd.setCursor(0,3);
      lcd.print("                    ");//0,2 - set Temperature setting
      lcd.setCursor(0,3);
      lcd.print(tuner.getPID_p());//0,2 - set Temperature setting
      lcd.setCursor(6,3);
      lcd.print(tuner.getPID_i());//0,2 - set Temperature setting
      lcd.setCursor(12,3);
      lcd.print(tuner.getPID_d());//0,2 - set Temperature setting

      regulator.Kp = tuner.getPID_p();
      regulator.Ki = tuner.getPID_i();
      regulator.Kd = tuner.getPID_d();
    }
  }  
  //t_current=therm.getTemp();
  t_current=therm.getTempAverage();
  unsigned long currentMillis = millis();
  
  //проверяем не прошел ли нужный интервал, если прошел то
  if(currentMillis - previousMillis > interval) {
    // сохраняем время последнего переключения
    previousMillis = currentMillis; 
    change_params(0,100,0);  
  }
  if (autotune!=1){
    regulator.setpoint = t_set;
    regulator.input = t_current;   // сообщаем регулятору текущую температуру
    // getResultTimer возвращает значение для управляющего устройства
    // (после вызова можно получать это значение как regulator.output)
    // обновление происходит по встроенному таймеру на millis()
    regulator.getResult();
    analogWrite(D0, regulator.output);  // отправляем на мосфет
  }
  if (motor_state == 1){
    if (motor_dir == 0){
      stepper.setSpeed(motor_speed);
    }else{
      stepper.setSpeed(-motor_speed);
    }
  }else{
    stepper.disable();
  }
  


  if (save==100){
    lcd.noBlink();
    lcd.cursor();
  }
  for (int i = 0; i < BTN_AMOUNT; i++) btn[i].tick();

  if (cursor==1){
      lcd.setCursor(3, 1);
  }
  if (cursor==2){
      lcd.setCursor(15, 0);
  }
  if (cursor==3){
      lcd.setCursor(15, 1);
  }
  if (cursor==4){
      lcd.setCursor(15, 2);
  }

  if (cursor==5){
      lcd.setCursor(0, 2);
  }
    //listen button held
  if (btn[0].held()) {
    Serial.println("hold enter");
    //enter change mode
    if (save==100){
      Serial.println("hold enter and save==100");
      save=cursor;
      lcd.blink(); 
      lcd.cursor();
    }else{
      //enter save mode

      if(save==1){
        //Serial.println("hold enter and save==1");
        //t_set=t_set_temp;
        lcd.setCursor(4,1);
        lcd.print("   ");
        lcd.setCursor(4,1);
        lcd.print(t_set);
        lcd.cursor();
        lcd.blink();
        delay(3000);
      }
      if(save==2){
        //Serial.println("hold enter and save==2");
        //motor_speed=motor_speed_temp;
        if (motor_state==1){
          if (motor_dir==0){
            stepper.setSpeed(motor_speed);
          }else{
            stepper.setSpeed(-motor_speed);
          }
        }
        
        lcd.setCursor(16,0);
        lcd.print("   ");
        lcd.setCursor(16,0);
        lcd.print(motor_speed);
        lcd.cursor();
        lcd.blink();
        delay(3000);
      }
      if(save==3){
        //Serial.println("hold enter and save==3");
        //motor_state=motor_state_temp;
        //motor_state_text=motor_state_temp_text;
        if (motor_state==0){
          stepper.stop();
          stepper.disable();
        }else{
          stepper.enable();
        }
        lcd.setCursor(16,1);
        lcd.print("   ");
        lcd.setCursor(16,1);
        lcd.print(motor_state_text);
        lcd.cursor();
        lcd.blink();
        delay(3000);
      }
      if(save==4){
        //Serial.println("hold enter and save==4");
        //motor_dir=motor_dir_temp;
        //motor_dir_text=motor_dir_temp_text;
        //тормозим перед сменой направления
        stepper.brake();
        lcd.setCursor(16,2);
        lcd.print("   ");
        lcd.setCursor(16,2);
        lcd.print(motor_dir_text);
        lcd.cursor();
        lcd.blink();
        delay(3000);
      }

      if (save==5){
        autotune=1;
        lcd.setCursor(0,2);
        lcd.cursor();
        lcd.blink();
        delay(3000);
      }
      lcd.noBlink(); 
      lcd.cursor();
      //lcd.blink();
      save=100;
    } 
  }
  //listen button click
  if (btn[0].click()) {
    //Serial.println("press enter");
    if(save!=100){
      save=100;
      // t_current_temp=t_current;
      // t_set_temp=t_set;
      // motor_speed_temp=motor_speed;
      // motor_state_temp=motor_state;
      // motor_state_temp_text=motor_state_text;
      // motor_dir_temp=motor_dir;
      // motor_dir_temp_text=motor_dir_text;
      lcd.setCursor(4,0);
      lcd.print("   ");
      lcd.setCursor(4,0);
      lcd.print(t_current);
      lcd.setCursor(4,1);
      lcd.print("   ");
      lcd.setCursor(4,1);
      lcd.print(t_set);
      lcd.setCursor(16,0);
      lcd.print("   ");
      lcd.setCursor(16,0);
      lcd.print(motor_speed);
      lcd.setCursor(16,1);
      lcd.print("   ");
      lcd.setCursor(16,1);
      lcd.print(motor_state_text);
      lcd.setCursor(16,2);
      lcd.print("   ");
      lcd.setCursor(16,2);
      lcd.print(motor_dir_text);
    }
    cursor++;
    if (cursor>5){
      cursor=1;
    }
  }  
  //TODO: how to know how much click done before held?
  //int clicks1 = btn[1].hasClicks();
  //int clicks2 = btn[2].hasClicks();
  if (btn[1].step(2) && save!=100) {
    //Serial.println("press right and save!=100 and HOLD");
    //Serial.println(clicks1);
    change_params(save,1,3);  
  }
  if (btn[2].step(2) && save!=100) {
    //Serial.println("press right and save!=100 and HOLD");
    change_params(save,0,3);  
  }
  if (btn[1].step(4) && save!=100) {
    //Serial.println("press right and save!=100 and HOLD");
    //Serial.println(clicks1);
    change_params(save,1,4);  
  }
  if (btn[2].step(4) && save!=100) {
    //Serial.println("press right and save!=100 and HOLD");
    change_params(save,0,4);  
  }
  if (btn[1].click() && save!=100) {
    //Serial.println("press right and save!=100");
    change_params(save,1,1);  
  } 
  if (btn[2].click() && save!=100) {
    //Serial.println("press left and save!=100");
    change_params(save,0,1);  
  } 

}
