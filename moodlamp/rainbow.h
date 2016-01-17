#ifndef RAINBOW_H
#define RAINBOW_H

#include <Ticker.h>
#include "rgbled.h"

class Rainbow
{
public:
    Rainbow(RGBLed *leds, int count_of_leds);
    void start();
    void stop();
    void pause();
    void reset();
    void tick();
    bool isRunning();
private:
    RGBLed *leds;
    int count_of_leds;

    int tick_delay = 15;

    Ticker ticker;
    bool running = false;
    int counter = 0;

};

#endif
