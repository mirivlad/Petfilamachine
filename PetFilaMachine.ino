#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// объявляем массив кнопок
#define BTN_AMOUNT 5
#include <EncButton2.h>
EncButton2<EB_BTN> btn[BTN_AMOUNT];



int cursor=0;
int t_current=230;
int t_set=230;
int motor_speed=100;


void setup()
{


  lcd.init();                      // initialize the lcd 
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(5,0);
  lcd.print("Welcome to");
  lcd.setCursor(0,1);
  lcd.print("PetFilament Machine");
  lcd.setCursor(2,2);
  lcd.print("Firmware ver 0.1");
  lcd.setCursor(0,3);
  lcd.print("Powered By Mirivlad");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Tc: ");//4,0 - set Temperature current
  lcd.setCursor(4,0);
  lcd.print(t_current);
  lcd.setCursor(13,0);
  lcd.print("Ts: ");//17,0 - set Temperature setting
  lcd.setCursor(17,0);
  lcd.print(t_set);
  lcd.setCursor(0,2);
  lcd.print("Mot.Speed: 100");//11,2 - Motor speed
  lcd.setCursor(11,2);
  lcd.print(motor_speed);

  btn[0].setPins(INPUT_PULLUP, D7);
  btn[1].setPins(INPUT_PULLUP, D6);
  btn[2].setPins(INPUT_PULLUP, D5);
  Serial.begin(115200);
}

void change_params(int cursor, int plus){
    if (cursor==0){
      if (plus==1){
        t_current++;
      }else{
        t_current--;
      }
      if (t_current>=300 || t_current<=0){
        //stop heating
      }
      lcd.setCursor(4,0);
      lcd.print("   ");
      lcd.setCursor(4,0);
      lcd.print(t_current);
    }
    if (cursor==1){
      if (plus==1){
        t_set++;
      }else{
        t_set--;
      }
      if (t_set>=300){
        t_set=300;
      }
      if (t_set<=0){
        t_set=0;
      }

      lcd.setCursor(17,0);
      lcd.print("   ");
      lcd.setCursor(17,0);
      lcd.print(t_set);
    }
    if (cursor==2){
      if (plus==1){
        motor_speed++;
      }else{
        motor_speed--;
      }
      if (motor_speed>=255){
        motor_speed=255;
      }
      if (motor_speed<=-255){
        motor_speed=-255;
      }

      lcd.setCursor(11,2);
      lcd.print("   ");
      lcd.setCursor(11,2);
      lcd.print(motor_speed);
    }
}
void loop()
{
  for (int i = 0; i < BTN_AMOUNT; i++) btn[i].tick();
  lcd.blink();
  if (cursor==0){
      lcd.setCursor(3, 0);
   }
  if (cursor==1){
    //lcd.noBlink();
      lcd.setCursor(16, 0);
  }
  if (cursor==2){
      lcd.setCursor(10, 2);
  }
  if (btn[0].click()) {
    Serial.println("press enter, cursor=");
    
      cursor++;
      if (cursor>2){
        cursor=0;
      }
      Serial.println(cursor);
  }  
  if (btn[1].click()) {
    Serial.println("press right, cursor=");
    Serial.println(cursor);
    change_params(cursor,1);  
  } 
  if (btn[2].click()) {
    Serial.println("press left, cursor=");
    Serial.println(cursor);
    change_params(cursor,0);  
  } 
  
}
