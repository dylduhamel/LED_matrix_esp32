/*******************************************************************
    Dylan Duhamel, Dec 14, 2023

    Simulation of exponential generation in random cell collisions.

 *******************************************************************/

// ----------------------------
// Additional Libraries - some of these will need to be installed.
// ----------------------------

#include <array>
#include <unistd.h>
#include <iostream>
#include <map>
#include <utility>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA

// --------------------------------
// -------   Matrix Config   ------
// --------------------------------

const int panelResX = 64;   // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 64;   // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 2;  // Total number of panels chained one to another.

// -------------------------------
// -------   Other Config   ------
// -------------------------------

struct Cell {
  float x, y;       // position
  float vx, vy;     // velocity
  uint8_t r, g, b;  // color
  float angle;      // direction of movement
};

const int CELL_COUNT = 12;
const int refreshDelay = 100;

// Array of cells
Cell cells[CELL_COUNT];

// Map to check for collisions
std::map<std::pair<int, int>, int> coordMap;

// placeholder for the matrix object
MatrixPanel_I2S_DMA* dma_display = nullptr;

// For game time
unsigned long lastTime = 0;

int getRandomCoord() {
  return random(64);
}

void drawCells() {
  for (int i = 0; i < CELL_COUNT; i++) {
    // Update position
    cells[i].x += cells[i].vx;
    cells[i].y += cells[i].vy;

    // Check for boundary collision
    if (cells[i].x < 0 || cells[i].x > 127)
      cells[i].vx *= -1;
    if (cells[i].y < 0 || cells[i].y > 63)
      cells[i].vy *= -1;

    dma_display->drawPixelRGB888(cells[i].x, cells[i].y, cells[i].r, cells[i].g, cells[i].b);
  }
}

void checkCollisions() {
  if (!coordMap.empty())
    coordMap.clear();

  for (int i = 0; i < CELL_COUNT; i++) {
    // Cast float coordinates to int before creating the pair
    int x = static_cast<int>(cells[i].x);
    int y = static_cast<int>(cells[i].y);

    std::pair<int, int> coordPair = std::make_pair(x, y);

    // Check if map for coord is not empty
    if (coordMap.count(coordPair) > 0) {
      coordMap[coordPair] += 1;
    } else {
      coordMap[coordPair] = 1;
    }
  }

  // Check for collisions (map value > 1)
  for (const auto& elem : coordMap) {
    if (elem.second > 1) {
      Serial.println("Collision");
      Serial.print("Collision detected at coordinates (");
      Serial.print(elem.first.first);
      Serial.print(", ");
      Serial.print(elem.first.second);
      Serial.print(") with count ");
      Serial.println(elem.second);
    }
  }
}


void displaySetup() {
  HUB75_I2S_CFG mxconfig(
    panelResX,   // Module width
    panelResY,   // Module height
    panel_chain  // Chain length
  );

  // This is how you enable the double buffer.
  // Double buffer can help with animation heavy projects
  // mxconfig.double_buff = true;

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
  dma_display->setBrightness8(100);  // range is 0-255, 0 - 0%, 255 - 100%

  // Allocate memory and start DMA display
  if (not dma_display->begin())
    Serial.println("****** I2S memory allocation failed ***********");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  // Initialize display
  displaySetup();

  // Initialize cells with random positions, velocities, and directions
  for (int i = 0; i < CELL_COUNT; i++) {
    cells[i].x = random(128);
    cells[i].y = random(64);
    cells[i].angle = random(0, 2 * PI);         // Random direction 0 to 2π
    float speed = random(1, 3);                 // Random speed 1 to 3
    cells[i].vx = speed * cos(cells[i].angle);  // Velocity component in x direction
    cells[i].vy = speed * sin(cells[i].angle);  // Velocity component in y direction
    cells[i].r = random(256);
    cells[i].g = random(256);
    cells[i].b = random(256);
  }
}

void loop() {
  drawCells();

  checkCollisions();

  // Simulation delay
  delay(refreshDelay);

  dma_display->clearScreen();
}