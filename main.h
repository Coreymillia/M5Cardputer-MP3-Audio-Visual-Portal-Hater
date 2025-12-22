#ifndef MAIN_H
#define MAIN_H

#include <M5Cardputer.h>

// Color definitions if not already defined
#ifndef GRAY
#define GRAY 0x8410
#endif

#ifndef BLACK  
#define BLACK 0x0000
#endif

#ifndef WHITE
#define WHITE 0xFFFF
#endif

#ifndef RED
#define RED 0xF800
#endif

#ifndef GREEN
#define GREEN 0x07E0
#endif

#ifndef BLUE
#define BLUE 0x001F
#endif

#ifndef YELLOW
#define YELLOW 0xFFE0
#endif

#ifndef CYAN
#define CYAN 0x07FF
#endif

// Key definitions for M5Cardputer
#ifndef KEY_ESC
#define KEY_ESC 0x1B  // ESC key
#endif

#ifndef KEY_ENTER
#define KEY_ENTER 0x0D  // Enter key
#endif

#ifndef KEY_UP
#define KEY_UP 0xB5  // Up arrow
#endif

#ifndef KEY_DOWN  
#define KEY_DOWN 0xB6  // Down arrow
#endif

#ifndef KEY_LEFT
#define KEY_LEFT 0xB4  // Left arrow
#endif

#ifndef KEY_RIGHT
#define KEY_RIGHT 0xB7  // Right arrow
#endif

// Forward declarations from main.cpp
void enterMenu();
void enterScreensaver();
void displayMenu();
void handleMenu();
void selectMenuItem();

#endif