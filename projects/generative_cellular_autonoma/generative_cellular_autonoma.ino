/*******************************************************************
    Dylan Duhamel, Dec 14, 2023

    Simulation on LED matrix of generative cellular structures.

    Based on mrfaptastic's example in ESP32-HUB75-MatrixPanel-I2S-DMA
    https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/blob/master/examples/2_PatternPlasma

 *******************************************************************/

// ----------------------------
// Additional Libraries - some of these will need to be installed.
// ----------------------------

#include <array>
#include <unistd.h>

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

std::array<std::array<int, panelResY>, panelResX> display{};
std::array<std::array<int, panelResY>, panelResX> swap{};

// placeholder for the matrix object
MatrixPanel_I2S_DMA *dma_display = nullptr;

// For game time
unsigned long lastTime = 0;

// Pixel coords
int x, y = 0;

// Square size for seed
const int squareSize = 5;

// Game colors
int r, g, b = 150;

const int refreshDelay = 100;
const int GAME_WIDTH = 64;
const int GAME_HEIGHT = 64;

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

// Determine neighbors and status
bool isAlive(int x, int y, std::array<std::array<int, GAME_HEIGHT>, GAME_WIDTH> &game) {
  int aliveNeighbors = 0;

  // Side neighbors
  // // W
  if (x > 0 && game[x - 1][y] == 1)
    aliveNeighbors += 1;
  // E
  if (x < GAME_WIDTH && game[x + 1][y] == 1)
    aliveNeighbors += 1;
  // N
  if (y > 0 && game[x][y - 1] == 1)
    aliveNeighbors += 1;
  // S
  if (y < GAME_HEIGHT && game[x][y + 1] == 1)
    aliveNeighbors += 1;

  // Corner neighbors
  // NW
  if (x > 0 && y > 0 && game[x - 1][y - 1] == 1)
    aliveNeighbors += 1;
  // NE
  if (x < GAME_WIDTH && y > 0 && game[x + 1][y - 1] == 1)
    aliveNeighbors += 1;
  // SW
  if (x > 0 && y < GAME_HEIGHT && game[x - 1][y + 1] == 1)
    aliveNeighbors += 1;
  // SE
  if (x < GAME_WIDTH && y < GAME_HEIGHT && game[x + 1][y + 1])
    aliveNeighbors += 1;

  // Square fractal
  if (game[x][y] == 1 || aliveNeighbors == 1 || aliveNeighbors == 5) return true;
  
  // Square fractal
  // if (game[x][y] == 1 || aliveNeighbors == 1 || aliveNeighbors == 5) return true;
  // if (game[x][y] == 0 && aliveNeighbors == 3) return true;
  // if (game[x][y] == 1 && aliveNeighbors == 8) return false;

    return false;
}

void initGameSeed() {
  int startX = (GAME_WIDTH - squareSize) / 2;
  int startY = (GAME_HEIGHT - squareSize) / 2;

  for (int i = startY; i < startY + squareSize; ++i) {
    for (int j = startX; j < startX + squareSize; ++j) {
      display[i][j] = 1;
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println(F("*****************************************************"));
  Serial.println(F("*        ESP32-HUB75-MatrixPanel Conway's GOL       *"));
  Serial.println(F("*****************************************************"));

  initGameSeed();

  displaySetup();

  Serial.println("Starting random pixel...");
}

void loop() {
  // Get time since program start
  unsigned long currentTime = millis();

  // Check each pixel based on rules
  for (int i = 0; i < GAME_WIDTH; i++) {
    for (int j = 0; j < GAME_HEIGHT; j++) {
      // Check pixel for rules
      swap[i][j] = isAlive(i, j, display) ? 1 : 0;
    }
  }

  // Draw pixels
  for (int i = 0; i < GAME_WIDTH; i++) {
    for (int j = 0; j < GAME_HEIGHT; j++) {
      if (swap[i][j]) {
        dma_display->drawPixelRGB888(i, j, r, g, b);
      } else {
        dma_display->drawPixelRGB888(i, j, 0, 0, 0);
      }
    }
  }

  // Swap matrix
  std::copy(swap.begin(), swap.end(), display.begin());

  delay(refreshDelay);
  dma_display->clearScreen();

  // Reset game after 1 minuite
  if (currentTime - lastTime >= 50000) {
    initGameSeed();

    r = random(255);
    g = random(255);
    g = random(255);

    lastTime = currentTime;
  }
}