#ifndef PT6314_H
#define PT6314_H

#include <inttypes.h>
#include "Print.h"

// 模式定义
#define COMMAND 0xF8
#define DATA 0xFA

// 指令集
#define VFD_CLEARDISPLAY 0x01
#define VFD_RETURNHOME 0x02
#define VFD_ENTRYMODESET 0x04
#define VFD_DISPLAYCONTROL 0x08
#define VFD_CURSORSHIFT 0x10
#define VFD_FUNCTIONSET 0x20
#define VFD_SETCGRAMADDR 0x40
#define VFD_SETDDRAMADDR 0x80

// Entry Mode
#define VFD_INCREMENT 0x02
#define VFD_CURSORSHIFTENABLED 0x00

// Display Control
#define VFD_DISPLAYON 0x04
#define VFD_CURSOROFF 0x00
#define VFD_BLINKOFF 0x00

// Function Set & Brightness
#define VFD_8BITMODE 0x10
#define VFD_2LINE 0x08
#define VFD_BRT_100 0x00
#define VFD_BRT_75 0x01
#define VFD_BRT_50 0x02
#define VFD_BRT_25 0x03

// 结构体定义在类外，确保 .ino 可见
struct VFD_Pins {
    uint8_t SCK;
    uint8_t STB;
    uint8_t SI;
};

class PT6314 : public Print {
public:
    PT6314(uint8_t sck, uint8_t stb, uint8_t si);
    PT6314(VFD_Pins pins); // 新增结构体构造

    void begin(uint8_t cols, uint8_t lines);
    void clear();
    void home();
    void display();
    void noDisplay();
    void setCursor(uint8_t col, uint8_t row);
    void setBrightness(uint8_t brt);
    void createChar(uint8_t location, uint8_t charmap[]);
    
    virtual size_t write(uint8_t value);
    void send(uint8_t mode, uint8_t value);
    void sendByte(uint8_t val);

private:
    uint8_t _sck_pin, _stb_pin, _si_pin;
    uint8_t _displayfunction, _displaycontrol, _displaymode;
};

#endif
