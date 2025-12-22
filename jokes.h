#pragma once
#include <Arduino.h>

// Portal Hater Jokes Collection
// This header declares the joke array that will be defined in jokes.cpp
// DO NOT put the actual jokes here - they go in jokes.cpp

extern const char* const jokes[] PROGMEM;
extern const uint16_t JOKE_COUNT;