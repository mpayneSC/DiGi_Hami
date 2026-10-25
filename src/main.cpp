#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <SPI.h>
#include <AnimatedGIF.h>
#include <SPIFFS.h>

// Pin definitions for ESP32-C3 Super Mini
// Avoid GPIO2, GPIO8, GPIO9 - these are strapping pins on the C3
// and can interfere with boot mode selection if driven at reset.
#define TFT_SCLK  4   // SPI Clock
#define TFT_MOSI  6   // SPI Data
#define TFT_CS    7   // Chip Select
#define TFT_DC    3   // Data/Command
#define TFT_RST   1   // Reset

// Create display object (uses the default SPI bus, remapped in setup())
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

// Create GIF object
AnimatedGIF gif;
bool gifReady = false;

// File handle for GIF
File gifFile;

// Display dimensions
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240

// Compose a frame in RAM, then transfer it in one SPI transaction.
// 115,200 bytes; fits the C3 without PSRAM.
uint16_t frameBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
bool frameDrawn = false;

void GIFDraw(GIFDRAW *draw) {
  int y = draw->iY + draw->y;
  int width = gif.getCanvasWidth(), height = gif.getCanvasHeight();
  if (width <= 0 || height <= 0 || y < 0 || y >= height) return;
  // Scale the prepared 160x160 canvas to the full round 240x240 screen.
  int top = (y * DISPLAY_HEIGHT + height - 1) / height;
  int bottom = ((y + 1) * DISPLAY_HEIGHT + height - 1) / height;
  for (int x = 0; x < draw->iWidth; ++x) {
    int screenX = draw->iX + x;
    if (screenX < 0 || screenX >= width) continue;
    uint8_t pixel = draw->pPixels[x];
    if (draw->ucHasTransparency && pixel == draw->ucTransparent) continue;
    int left = (screenX * DISPLAY_WIDTH + width - 1) / width;
    int right = ((screenX + 1) * DISPLAY_WIDTH + width - 1) / width;
    for (int row = top; row < bottom; ++row)
      for (int col = left; col < right; ++col)
        frameBuffer[row * DISPLAY_WIDTH + col] = draw->pPalette[pixel];
  }
  frameDrawn = true;
}

// Function to open GIF file
void * GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = SPIFFS.open(fname);
  if (gifFile) {
    *pSize = gifFile.size();
    return (void *)&gifFile;
  }
  return NULL;
}

// Function to close GIF file
void GIFCloseFile(void *pHandle) {
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
    f->close();
}

// Function to read GIF file
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

// Function to seek in GIF file
int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  return pFile->iPos;
}

#include "pet.h"

void setup() {
  Serial.begin(115200);
  // Note: on the C3 Super Mini, make sure "USB CDC On Boot" is set to
  // "Enabled" in Tools menu, otherwise Serial output won't show up
  // over the USB port.

  // Explicitly map the SPI pins - the C3 has no fixed VSPI/HSPI like
  // the classic ESP32, so pins must be assigned via the GPIO matrix.
  SPI.begin(TFT_SCLK, -1 /* MISO not used */, TFT_MOSI, TFT_CS);

  // Initialize SPIFFS
  bool mounted = SPIFFS.begin(false);
  if (!mounted) {
    Serial.println("SPIFFS initialization failed!");
  } else {
    Serial.println("SPIFFS initialized successfully!");
  }

  // Initialize the display
  tft.begin(40000000); // Explicit 40 MHz display SPI clock
  tft.setRotation(0);
  tft.fillScreen(0x0000); // Black background

  Serial.println("Display initialized successfully!");

  // Initialize GIF decoder
  gif.begin(LITTLE_ENDIAN_PIXELS);

  petSetup(mounted);
}

void loop() {
  petLoop();
}

// Previous playback loop retained for review.
/*
void loop() {
  // Open the GIF file (replace with your GIF filename)
  // The GIF should be uploaded to SPIFFS with the name "animation.gif"
  if (gif.open("/animation.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());

    // Center the GIF on the round display if it's smaller than 240x240
    int xOffset = (DISPLAY_WIDTH - gif.getCanvasWidth()) / 2;
    int yOffset = (DISPLAY_HEIGHT - gif.getCanvasHeight()) / 2;

    // Play the GIF
    while (gif.playFrame(true, NULL)) {
      // The frame is automatically drawn by the GIFDraw callback
      // Add a small delay if the GIF plays too fast
      // delay(10); // Uncomment and adjust if needed
    }

    // Close the GIF file
    gif.close();

    // Small delay before restarting the loop
    delay(100);
  } else {
    Serial.println("Failed to open GIF file!");
    Serial.println("Make sure 'animation.gif' is uploaded to SPIFFS");
    delay(5000); // Wait 5 seconds before retrying
  }
}
*/
