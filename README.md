# Petfilamachine
Простая прошивка для станка переработки бутылок из PET в пруток для 3d-принтера.
A simple firmware for a machine for processing plastic bottles into filament for a 3D printer running Arduino Nano

#### Про проект

Я начал делать станок для протяжки ленты из ПЭТ бутылок в пруток для 3д принтера. Я нашел готовый станок ПетПулл2 и он показался мне замечательным. Однако вскрылся один минус. Автор станка по какой-то причине выкладывает для него только скомпилированную прошивку в хекс формате.
Я решил что возможно для меня будет неплохой практикой попробовать написать свою прошивку. 

Скетч компилируется в Arduino IDE.

#### About this project

I started making a machine for pulling a ribbon from PET bottles into a rod for a 3D printer. I found a ready-made PetPull2 machine and it seemed wonderful to me. However, there was one downside. For some reason, the author of the machine uploads for him only the compiled firmware in hex format.
I decided that maybe it would be a good practice for me to try to write my own firmware. 

Compiling sketch in Arduino IDE

#### TaskList

- [x] Управление кнопками
- [ ] Сохранение настроек и загрузка их при включении
- [X] Подключение термистора
- [X] Считывание данных с термистора
- [ ] Подключение мотора
- [ ] Подключение нагревателя
- [ ] Настройка PID

#### Some info about libraries and functions

##### EncButton by AlexGyver

```cpp
bool busy();        // вернёт true, если всё ещё нужно вызывать tick для опроса таймаутов
bool state();       // текущее состояние кнопки (true нажата, false не нажата)
bool press();       // кнопка была нажата [однократное срабатывание]
bool release();     // кнопка была отпущена [однократное срабатывание]
bool click();       // клик (нажата и отпущена) [однократное срабатывание]
bool held();        // кнопка была удержана [однократное срабатывание]
bool hold();        // кнопка удерживается [постоянное срабатывание]
bool step();        // режим импульсного удержания
bool step(uint8_t clicks);    // режим импульсного удержания с предварительным накликиванием
bool releaseStep(); // отпущена после режима step
bool releaseStep(uint8_t clicks); // отпущена после режима step с предварительным накликиванием
uint8_t clicks;     // доступ к счётчику кликов
uint8_t hasClicks();            // вернёт количество кликов, если они есть
bool hasClicks(uint8_t num);    // проверка на наличие указанного количества кликов
```

##### GyverStepper2 by AlexGyver

###### Логика работы

- setTarget()/setTargetDeg() отправляет мотор на указанную позицию
- Движение происходит в tick(), который нужно опрашивать постоянно. Либо в tickManual, который нужно вызывать с периодом, полученным из getPeriod() и пересчитывать на каждом шаге
- tick() вернёт true, если мотор крутится
- ready() однократно вернёт true, если мотор доехал до цели и остановился
- Во время движения к цели можно вызвать pause(), тогда мотор доедет до точки и остановится, ready() не вернёт true
- Во время движения к цели можно вызвать stop(), тогда мотор затормозит с заданным ускорением, ready() не вернёт true
- Во время движения к цели можно вызвать brake(), тогда мотор остановится, ready() не вернёт true
- После остановки можно вызвать resume(), мотор продолжит движение к цели
- Постоянное вращение задаётся setSpeed()/setSpeedDeg(). Остановиться можно резко - stop() или brake()
- Скорость и ускорение можно задать в любое время, но применяются они после остановки мотора!

###### Инициализация

```cpp
GStepper2<STEPPER2WIRE> stepper(шаговНаОборот, step, dir);                          // драйвер step-dir
GStepper2<STEPPER2WIRE> stepper(шаговНаОборот, step, dir, en);                      // драйвер step-dir + пин enable
GStepper2<STEPPER4WIRE> stepper(шаговНаОборот, pin1, pin2, pin3, pin4);             // драйвер 4 пин
GStepper2<STEPPER4WIRE> stepper(шаговНаОборот, pin1, pin2, pin3, pin4, en);         // драйвер 4 пин + enable
GStepper2<STEPPER4WIRE_HALF> stepper(шаговНаОборот, pin1, pin2, pin3, pin4);        // драйвер 4 пин полушаг
GStepper2<STEPPER4WIRE_HALF> stepper(шаговНаОборот, pin1, pin2, pin3, pin4, en);    // драйвер 4 пин полушаг + enable
```

###### Использование

```cpp
// === наследуется из Stepper ====
void step();                                // сделать шаг
void invertEn(bool val);                    // инвертировать поведение EN пина
void reverse(bool val);                     // инвертировать направление мотора
void disable();                             // отключить питание и EN
void enable();                              // включить питание и EN
void attachStep(void (*handler)(uint8_t));  // подключить обработчик шага
void attachPower(void (*handler)(bool));    // подключить обработчик питания

int32_t pos;                                // текущая позиция в шагах
int8_t dir;                                 // направление (1, -1)

// ========= GStepper2 ==========
// тикер
bool tick();                                // тикер движения, вызывать часто. Вернёт true, если мотор движется
bool tickManual();                          // ручной тикер для вызова в прерывании таймера с периодом getPeriod(). Вернёт true, если мотор движется
bool ready();                               // однократно вернёт true, если мотор доехал до установленной позиции и остановился

// вращение
void setSpeed(int16_t speed);               // установить скорость в шагах/сек и запустить вращение
void setSpeed(float speed);                 // установить скорость в шагах/сек (float) и запустить вращение

// движение к цели
void setTarget(int32_t ntar, GS_posType type = ABSOLUTE);       // установить цель в шагах и опционально режим ABSOLUTE/RELATIVE
void setTargetDeg(int32_t ntar, GS_posType type = ABSOLUTE);    // установить цель в градусах и опционально режим ABSOLUTE/RELATIVE
int32_t getTarget();                                            // получить целевую позицию в шагах

void setAcceleration(uint16_t nA);          // установка ускорения в шаг/сек^2
void setMaxSpeed(int speed);                // установить скорость движения при следовании к позиции setTarget() в шагах/сек
void setMaxSpeed(float speed);              // установить скорость движения при следовании к позиции setTarget() в шагах/сек, float
void setMaxSpeedDeg(int speed);             // установить скорость движения при следовании к позиции в град/сек
void setMaxSpeedDeg(float speed);           // установить скорость движения при следовании к позиции в град/сек, float

void setCurrent(int32_t npos);              // установить текущую позицию
int32_t getCurrent();                       // получить текущую позицию
void reset();                               // сбросить текущую позицию в 0

// всякое
uint32_t getPeriod();                       // получить текущий период тиков
void brake();                               // резко остановить мотор
void pause();                               // пауза - доехать до заданной точки и ждать (ready() не вернёт true, пока ты на паузе)
void resume();                              // продолжить движение после остановки/паузы
uint8_t getStatus();                        // текущий статус: 0 - стоим, 1 - едем, 2 - едем к точке паузы, 3 - крутимся со скоростью, 4 - тормозим

// ===== ДЕФАЙНЫ НАСТРОЕК =====
// дефайнить перед подключением библиотеки
#define GS_NO_ACCEL                         // отключить модуль движения с ускорением (уменьшить вес кода)

#define GS_FAST_PROFILE размер_массива (например 10)
Включает быстрый планировщик скорости. Участок разгона/торможения разбивается 
на указанное количество отрезков (+8 байт SRAM на участок), на них скорость будет одинаковая. 
Это позволяет быстро вычислять скорость мотора и достигнуть 30000 шаг/с на участке разгона 
(в обычном режиме в два раза меньше).
```

###### Пример

```cpp
#include "GyverStepper2.h"
GStepper2<STEPPER2WIRE> stepper(2048, 2, 3);

void setup() {
  Serial.begin(9600);
  //stepper.enable();
  stepper.setMaxSpeed(100);     // скорость движения к цели
  stepper.setAcceleration(200); // ускорение
  stepper.setTarget(300);       // цель
}

bool dir = 1;
void loop() {
  stepper.tick();   // мотор асинхронно крутится тут

  // если приехали
  if (stepper.ready()) {
    dir = !dir;   // разворачиваем
    stepper.setTarget(dir * 300); // едем в другую сторону
  }

  // асинхронный вывод в порт
  static uint32_t tmr;
  if (millis() - tmr >= 30) {
    tmr = millis();
    Serial.println(stepper.pos);
  }
}
```
