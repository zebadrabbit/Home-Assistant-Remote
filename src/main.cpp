#include <Arduino.h>
#include "app.h"

void setup() {
    App::instance().init();
}

void loop() {
    App::instance().loop();
}