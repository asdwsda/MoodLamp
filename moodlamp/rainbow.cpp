#include "rainbow.h"

void animator(Rainbow* rainbow) {
    rainbow->tick();
}

Rainbow::Rainbow(RGBLed *leds, int count_of_leds): leds(leds), count_of_leds(count_of_leds) { }

void Rainbow::start() {
    if (!running) {
        ticker.attach_ms(tick_delay, animator, this);
        running = true;
    }
}

void Rainbow::stop() {
    if (running) {
        ticker.detach();
        running = false;
        reset();
    }
}

void Rainbow::pause() {
    if (running) {
        ticker.detach();
        running = false;
    }
}

void Rainbow::reset() {
    counter = 0;
    for (int i; i < count_of_leds; i++) {
        leds[i].setColor({0, 0, 0});
    }
}

bool Rainbow::isRunning() {
    return running;
}

void Rainbow::tick() {
    float rad;
    RGB rgb;

    counter++;
    rad = (counter) * 20 * PI / 180;

    rgb.r = (int)((cos(rad/128) + 1) / 2 * 255);
    rgb.g = (int)((cos(rad/128 + 90) + 1) / 2 * 255);
    rgb.b = (int)((cos(rad/128 + 180) + 1) / 2 * 255);

    for (int i; i < count_of_leds; i++) {
        leds[i].setColor(rgb);
    }
}
