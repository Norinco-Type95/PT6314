#include "PT6314.h"
#include "Arduino.h"

// 传统构造函数
PT6314::PT6314(uint8_t sck, uint8_t stb, uint8_t si) {
    _sck_pin = sck;
    _stb_pin = stb;
    _si_pin = si;
    
    pinMode(_sck_pin, OUTPUT);
    pinMode(_stb_pin, OUTPUT);
    pinMode(_si_pin, OUTPUT);

    // 硬件保护：锁定 Mode 3 空闲状态 (SCK High)
    digitalWrite(_stb_pin, HIGH);
    digitalWrite(_sck_pin, HIGH);
    digitalWrite(_si_pin, LOW);
}

// 结构体构造函数 (调用上面的初始化逻辑)
PT6314::PT6314(VFD_Pins pins) : PT6314(pins.SCK, pins.STB, pins.SI) {}

void PT6314::begin(uint8_t cols, uint8_t lines) {
    _displayfunction = VFD_FUNCTIONSET | VFD_8BITMODE | VFD_BRT_100;
    if (lines > 1) _displayfunction |= VFD_2LINE;

    delay(100); // 等待供电稳定

    // 1. 强制三阶复位协议
    for(int i = 0; i < 3; i++) {
        send(COMMAND, _displayfunction);
        delay(5);
    }

    // 2. 序列初始化
    clear(); 
    
    _displaycontrol = VFD_DISPLAYCONTROL | VFD_DISPLAYON | VFD_CURSOROFF | VFD_BLINKOFF;
    send(COMMAND, _displaycontrol);
    
    _displaymode = VFD_ENTRYMODESET | VFD_INCREMENT | VFD_CURSORSHIFTENABLED;
    send(COMMAND, _displaymode);
}

// 核心底层：SPI Mode 3 (CPOL=1, CPHA=1), MSB-First
void PT6314::sendByte(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        digitalWrite(_si_pin, (val & (1 << i)) ? HIGH : LOW);
        delayMicroseconds(4);    // 建立时间 (T_setup)
        
        digitalWrite(_sck_pin, LOW); // 下降沿采样
        delayMicroseconds(8);    // T_low
        
        digitalWrite(_sck_pin, HIGH);
        delayMicroseconds(4);    // T_high_end
    }
}

// 发送 16-bit 复合帧
void PT6314::send(uint8_t mode, uint8_t value) {
    digitalWrite(_stb_pin, LOW);
    delayMicroseconds(10); 

    sendByte(mode);           // 发送 Start Byte (0xF8/0xFA)
    delayMicroseconds(20);    // 关键 tWAIT 静默期
    sendByte(value);          // 发送 Data Byte

    delayMicroseconds(10);
    digitalWrite(_stb_pin, HIGH);
    
    // 指令处理开销
    if (value == VFD_CLEARDISPLAY) {
        delay(10); 
    } else {
        delayMicroseconds(200);
    }
}

void PT6314::setCursor(uint8_t col, uint8_t row) {
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    send(COMMAND, addr);
}

void PT6314::clear() {
    send(COMMAND, VFD_CLEARDISPLAY);
    delay(10); 
}

void PT6314::setBrightness(uint8_t brt) {
    _displayfunction &= ~0x03; 
    if (brt <= 25)      _displayfunction |= VFD_BRT_25;
    else if (brt <= 50) _displayfunction |= VFD_BRT_50;
    else if (brt <= 75) _displayfunction |= VFD_BRT_75;
    else                _displayfunction |= VFD_BRT_100;

    send(COMMAND, _displayfunction);
}

void PT6314::display() {
    _displaycontrol |= VFD_DISPLAYON;
    send(COMMAND, _displaycontrol);
}

void PT6314::noDisplay() {
    _displaycontrol &= ~VFD_DISPLAYON;
    send(COMMAND, _displaycontrol);
}

void PT6314::home() {
    send(COMMAND, VFD_RETURNHOME);
    delay(2);
}

size_t PT6314::write(uint8_t value) {
    send(DATA, value);
    return 1;
}

void PT6314::createChar(uint8_t location, uint8_t charmap[]) {
    location &= 0x7;
    send(COMMAND, VFD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++) {
        send(DATA, charmap[i]);
    }
}
