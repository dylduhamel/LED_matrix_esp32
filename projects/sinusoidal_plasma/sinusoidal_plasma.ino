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

#include <FastLED.h>
// This is the library for manipulating RGB values

// Can be installed from the library manager (Search for "FastLED")
// https://github.com/FastLED/FastLED/tree/master

// --------------------------------
// -------   Matrix Config   ------
// --------------------------------

const int panelResX = 64;  // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 64;  // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 1; // Total number of panels chained one to another.

// -------------------------------
// -------   Other Config   ------
// -------------------------------

// placeholder for the matrix object
MatrixPanel_I2S_DMA *dma_display = nullptr;

uint16_t time_counter = 0, cycles = 0, fps = 0;
unsigned long fps_timer;

CRGB currentColor;
CRGBPalette16 palettes[] = {HeatColors_p, LavaColors_p, RainbowColors_p, RainbowStripeColors_p, CloudColors_p};
CRGBPalette16 currentPalette = palettes[0];

CRGB ColorFromCurrentPalette(uint8_t index = 0, uint8_t brightness = 255, TBlendType blendType = LINEARBLEND)
{
    return ColorFromPalette(currentPalette, index, brightness, blendType);
}

void displaySetup()
{
    HUB75_I2S_CFG mxconfig(
        panelResX,  // Module width
        panelResY,  // Module height
        panel_chain // Chain length
    );

    // This is how you enable the double buffer.
    // Double buffer can help with animation heavy projects
    mxconfig.double_buff = true;

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
    dma_display->setBrightness8(192); // range is 0-255, 0 - 0%, 255 - 100%

    // Allocate memory and start DMA display
    if (not dma_display->begin())
        Serial.println("****** I2S memory allocation failed ***********");
}

void setup()
{
    delay(3000); // power-up safety delay

    Serial.begin(115200);

    Serial.println(F("*****************************************************"));
    Serial.println(F("*        ESP32-HUB75-MatrixPanel-I2S-DMA DEMO       *"));
    Serial.println(F("*****************************************************"));

    displaySetup();

    // well, hope we are OK, let's draw some colors first :)
    Serial.println("Fill screen: RED");
    dma_display->fillScreenRGB888(255, 0, 0);
    delay(1000);

    Serial.println("Fill screen: GREEN");
    dma_display->fillScreenRGB888(0, 255, 0);
    delay(1000);

    Serial.println("Fill screen: BLUE");
    dma_display->fillScreenRGB888(0, 0, 255);
    delay(1000);

    Serial.println("Fill screen: Neutral White");
    dma_display->fillScreenRGB888(64, 64, 64);
    delay(1000);

    Serial.println("Fill screen: black");
    dma_display->fillScreenRGB888(0, 0, 0);
    delay(1000);

    // Set current FastLED palette
    currentPalette = RainbowColors_p;
    Serial.println("Starting plasma effect...");
    fps_timer = millis();
}

void loop()
{

    for (int x = 0; x < PANE_WIDTH; x++)
    {
        for (int y = 0; y < PANE_HEIGHT; y++)
        {
            int16_t v = 0;
            uint8_t wibble = sin8(time_counter);
            v += sin16(x * wibble * 3 + time_counter);
            v += cos16(y * (128 - wibble) + time_counter);
            v += sin16(y * x * cos8(-time_counter) / 8);

            currentColor = ColorFromPalette(currentPalette, (v >> 8) + 127); //, brightness, currentBlendType);
            dma_display->drawPixelRGB888(x, y, currentColor.r, currentColor.g, currentColor.b);
        }
    }

    ++time_counter;
    ++cycles;
    ++fps;

    if (cycles >= 1024)
    {
        time_counter = 0;
        cycles = 0;
        currentPalette = palettes[random(0, sizeof(palettes) / sizeof(palettes[0]))];
    }

    // print FPS rate every 5 seconds
    // Note: this is NOT a matrix refresh rate, it's the number of data frames being drawn to the DMA buffer per second
    if (fps_timer + 5000 < millis())
    {
        Serial.printf_P(PSTR("Effect fps: %d\n"), fps / 5);
        fps_timer = millis();
        fps = 0;
    }
} // end loop