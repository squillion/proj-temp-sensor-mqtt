#include <LCD.h>
#include "characters.h"
#include "ui.h"

#define POS_Y_TEMP 48
#define POS_Y_PRESSURE 96
#define POS_Y_LUX 144

void uiInit() {
  Tft.lcd_init();
  Tft.lcd_clear_screen(BLACK);

  // Display status icons
  uiDisplayStatus(STATUS_WIFI, STATUS_CONNECTING);
  uiDisplayStatus(STATUS_MQTT, STATUS_CONNECTING);

  // Display value area
  displayImage(0, POS_Y_TEMP, characters[CHR_SYMBOL_CELSIUS], CHR_SIZE, WHITE);
  displayImage(0, POS_Y_PRESSURE, characters[CHR_SYMBOL_MILLIBARS], CHR_SIZE, WHITE);
  displayImage(0, POS_Y_LUX, characters[CHR_SYMBOL_LUX], CHR_SIZE, WHITE);
  uiClearValues();

  // Make sure TFT shield's CS pins are high
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(10, HIGH);
}

// Display error on screen
void uiDisplayError(const char *error) {
  Tft.lcd_clear_screen(BLACK);
  Tft.lcd_display_string(0, 0, (const uint8_t*) error, 16, WHITE);
}

// Display status icons in upper right of screen
void uiDisplayStatus(int type, int status) {
  int colour = WHITE;
  if (status == STATUS_CONNECTING) {
    colour = GRAY;
  }
  if (type == STATUS_WIFI) {
    displayImage((LCD_HEIGHT - 1 - 40), 0, wifi, STATUS_SIZE, colour);
  } else if (type == STATUS_MQTT) {
    displayImage((LCD_HEIGHT - 1 - 16), 0, mqtt, STATUS_SIZE, colour);
  }
}

// Max length of displayed value
// Used to blank out previous value
int maxlen = 3;

void uiClearValues() {
  // Display three minus signs to show value has not been received
  for (int n = 1; n < (maxlen + 1); n++) {
    if (n < 4) {
      displayImage(n * CHR_SIZE, POS_Y_TEMP, characters[CHR_MINUS], CHR_SIZE, WHITE);
      displayImage(n * CHR_SIZE, POS_Y_PRESSURE, characters[CHR_MINUS], CHR_SIZE, WHITE);
      displayImage(n * CHR_SIZE, POS_Y_LUX, characters[CHR_MINUS], CHR_SIZE, WHITE);
    } else {
      displayImage(n * CHR_SIZE, POS_Y_TEMP, NULL, CHR_SIZE, BLACK);
      displayImage(n * CHR_SIZE, POS_Y_PRESSURE, NULL, CHR_SIZE, BLACK);
      displayImage(n * CHR_SIZE, POS_Y_LUX, NULL, CHR_SIZE, BLACK);
    }
  }
}

void uiDisplayTemp(int value) {
  displayNumber(CHR_SIZE, POS_Y_TEMP, value, CHR_UNITS_CELSIUS);
}

void uiDisplayPressure(int value) {
  displayNumber(CHR_SIZE, POS_Y_PRESSURE, value, CHR_UNITS_MILLIBARS);
}

void uiDisplayLux(int value) {
  displayNumber(CHR_SIZE, POS_Y_LUX, value, CHR_UNITS_LUX);
}

void displayImage(int xstart, int ystart, char* img, int imgsize, int colour)
{
  // Loop over array and display a pixel if the value is set
  for (int x = 0; x < imgsize; x++) {
    for (int y = 0; y < imgsize; y++) {
      int xp = x + xstart;
      int yp = y + ystart;

      // Convert to landscape
      int txp = xp;
      xp = yp;
      yp = (LCD_HEIGHT - 1) - txp;

      if (img == NULL) {
        Tft.lcd_draw_point(xp, yp, BLACK);
      } else {
        if (img[(y * imgsize) + x] != 0)
          Tft.lcd_draw_point(xp, yp, colour);
        else
          Tft.lcd_draw_point(xp, yp, BLACK);
      }
    }
  }
}

// Convert number into characters to be displayed
void displayNumber(int xstart, int ystart, int num, int units)
{
  int len = 0;
  int n = num;
  bool negative = false;

  if (num < 0) {
    negative = true;
    num = abs(num);
  }

  // Work out length of displayed value
  do {
    len++;
    n /= 10;
  } while (n);

  // Show minus if number is negative
  if (negative) {
    displayImage(xstart, ystart, characters[CHR_MINUS], CHR_SIZE, WHITE);
    len++;
  }

  // Display numbers
  int xpos = (len - 1) * CHR_SIZE;
  int unitxpos = xpos + CHR_SIZE;
  do
  {
    n = num % 10;
    displayImage(xstart + xpos, ystart, characters[CHR_0 + n], CHR_SIZE, WHITE);
    xpos -=  CHR_SIZE;
    num /= 10;
  } while (num);

  // Display units
  if (units != 0) {
    displayImage(xstart + unitxpos, ystart, characters[units], CHR_SIZE, WHITE);
    unitxpos += CHR_SIZE;
    len++;
  }

  // Blank out spaces after number if previous value was longer
  if (len > maxlen) {
    maxlen = len;
  } else {
    for (n = 0; n < (maxlen - len); n++) {
      displayImage(xstart + unitxpos, ystart, NULL, CHR_SIZE, BLACK);
      unitxpos += CHR_SIZE;
    }
  }
}
