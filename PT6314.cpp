#include "PT6314.h"
#include <inttypes.h>
#include "Arduino.h"

// When PT6314 is initialized, the internal status after power supply has been reset is as follows:
// 1. Display Clear: 20H (space code) fills the DDRAM
// 2. Address Counter is set to 00H
// 3. Address Counter is pointed to the DDRAM
// 4. Display ON/OFF: D=0, C=0, B=0 (Display OFF)
// 5. Entry Mode Set: I/D=1, S=0 (Increment, Cursor Shifts are enabled)
// 6. Function Set: DL=1, N=1 (8-Bit MCU Interface, 2-Line Display are enabled.)
// 7. Brightness Control: BR0=BR1=0 (Brightness = 100%) 

PT6314::PT6314(uint8_t sck, uint8_t stb, uint8_t si)
{
    init(sck, stb, si);
}

void PT6314::init(uint8_t sck, uint8_t stb, uint8_t si)
{
    _sck_pin = sck;
    _stb_pin = stb;
    _si_pin = si;

    pinMode(_sck_pin, OUTPUT);
    pinMode(_stb_pin, OUTPUT);
    pinMode(_si_pin, OUTPUT);

    _displayfunction = VFD_FUNCTIONSET | VFD_8BITMODE | VFD_1LINE | VFD_BRT_100;
}

void PT6314::begin(uint8_t cols, uint8_t lines)
{
    // 根据传入的行数参数动态修改模式位
    if (lines > 1) {
        _displayfunction |= VFD_2LINE; // 强制开启 2 行模式 (N=1)
    } else {
        _displayfunction &= ~VFD_2LINE; // 设为 1 行模式 (N=0)
    }

    // 执行硬件初始化序列
    delay(50); 
    send(COMMAND, _displayfunction); // 发送 Function Set
    
    _displaycontrol = VFD_DISPLAYON | VFD_CURSOROFF | VFD_BLINKOFF;
    display(); // 开启显示
    clear();   // 清屏
    
    _displaymode = VFD_ENTRYMODESET | VFD_INCREMENT | VFD_CURSORSHIFTENABLED;
    send(COMMAND, _displaymode); // 设置进入模式
}

// Sends to the PT6314 2 bytes:
// Start Byte (mode)
// Instruction/Data Byte (value)
void PT6314::send(uint8_t mode, uint8_t value)
{
    int full_payload = value | (mode << 8);
    digitalWrite(_stb_pin, 0);
    delayMicroseconds(1);
    digitalWrite(_sck_pin, 0);
    for (int i = 15; i > -1; i--)
    {
        digitalWrite(_si_pin, bitRead(full_payload, i));
        clockPulse();
    }
    digitalWrite(_sck_pin, 1);
    digitalWrite(_stb_pin, 1);
    delayMicroseconds(1);
}

// Sends clock pulse to the PT6314 clock pin
void PT6314::clockPulse()
{
    delayMicroseconds(2);
    digitalWrite(_sck_pin, 1);
    delayMicroseconds(2);
    digitalWrite(_sck_pin, 0);
}

// The CLEAR DISPLAY Instruction performs the following operations:
// 1. Fills all Display Data RAM (DDRAM) location with 20H (Blank Character).
// 2. Clears the contents of the Address Counter (ACC) to 00H.
// 3. Sets the display for Zero Character Shift (Returns to original position.)
// 4. Sets the Address Counter to point to the Display Data RAM (DDRAM).
// 5. If the cursor is displayed, this instruction will move the cursor to the 
// left most character in the upper display line.
//
// 6. Sets the Address Counter (ACC) to increment on each access of the DDRAM or CGRAM. 
void PT6314::clear()
{
    send(COMMAND, VFD_CLEARDISPLAY);
    delayMicroseconds(2000);
}

// The CURSOR HOME Instruction performs the following operations:
// 1. Clears the contents of the Address Counter (ACC) to 00H.
// 2. Sets the Address Counter to point to the Display Data RAM (DDRAM).
// 3. Sets the Display for Zero Character Shift (Returns to the original position).
// 4. If the cursor is displayed, this instruction moves the cursor 
// to the left most character in the upper line display. 
void PT6314::home()
{
    send(COMMAND, VFD_RETURNHOME);
}

// Sets display off
// Note - this command will not clear DDRAM
void PT6314::noDisplay()
{
    _displaycontrol &= ~VFD_DISPLAYON;
    send(COMMAND, _displaycontrol);
}

// Sets display on 
void PT6314::display()
{
    _displaycontrol |= VFD_DISPLAYON;
    send(COMMAND, _displaycontrol);
}

// Turns the underline cursor off
void PT6314::noCursor()
{
    _displaycontrol &= ~VFD_CURSORON;
    send(COMMAND, _displaycontrol);
}

// Turns the underline cursor on
void PT6314::cursor()
{
    _displaycontrol |= VFD_CURSORON;
    send(COMMAND, _displaycontrol);
}

// Turn the blinking cursor off
void PT6314::noBlink()
{
    _displaycontrol &= ~VFD_BLINKON;
    send(COMMAND, _displaycontrol);
}

// Turn the blinking cursor on
void PT6314::blink()
{
    _displaycontrol |= VFD_BLINKON;
    send(COMMAND, _displaycontrol);
}

// Allow us to change cursor position
void PT6314::setCursor(uint8_t col, uint8_t row)
{
    uint8_t position = col;
    if (row > 0)
    {
        position += 0x40;
    }
    send(COMMAND, VFD_SETDDRAMADDR | position);
}

// Allows us to set display brightness
// PT6314 have 4 brightness levels
void PT6314::setBrightness(uint8_t brt)
{
    // 1. 核心修复：清除低 2 位亮度位 (0x03)，并确保不改动 VFD_2LINE 等关键位
    _displayfunction &= ~0x03; 

    // 2. 根据百分比设置新位
    switch (brt)
    {
        case 100: _displayfunction |= VFD_BRT_100; break;
        case 75:  _displayfunction |= VFD_BRT_75;  break;
        case 50:  _displayfunction |= VFD_BRT_50;  break;
        case 25:  _displayfunction |= VFD_BRT_25;  break;
        default:  _displayfunction |= VFD_BRT_100; break;
    }

    // 3. 发送包含正确 N (行数) 位的完整指令
    send(COMMAND, _displayfunction);
}
// Write raw data
inline size_t PT6314::write(uint8_t value)
{
    send(DATA, value);
    return 1; // assume sucess
}
// Allows us to fill the first 8 CGRAM locations
// with custom characters.
// Note - in some displays, there are only 7 rows. 
// In this case, the eighth raw is responsible for the cursor.
void PT6314::createChar(uint8_t location, uint8_t charmap[])
{
    location &= 0x7; // we only have 8 locations 0-7
    send(COMMAND, VFD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++)
    {
        send(DATA, charmap[i]);
    }
}
