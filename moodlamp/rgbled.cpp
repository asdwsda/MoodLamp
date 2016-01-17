#include "rgbled.h"

RGB RGB::operator-(const RGB& other) {
    RGB sub;
    sub.r = this->r - other.r;
    sub.g = this->g - other.g;
    sub.b = this->b - other.b;
    return sub;
}

RGBLed::RGBLed(uint8_t r_pin, uint8_t g_pin, uint8_t b_pin) {
    _red.pin = r_pin;
    _red.value = 0;
    pinMode(_red.pin, OUTPUT);

    _green.pin = g_pin;
    _green.value = 0;
    pinMode(_green.pin, OUTPUT);

    _blue.pin = b_pin;
    _blue.value = 0;
    pinMode(_blue.pin, OUTPUT);
}

void RGBLed::setColor(RGB rgb) {
    setRed(rgb.r);
    setGreen(rgb.g);
    setBlue(rgb.b);
}

void RGBLed::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    setChannel(&_red, red);
    setChannel(&_green, green);
    setChannel(&_green, blue);
}

void RGBLed::setRed(uint8_t red) {
    setChannel(&_red, red);
}

void RGBLed::setGreen(uint8_t green) {
    setChannel(&_green, green);
}

void RGBLed::setBlue(uint8_t blue) {
    setChannel(&_blue, blue);
}

void RGBLed::setChannel(Channel* channel, uint8_t value) {
    channel->value = value;
    analogWrite(channel->pin, channel->value);
}

RGB RGBLed::getColor() {
    RGB rgb = {_red.value, _green.value, _blue.value};
    return rgb;
}

uint8_t RGBLed::getRed() {
    return _red.value;
}

uint8_t RGBLed::getGreen() {
    return _green.value;
}

uint8_t RGBLed::getBlue() {
    return _blue.value;
}
