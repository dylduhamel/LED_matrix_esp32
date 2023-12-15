/*******************************************************************
    Dylan Duhamel, Dec 14, 2023

    Bouncing squares of different colours and sizes

    Based on mrfaptastic's example in ESP32-HUB75-MatrixPanel-I2S-DMA
    https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/blob/master/examples/2_PatternPlasma

 *******************************************************************/

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA


// --------------------------------
// -------   Matrix Config   ------
// --------------------------------

const int panelResX = 64;   // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 64;   // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 1;  // Total number of panels chained one to another.

// -------------------------------
// -------   Other Config   ------
// -------------------------------

// placeholder for the matrix object
MatrixPanel_I2S_DMA *dma_display = nullptr;

uint16_t fps = 0;
unsigned long fps_timer;

// Pixel coords
int x, y = 0;

const int refreshDelay = 100;

int getRandomCoord() {
    return random(64);
  }

void displaySetup() {
  HUB75_I2S_CFG mxconfig(
    panelResX,   // Module width
    panelResY,   // Module height
    panel_chain  // Chain length
  );

  // This is how you enable the double buffer.
  // Double buffer can help with animation heavy projects
  //mxconfig.double_buff = true;

  // If you are using a 64x64 matrix you need to pass a value for the E pin
  // The trinity connects GPIO 18 to E.
  // This can be commented out for any smaller displays (but should work fine with it)
  mxconfig.gpio.e = 18;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  mxconfig.clkphase = false;

  // Some matrix panels use different ICs for driving them and some of them have strange quirks.
  // If the display is not working right, try this.
  // mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  // let's adjust default brightness to about 75%
  //dma_display->setBrightness8(192); // range is 0-255, 0 - 0%, 255 - 100%

  // Allocate memory and start DMA display
  if (not dma_display->begin())
    Serial.println("****** I2S memory allocation failed ***********");
}

void setup() {
  Serial.begin(115200);

  Serial.println(F("*****************************************************"));
  Serial.println(F("*        ESP32-HUB75-MatrixPanel-I2S-DMA DEMO       *"));
  Serial.println(F("*****************************************************"));

  displaySetup();

  Serial.println("Starting random pixel...");
  fps_timer = millis();
}

void loop() {
  x = getRandomCoord();
  y = getRandomCoord();
  dma_display->drawPixelRGB888(x, y, 255, 0, 0);

  // print FPS rate every 5 seconds
  // Note: this is NOT a matrix refresh rate, it's the number of data frames being drawn to the DMA buffer per second
  if (fps_timer + 5000 < millis()) {
    Serial.printf_P(PSTR("Effect fps: %d\n"), fps / 5);
    fps_timer = millis();
    fps = 0;
  }
  delay(refreshDelay);
  dma_display->clearScreen();
}