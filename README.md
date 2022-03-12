# Petfilamachine
Простая прошивка для станка переработки бутылок из PET в пруток для 3d-принтера.
A simple firmware for a machine for processing plastic bottles into filament for a 3D printer running esp8266 

===========

Я начал делать станок для протяжки ленты из ПЭТ бутылок в пруток для 3д принтера. Я нашел готовый станок ПетПулл2 и он показался мне замечательным. Однако вскрылся один минус. Автор станка пок акой-то причине выкладывает для него только скомпилированную прошивку в хекс формате.
Я решил что возможно для меня будет неплохой практикой попробовать написать свою прошивку. И чтобы не возникало еще каких-либо вопросов - я взял вместо ардуино нано - есп8266. Под этот контроллер у автора станка прошивки нет, и сам микроконтроллер в данное время дефицита полупроводников - дешевле чем Ардуино нано.

Скетч компилируется в Arduino IDE.

===========

I started making a machine for pulling a ribbon from PET bottles into a rod for a 3D printer. I found a ready-made PetPull2 machine and it seemed wonderful to me. However, there was one downside. For some reason, the author of the machine uploads for him only the compiled firmware in hex format.
I decided that maybe it would be a good practice for me to try to write my own firmware. And so that there are no more questions - I took instead of arduino nano - esp8266. The author of the machine does not have firmware for this controller, and the microcontroller itself is currently in short supply of semiconductors - cheaper than Arduino nano. 


// ================ BUTTON ================
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
