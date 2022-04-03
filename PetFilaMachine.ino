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

#define L_BTN D0    // left button
#define R_BTN D3    // right button
#define E_BTN D4    // enter button
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
GyverNTC therm(0, 100000, 3950);

//enable button lib and define buttons array
#define BTN_AMOUNT 5
#define EB_HOLD 1000
#include <EncButton2.h>
EncButton2<EB_BTN> btn[BTN_AMOUNT];

//define vars
int cursor=1; //позиция курсора
int t_current=230;
int t_current_temp=230;
int t_set=230;
int t_set_temp=230;
int motor_speed=100;
int motor_speed_temp=100;
int motor_state = 0;//0 - off, 1 - on
String motor_state_text = "OFF";
int motor_state_temp = 0;
String motor_state_temp_text = "OFF";
int motor_dir = 0;//0 - forward, 1 - backward (FWD, BWD)
String motor_dir_text = "FWD";
int motor_dir_temp = 0;
String motor_dir_temp_text = "FWD";

int save=100; //режим работы меню. 100- режим выбора. 1,2,3 - выбранное значение
float filT = 0; //фильтрованное значение датчика
long previousMillis = 0;        // храним время последнего переключения светодиода
long interval = 1000;           // интервал между включение/выключением светодиода (1 секунда)

void setup()
{
  Serial.begin(115200);
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

  btn[0].setPins(INPUT_PULLUP, L_BTN);
  btn[1].setPins(INPUT_PULLUP, R_BTN);
  btn[2].setPins(INPUT_PULLUP, E_BTN);
  
  Serial.println("+++++Menu screen ok");
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
      if (plus==1){
        t_current_temp++;
      }
      if (plus==0){
        t_current_temp--;
      }      
      if (t_current>=300 || t_current<=0){
        //stop heating
      }
      lcd.noBlink();
      lcd.noCursor();
      lcd.setCursor(4,0);
      lcd.print("   ");
      lcd.setCursor(4,0);
      lcd.print(t_current_temp);
      Serial.println("+++++Set current temp ok");
    }
    //change needed temperature
    if (save==1){
      
      if (plus==1){
        t_set_temp+=step_val;
      }
      if (plus==0){
        t_set_temp-=step_val;
      }
      if (t_set_temp>=300){
        t_set_temp=300;
      }
      if (t_set_temp<=0){
        t_set_temp=0;
      }

      lcd.setCursor(4,1);
      lcd.print("   ");
      lcd.setCursor(4,1);
      lcd.print(t_set_temp);
      Serial.println("+++++Set needing temp ok");
    }
    //change motor speed
    if (save==2){
      if (plus==1){
        motor_speed_temp+=step_val;
      }
      if (plus==0){
        motor_speed_temp-=step_val;
      } 
      if (motor_speed_temp>=100){
        motor_speed_temp=100;
      }
      if (motor_speed_temp<=0){
        motor_speed_temp=0;
      }

      lcd.setCursor(16,0);
      lcd.print("   ");
      lcd.setCursor(16,0);
      lcd.print(motor_speed_temp);
      Serial.println("+++++Set motor speed ok");
    }
    //change motor state
    if (save==3){

      if (motor_state_temp>=1){
        motor_state_temp=0;
        motor_state_temp_text="OFF";
      }
      if (motor_state_temp<=0){
        motor_state_temp=1;
        motor_state_temp_text="ON";
      }

      lcd.setCursor(16,1);
      lcd.print("   ");
      lcd.setCursor(16,1);
      lcd.print(motor_state_temp_text);
      Serial.println("+++++Set motor state ok");
    }
    if (save==4){

      if (motor_dir_temp>=1){
        motor_dir_temp=0;
        motor_dir_temp_text="FWD";
      }
      if (motor_dir_temp<=0){
        motor_dir_temp=1;
        motor_dir_temp_text="BWD";
      }

      lcd.setCursor(16,2);
      lcd.print("   ");
      lcd.setCursor(16,2);
      lcd.print(motor_dir_temp_text);
      Serial.println("+++++Set motor direction ok");
    }
}

void loop()
{ 
  //get temperature with filtration
  //Serial.println("=====Try get temp");
  //filT += (therm.getTemp() - filT) * 0.1;
  t_current_temp=filT;
  //Serial.println("=====Get temp");
  //therm.getTempAverage();
  unsigned long currentMillis = millis();
  
  //проверяем не прошел ли нужный интервал, если прошел то
  if(currentMillis - previousMillis > interval) {
    // сохраняем время последнего переключения
    previousMillis = currentMillis; 

    change_params(0,100,0);  
    Serial.println("=====Send current temp");
    
  }

  
  
  if (save==100){
    lcd.noBlink();
    lcd.cursor();
  }
  for (int i = 0; i < BTN_AMOUNT; i++) btn[i].tick();
  //moving cursor
  //if (cursor==0){
  //    lcd.setCursor(3, 0);
  //}
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
      // if(save==0){
      //   Serial.println("hold enter and save==0");
      //   t_current=t_current_temp;
      //   lcd.setCursor(4,0);
      //   lcd.print("   ");
      //   lcd.setCursor(4,0);
      //   lcd.print(t_current);
      //   lcd.cursor();
      //   lcd.blink();
      //   delay(3000);
      // }
      if(save==1){
        //Serial.println("hold enter and save==1");
        t_set=t_set_temp;
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
        motor_speed=motor_speed_temp;
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
        motor_state=motor_state_temp;
        motor_state_text=motor_state_temp_text;
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
        motor_dir=motor_dir_temp;
        motor_dir_text=motor_dir_temp_text;
        lcd.setCursor(16,2);
        lcd.print("   ");
        lcd.setCursor(16,2);
        lcd.print(motor_dir_temp_text);
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
      t_current_temp=t_current;
      t_set_temp=t_set;
      motor_speed_temp=motor_speed;
      motor_state_temp=motor_state;
      motor_state_temp_text=motor_state_text;
      motor_dir_temp=motor_dir;
      motor_dir_temp_text=motor_dir_text;
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
    if (cursor>4){
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
