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
