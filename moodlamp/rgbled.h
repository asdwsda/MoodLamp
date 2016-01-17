#ifndef RGBLED_H
#define RGBLED_H

#include "Arduino.h"

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    RGB operator-(const RGB& other);
};

class RGBLed
{
public:
    RGBLed(uint8_t r_pin, uint8_t g_pin, uint8_t b_pin);
    void setColor(RGB rgb);
    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void setRed(uint8_t red);
    void setGreen(uint8_t green);
    void setBlue(uint8_t blue);

    RGB getColor();
    uint8_t getRed();
    uint8_t getGreen();
    uint8_t getBlue();
private:
    struct Channel {
        uint8_t pin;
        uint8_t value;
    } _red, _green, _blue;

    void setChannel(Channel* c, uint8_t value);
};

#endif
