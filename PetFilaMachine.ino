#if !defined(ESP8266)
  #error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd;

#define L_BTN D4
#define R_BTN D3
#define E_BTN D5

#define T_PIN A0

#define STEPS 400
#define STEP_PIN D7
#define DIR_PIN D8
#define EN_PIN D6

#define MOS_PIN D0

#include <EEPROM.h>

// EEPROM addresses
#define EEPROM_ADDR_TEMP_TARGET 0
#define EEPROM_ADDR_MOTOR_SPEED 4
#define EEPROM_ADDR_MOTOR_DIR 8
#define EEPROM_ADDR_PID_P 9
#define EEPROM_ADDR_PID_I 13
#define EEPROM_ADDR_PID_D 17
#define EEPROM_MAGIC 0xAB
#define EEPROM_MAGIC_ADDR 21

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

#include <GyverNTC.h>
GyverNTC therm(T_PIN, 100000, 3950, 25, 8890);

#define BTN_AMOUNT 3
#include <EncButton2.h>
EncButton2<EB_BTN> btn[BTN_AMOUNT];

#include <GyverStepper2.h>
GStepper2<STEPPER2WIRE> stepper(STEPS, STEP_PIN, DIR_PIN, EN_PIN);

#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
#define USING_TIM_DIV1                false
#define USING_TIM_DIV16               false
#define USING_TIM_DIV256              true
#include "ESP8266TimerInterrupt.h"
ESP8266Timer ITimer;

#define P 16.67
#define I 0.75
#define D 91.91
#define DT 10
#include "GyverPID.h"
GyverPID regulator(P, I, D, DT);

#include "PIDtuner.h"
PIDtuner tuner;

// ============================================================================
// CONSTANTS AND ENUMS
// ============================================================================

enum MenuState {
  STATE_VIEW,
  STATE_EDIT_TEMP,
  STATE_EDIT_SPEED,
  STATE_EDIT_MOTOR_STATE,
  STATE_EDIT_MOTOR_DIR,
  STATE_AUTOTUNE,
  STATE_EDIT_HEATER
};

enum CursorPosition {
  CURSOR_TARGET_TEMP = 1,
  CURSOR_MOTOR_SPEED = 2,
  CURSOR_MOTOR_STATE = 3,
  CURSOR_MOTOR_DIR = 4,
  CURSOR_AUTOTUNE = 5,
  CURSOR_HEATER_STATE = 6
};

const uint8_t PIN_MOSFET = D0;

const int TEMP_MIN = 0;
const int TEMP_MAX = 300;
const int SPEED_MIN = 0;
const int SPEED_MAX = 10000;

const unsigned long UPDATE_TEMP_INTERVAL = 1000;
const unsigned long CONFIRM_DELAY = 500;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

MenuState menuState = STATE_VIEW;
CursorPosition cursorPos = CURSOR_TARGET_TEMP;
bool autotuneActive = false;

int tempCurrent = 25;
int tempTarget = 25;

int motorSpeed = 100;
bool motorRunning = false;
bool motorDirection = false;

bool heaterEnabled = false;

unsigned long lastTempUpdate = 0;
unsigned long lastConfirmTime = 0;
bool pendingConfirmation = false;

// ============================================================================
// TIMER INTERRUPT HANDLER
// ============================================================================

void IRAM_ATTR TimerHandler()
{
  if (motorRunning) {
    stepper.tick();
  }
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

void displayInit() {
  int status = lcd.begin(20, 4);
  if (status) {
    Serial.println("-----Err init screen\n");
    lcd.fatalError(status);
  }
  
  lcd.createChar(0, motor_char_1);
  lcd.createChar(1, motor_char_2);
}

void displayWelcome() {
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("Welcome to");
  lcd.setCursor(0, 1);
  lcd.print("PetFilament Machine");
  lcd.setCursor(2, 2);
  lcd.print("Firmware ver 0.2");
  lcd.setCursor(0, 3);
  lcd.print("Powered By Mirivlad");
  delay(1000);
}

void displayMainScreen() {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Tc: ");
  lcd.setCursor(4, 0);
  lcd.print(tempCurrent);
  lcd.print(" Ts: ");
  lcd.setCursor(10, 0);
  lcd.print(tempTarget);
  
  lcd.setCursor(15, 0);
  lcd.write(0);
  lcd.write(1);
  
  lcd.setCursor(0, 1);
  lcd.print("Spd: ");
  lcd.setCursor(5, 1);
  lcd.print(motorSpeed);
  lcd.print(" Act: ");
  lcd.setCursor(12, 1);
  lcd.print(motorRunning ? "ON " : "OFF");
  
  lcd.setCursor(0, 2);
  lcd.print("Dir: ");
  lcd.setCursor(5, 2);
  lcd.print(motorDirection ? "BWD" : "FWD");
  lcd.print("  ");
  lcd.setCursor(10, 2);
  lcd.print("PID TUNE");
  
  lcd.setCursor(0, 3);
  lcd.print("Htr: ");
  lcd.setCursor(5, 3);
  lcd.print(heaterEnabled ? "ON " : "OFF");
  lcd.print("                    ");
}

void updateDisplay() {
  lcd.setCursor(4, 0);
  lcd.print("   ");
  lcd.setCursor(4, 0);
  lcd.print(tempCurrent);
  
  if (menuState == STATE_VIEW) {
    lcd.noBlink();
    lcd.cursor();
    
    switch (cursorPos) {
      case CURSOR_TARGET_TEMP:
        lcd.setCursor(10, 0);
        break;
      case CURSOR_MOTOR_SPEED:
        lcd.setCursor(5, 1);
        break;
      case CURSOR_MOTOR_STATE:
        lcd.setCursor(12, 1);
        break;
      case CURSOR_MOTOR_DIR:
        lcd.setCursor(5, 2);
        break;
      case CURSOR_AUTOTUNE:
        lcd.setCursor(10, 2);
        break;
      case CURSOR_HEATER_STATE:
        lcd.setCursor(5, 3);
        break;
    }
  } else {
    lcd.blink();
    lcd.cursor();
    
    switch (menuState) {
      case STATE_EDIT_TEMP:
        lcd.setCursor(10, 0);
        break;
      case STATE_EDIT_SPEED:
        lcd.setCursor(5, 1);
        break;
      case STATE_EDIT_MOTOR_STATE:
        lcd.setCursor(12, 1);
        break;
      case STATE_EDIT_MOTOR_DIR:
        lcd.setCursor(5, 2);
        break;
      case STATE_AUTOTUNE:
        lcd.setCursor(10, 2);
        break;
      case STATE_EDIT_HEATER:
        lcd.setCursor(5, 3);
        break;
    }
  }
}

void displayPIDValues(float p, float i, float d) {
  lcd.setCursor(0, 3);
  lcd.print("P=");
  lcd.print(p, 2);
  lcd.print(" I=");
  lcd.print(i, 2);
  lcd.print(" D=");
  lcd.print(d, 2);
}

void displayMessage(const char* msg) {
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(msg);
}

// ============================================================================
// PARAMETER CHANGE FUNCTIONS
// ============================================================================

void changeTargetTemp(int delta) {
  tempTarget += delta;
  tempTarget = constrain(tempTarget, TEMP_MIN, TEMP_MAX);
  
  lcd.setCursor(10, 0);
  lcd.print("   ");
  lcd.setCursor(10, 0);
  lcd.print(tempTarget);
  
  Serial.print("Target temp: ");
  Serial.println(tempTarget);
}

void changeMotorSpeed(int delta) {
  motorSpeed += delta;
  motorSpeed = constrain(motorSpeed, SPEED_MIN, SPEED_MAX);
  
  if (motorRunning) {
    if (!motorDirection) {
      stepper.setSpeed(motorSpeed);
    } else {
      stepper.setSpeed(-motorSpeed);
    }
  }
  
  lcd.setCursor(5, 1);
  lcd.print("     ");
  lcd.setCursor(5, 1);
  lcd.print(motorSpeed);
  
  Serial.print("Motor speed: ");
  Serial.println(motorSpeed);
}

void toggleMotorState() {
  motorRunning = !motorRunning;
  
  if (motorRunning) {
    stepper.enable();
    if (!motorDirection) {
      stepper.setSpeed(motorSpeed);
    } else {
      stepper.setSpeed(-motorSpeed);
    }
  } else {
    stepper.disable();
  }
  
  lcd.setCursor(12, 1);
  lcd.print(motorRunning ? "ON " : "OFF");
  
  Serial.print("Motor state: ");
  Serial.println(motorRunning ? "ON" : "OFF");
}

void toggleMotorDirection() {
  stepper.brake();
  delay(100);
  
  motorDirection = !motorDirection;
  
  if (motorRunning) {
    if (!motorDirection) {
      stepper.setSpeed(motorSpeed);
    } else {
      stepper.setSpeed(-motorSpeed);
    }
  }
  
  lcd.setCursor(5, 2);
  lcd.print(motorDirection ? "BWD" : "FWD");
  
  Serial.print("Motor direction: ");
  Serial.println(motorDirection ? "BWD" : "FWD");
}

void toggleHeaterState() {
  heaterEnabled = !heaterEnabled;
  
  if (!heaterEnabled) {
    analogWrite(PIN_MOSFET, 0);
  }
  
  lcd.setCursor(5, 3);
  lcd.print(heaterEnabled ? "ON " : "OFF");
  
  Serial.print("Heater state: ");
  Serial.println(heaterEnabled ? "ON" : "OFF");
}

void startAutotune() {
  autotuneActive = true;
  tuner.setParameters(NORMAL, tempTarget, 15, 5000, 0.08, 15000, 500);
  displayMessage("Autotuning...");
  Serial.println("Starting PID autotune");
}

void completeAutotune() {
  autotuneActive = false;
  
  float p = tuner.getPID_p();
  float i = tuner.getPID_i();
  float d = tuner.getPID_d();
  
  regulator.Kp = p;
  regulator.Ki = i;
  regulator.Kd = d;
  
  displayPIDValues(p, i, d);
  
  tuner.debugText();
  
  saveSettings();
  
  Serial.println("Autotune completed");
}

// ============================================================================
// MENU STATE MACHINE HANDLERS
// ============================================================================

void handleViewMode() {
  lcd.noBlink();
  lcd.cursor();
}

void handleEditMode() {
  lcd.blink();
  lcd.cursor();
}

// ============================================================================
// BUTTON HANDLERS
// ============================================================================

void handleEnterButton() {
  if (btn[0].held()) {
    if (menuState != STATE_VIEW) {
      pendingConfirmation = true;
      lastConfirmTime = millis();
      
      switch (menuState) {
        case STATE_EDIT_TEMP:
          displayMessage("Temp saved");
          saveSettings();
          break;
        case STATE_EDIT_SPEED:
          displayMessage("Speed saved");
          saveSettings();
          break;
        case STATE_EDIT_MOTOR_STATE:
          displayMessage("Motor saved");
          saveSettings();
          break;
        case STATE_EDIT_MOTOR_DIR:
          displayMessage("Dir saved");
          saveSettings();
          break;
        case STATE_EDIT_HEATER:
          displayMessage("Heater saved");
          break;
      }
      
      delay(CONFIRM_DELAY);
      menuState = STATE_VIEW;
      pendingConfirmation = false;
    }
  }
  
  if (btn[0].click()) {
    if (menuState == STATE_VIEW) {
      switch (cursorPos) {
        case CURSOR_TARGET_TEMP:
          menuState = STATE_EDIT_TEMP;
          break;
        case CURSOR_MOTOR_SPEED:
          menuState = STATE_EDIT_SPEED;
          break;
        case CURSOR_MOTOR_STATE:
          menuState = STATE_EDIT_MOTOR_STATE;
          break;
        case CURSOR_MOTOR_DIR:
          menuState = STATE_EDIT_MOTOR_DIR;
          break;
        case CURSOR_AUTOTUNE:
          menuState = STATE_AUTOTUNE;
          startAutotune();
          break;
        case CURSOR_HEATER_STATE:
          menuState = STATE_EDIT_HEATER;
          break;
      }
    } else {
      menuState = STATE_VIEW;
      displayMessage("");
    }
    
    if (menuState == STATE_VIEW) {
      cursorPos++;
      if (cursorPos > CURSOR_HEATER_STATE) {
        cursorPos = CURSOR_TARGET_TEMP;
      }
    }
  }
}

void handleRightButton() {
  if (menuState == STATE_VIEW) {
    return;
  }
  
  if (btn[1].step(2)) {
    switch (menuState) {
      case STATE_EDIT_TEMP:
        changeTargetTemp(5);
        break;
      case STATE_EDIT_SPEED:
        changeMotorSpeed(10);
        break;
    }
  } else if (btn[1].step(4)) {
    switch (menuState) {
      case STATE_EDIT_TEMP:
        changeTargetTemp(3);
        break;
      case STATE_EDIT_SPEED:
        changeMotorSpeed(5);
        break;
    }
  } else if (btn[1].click()) {
    switch (menuState) {
      case STATE_EDIT_TEMP:
        changeTargetTemp(1);
        break;
      case STATE_EDIT_SPEED:
        changeMotorSpeed(1);
        break;
      case STATE_EDIT_MOTOR_STATE:
        toggleMotorState();
        break;
      case STATE_EDIT_MOTOR_DIR:
        toggleMotorDirection();
        break;
      case STATE_EDIT_HEATER:
        toggleHeaterState();
        break;
    }
  }
}

void handleLeftButton() {
  if (menuState == STATE_VIEW) {
    return;
  }
  
  if (btn[2].step(2)) {
    switch (menuState) {
      case STATE_EDIT_TEMP:
        changeTargetTemp(-5);
        break;
      case STATE_EDIT_SPEED:
        changeMotorSpeed(-10);
        break;
    }
  } else if (btn[2].step(4)) {
    switch (menuState) {
      case STATE_EDIT_TEMP:
        changeTargetTemp(-3);
        break;
      case STATE_EDIT_SPEED:
        changeMotorSpeed(-5);
        break;
    }
  } else if (btn[2].click()) {
    switch (menuState) {
      case STATE_EDIT_TEMP:
        changeTargetTemp(-1);
        break;
      case STATE_EDIT_SPEED:
        changeMotorSpeed(-1);
        break;
      case STATE_EDIT_MOTOR_STATE:
        toggleMotorState();
        break;
      case STATE_EDIT_MOTOR_DIR:
        toggleMotorDirection();
        break;
      case STATE_EDIT_HEATER:
        toggleHeaterState();
        break;
    }
  }
}

// ============================================================================
// MAIN CONTROL FUNCTIONS
// ============================================================================

void updateTemperature() {
  tempCurrent = therm.getTempAverage();
}

void controlHeater() {
  if (autotuneActive) {
    tuner.setInput(tempCurrent);
    tuner.compute();
    analogWrite(PIN_MOSFET, tuner.getOutput());
    
    if (tuner.getAccuracy() > 95) {
      completeAutotune();
    }
  } else if (heaterEnabled) {
    regulator.setpoint = tempTarget;
    regulator.input = tempCurrent;
    int output = regulator.getResultTimer();
    output = constrain(output, 0, 255);
    analogWrite(PIN_MOSFET, output);
  } else {
    analogWrite(PIN_MOSFET, 0);
  }
}

void controlMotor() {
  if (motorRunning) {
    if (!motorDirection) {
      stepper.setSpeed(motorSpeed);
    } else {
      stepper.setSpeed(-motorSpeed);
    }
  } else {
    stepper.disable();
  }
}

// ============================================================================
// EEPROM FUNCTIONS
// ============================================================================

void saveSettings() {
  EEPROM.begin(32);
  
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  
  EEPROM.put(EEPROM_ADDR_TEMP_TARGET, tempTarget);
  EEPROM.put(EEPROM_ADDR_MOTOR_SPEED, motorSpeed);
  EEPROM.put(EEPROM_ADDR_MOTOR_DIR, motorDirection);
  
  float p = regulator.Kp;
  float i = regulator.Ki;
  float d = regulator.Kd;
  EEPROM.put(EEPROM_ADDR_PID_P, p);
  EEPROM.put(EEPROM_ADDR_PID_I, i);
  EEPROM.put(EEPROM_ADDR_PID_D, d);
  
  EEPROM.end();
  
  Serial.println("Settings saved to EEPROM");
}

void loadSettings() {
  EEPROM.begin(32);
  
  byte magic = EEPROM.read(EEPROM_MAGIC_ADDR);
  
  if (magic == EEPROM_MAGIC) {
    int loadedTemp;
    int loadedSpeed;
    bool loadedDir;
    float loadedP, loadedI, loadedD;
    
    EEPROM.get(EEPROM_ADDR_TEMP_TARGET, loadedTemp);
    EEPROM.get(EEPROM_ADDR_MOTOR_SPEED, loadedSpeed);
    EEPROM.get(EEPROM_ADDR_MOTOR_DIR, loadedDir);
    EEPROM.get(EEPROM_ADDR_PID_P, loadedP);
    EEPROM.get(EEPROM_ADDR_PID_I, loadedI);
    EEPROM.get(EEPROM_ADDR_PID_D, loadedD);
    
    if (loadedTemp >= TEMP_MIN && loadedTemp <= TEMP_MAX) {
      tempTarget = loadedTemp;
    }
    
    if (loadedSpeed >= SPEED_MIN && loadedSpeed <= SPEED_MAX) {
      motorSpeed = loadedSpeed;
    }
    
    motorDirection = loadedDir;
    
    if (loadedP > 0 && loadedI > 0 && loadedD > 0) {
      regulator.Kp = loadedP;
      regulator.Ki = loadedI;
      regulator.Kd = loadedD;
    }
    
    Serial.println("Settings loaded from EEPROM");
  } else {
    Serial.println("No valid settings in EEPROM, using defaults");
  }
  
  EEPROM.end();
}

// ============================================================================
// SETUP AND LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  
  displayInit();
  displayWelcome();
  displayMainScreen();
  
  btn[0].setPins(INPUT_PULLUP, E_BTN);
  btn[1].setPins(INPUT_PULLUP, R_BTN);
  btn[2].setPins(INPUT_PULLUP, L_BTN);
  
  if (ITimer.attachInterruptInterval(100, TimerHandler)) {
    Serial.print(F("Starting ITimer OK, millis() = "));
    Serial.println(millis());
  } else {
    Serial.println(F("Can't set ITimer correctly"));
  }
  
  regulator.setDirection(NORMAL);
  regulator.setLimits(0, 255);
  regulator.setpoint = tempTarget;
  
  loadSettings();
  
  motorRunning = false;
  heaterEnabled = false;
  
  Serial.println("+++++ Setup complete");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastTempUpdate >= UPDATE_TEMP_INTERVAL) {
    lastTempUpdate = currentMillis;
    updateTemperature();
    updateDisplay();
  }
  
  controlHeater();
  controlMotor();
  
  for (int i = 0; i < BTN_AMOUNT; i++) {
    btn[i].tick();
  }
  
  handleEnterButton();
  handleRightButton();
  handleLeftButton();
  
  if (menuState == STATE_VIEW) {
    handleViewMode();
  } else {
    handleEditMode();
  }
  
  updateDisplay();
}
