// Screensaver Module for XtraCardputer
// Contains simplified screensaver functionality

#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#include <M5Cardputer.h>
#include <math.h>

// Define PI as float to avoid narrowing conversion warnings
#ifndef PI
#define PI 3.14159265359f
#endif

// Animation modes - subset of original effects for memory efficiency
enum AnimationMode {
  BOXED = 0,
  SPIRAL,
  MATRIX,
  PLASMA,
  FIRE,
  STARS,
  TUNNEL,
  WAVE,
  LIFE,
  MODE_COUNT = 9
};

// Screensaver state
extern AnimationMode currentMode;
extern float animationTime;
extern unsigned long lastModeChange;
extern unsigned long frameCount;

// Function declarations
void initScreensaver();
void handleScreensaver();
void drawCurrentEffect();
void showEffectInfo();
int mandelbrotIterations(float cx, float cy);

// Effect implementations
void drawBoxed();
void drawSpiral();
void drawMatrix();
void drawPlasma();
void drawFire();
void drawStars();
void drawTunnel();
void drawWave();
void drawLife();

#endif