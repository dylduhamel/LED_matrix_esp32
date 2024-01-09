/*******************************************************************
    Dylan Duhamel, Dec 14, 2023

    Simulation of exponential generation in random cell collisions.

 *******************************************************************/

// ----------------------------
// Additional Libraries - some of these will need to be installed.
// ----------------------------

#include <unistd.h>
#include <iostream>
#include <vector>
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

  // Default constructor
  Cell()
    : x(0), y(0), vx(0), vy(0), r(0), g(0), b(0), angle(0) {
    // Initialize all members to zero (or any other default value)
  }
};

bool run = true;

const int REFRESH_DELAY = 65;
const int MAX_CELLS = 15;
const int STARTING_CELLS = 7;
int CELL_COUNT = STARTING_CELLS;

// Define the size of each bin in the grid
const int binSize = 2;  // Adjust as needed

// Calculate the number of bins required for the display
const int numBinsX = (128 / binSize) + 1;  // +1 to handle edge cases
const int numBinsY = (64 / binSize) + 1;

// 2D vector for the grid
std::vector<std::vector<std::vector<int>>> grid(numBinsX, std::vector<std::vector<int>>(numBinsY));

// Array of cells
Cell cells[MAX_CELLS];

// placeholder for the matrix object
MatrixPanel_I2S_DMA* dma_display = nullptr;

// For game time
unsigned long lastTime = 0;

void initGrid() {
  Serial.println("Initializing Grid...");
  for (int i = 0; i < numBinsX; ++i) {
    if (i >= grid.size()) {
      Serial.println("Grid X index out of bounds");
      break;
    }

    for (int j = 0; j < numBinsY; ++j) {
      if (j >= grid[i].size()) {
        Serial.println("Grid Y index out of bounds");
        break;
      }

      grid[i][j].clear();
    }
  }
  Serial.println("Grid Initialized");
}


void printCells() {
  for (int i = 0; i < CELL_COUNT; ++i) {
    Serial.print("Cell ");
    Serial.print(i + 1);
    Serial.print("  (");
    Serial.print(cells[i].x);
    Serial.print(", ");
    Serial.print(cells[i].y);
    Serial.print(")\n");
  }
}

void initCells() {
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

void clearCells() {
  for (int i = 0; i < MAX_CELLS; ++i) {
    cells[i] = Cell();  // Assign a new default-constructed Cell
  }
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

void spawnCell(int x, int y) {
  if (CELL_COUNT > MAX_CELLS) {
    return;
  }
  
  // Create new cell
  Cell newCell;

  newCell.x = (float)x;
  newCell.y = (float)y;
  newCell.angle = random(0, 2 * PI);        // Random direction 0 to 2π
  float speed = random(1, 3);               // Random speed 1 to 3
  newCell.vx = speed * cos(newCell.angle);  // Velocity component in x direction
  newCell.vy = speed * sin(newCell.angle);  // Velocity component in y direction
  newCell.r = random(256);
  newCell.g = random(256);
  newCell.b = random(256);

  cells[CELL_COUNT] = newCell;
  CELL_COUNT += 1;
}

void checkCollisions() {
  initGrid();  // Initialize the grid for this frame
  Serial.println("initGrid exited successfully");

  // Populate the grid with cell indices
  for (int i = 0; i < CELL_COUNT; i++) {
    // Ensure that the cell's position is within the bounds of the grid
    int cellX = max(0, min(panelResX * panel_chain - 1, (int)cells[i].x));
    int cellY = max(0, min(panelResY * panel_chain - 1, (int)cells[i].y));

    // Calculate bin indices
    int binX = min(numBinsX - 1, cellX / binSize);
    int binY = min(numBinsY - 1, cellY / binSize);

    grid[binX][binY].push_back(i);
  }


  Serial.println("Cells populated successfully");

  // Flag to check if a cell has been spawned in this frame
  bool cellSpawned = false;

  // Check for collisions in each bin
  for (int x = 0; x < numBinsX && !cellSpawned; ++x) {
    for (int y = 0; y < numBinsY && !cellSpawned; ++y) {
      if (grid[x][y].size() > 1) {
        // Collision detected, spawn only one new cell per frame where collision is detected
        int cellX = x * binSize + binSize / 2;
        int cellY = y * binSize + binSize / 2;
        Serial.print("Spawn cell at (");
        Serial.print(cellX);
        Serial.print(", ");
        Serial.print(cellY);
        Serial.println(")");
        spawnCell(cellX, cellY);
        cellSpawned = true;
      }
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

  dma_display->setRotation(2);

  // Allocate memory and start DMA display
  if (not dma_display->begin())
    Serial.println("****** I2S memory allocation failed ***********");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  // Initialize display
  displaySetup();

  initCells();
}

void loop() {
  initGrid();
  Serial.println("A");
  drawCells();
  Serial.println("B");
  checkCollisions();
  Serial.println("C");

  delay(REFRESH_DELAY);

  // Check if the cell count exceeds the maximum allowed
  if (CELL_COUNT > MAX_CELLS) {
    // Reset the cell count to 5
    CELL_COUNT = 5;

    // Clear all cell data
    clearCells();

    // Reinitialize the cells with the new count
    initCells();
  }

  dma_display->clearScreen();
}
