/*
Arduino-MAX30100 oximetry / heart rate integrated sensor library
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define TFT_PIN_CS   10 // Arduino-Pin an Display CS   
#define TFT_PIN_DC   9  // Arduino-Pin an 
#define TFT_PIN_RST  8  // Arduino Reset-Pin

#include <SPI.h>
#include <Adafruit_GFX.h>    // Adafruit Grafik-Bibliothek
#include <Adafruit_ST7735.h> // Adafruit ST7735-Bibliothek

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);  // Display-Bibliothek Setup

#include <Wire.h>
//#include "MAX30102.h"
#include "MAX30102_PulseOximeter.h"

#define REPORTING_PERIOD_MS     100
// Sampling is tightly related to the dynamic range of the ADC.
// refer to the datasheet for further info
// #define SAMPLING_RATE                       MAX30102_SAMPRATE_100HZ

// The LEDs currents must be set to a level that avoids clipping and maximises the
// dynamic range
// #define IR_LED_CURRENT                      MAX30102_LED_CURR_50MA
// #define RED_LED_CURRENT                     MAX30102_LED_CURR_27_1MA

// The pulse width of the LEDs driving determines the resolution of
// the ADC (which is a Sigma-Delta).
// set HIGHRES_MODE to true only when setting PULSE_WIDTH to MAX30100_SPC_PW_1600US_16BITS
// #define PULSE_WIDTH                         MAX30102_SPC_PW_118US_16BITS
// #define HIGHRES_MODE                        true

// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
//MAX30102 sensor;
PulseOximeter pox;

uint32_t tsLastReport = 0;
//int32_t ir, red;

static const unsigned char PROGMEM logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,              //Logo2 and Logo3 are two bmp pictures that display on the OLED if called
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,  };

static const unsigned char PROGMEM logo3_bmp[] =
{ 0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00  };
    
// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
    tft.drawBitmap(120,40,logo3_bmp,32,32, ST7735_BLACK);
    tft.drawBitmap(125,45,logo2_bmp,24,21, ST7735_WHITE);
    tft.fillRect(65, 50, 21, 16, ST7735_BLACK);
    tft.setCursor(0,40);
    tft.print("Beat!");
    tft.setCursor(66,50);
    tft.print(int(pox.getHeartRate()));
    tft.setCursor(66,58);
    tft.print(abs(pox.getSpO2()));
    tsLastReport = millis();
}

void setup()
{
    Serial.begin(115200);
    tft.initR(INITR_BLACKTAB);   // ST7735-Chip initialisieren
    tft.setRotation(1);
    tft.fillScreen(ST7735_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30,4);
    tft.setTextColor(ST7735_WHITE);
    tft.println("Oximeter");
  
    tft.setTextSize(0);
    tft.println("Init pulse oximeter..");

    // Initialize the PulseOximeter instance
    // Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip
    if (!pox.begin()) {
        tft.print("FAILED");
        for(;;);
    } else {
        tft.print("SUCCESS");
    }

    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);
    tft.setCursor(0,50);
    tft.print("Heart rate:");
    tft.setCursor(0,58);
    tft.print("SpO2      :"); 
    tft.setCursor(88,50);
    tft.println("bpm");
    tft.print("SpO2      :");
    tft.setCursor(88,58);
    tft.print("%");           
}

void loop()
{
    // Make sure to call update as fast as possible
    pox.update();
    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
/*    sensor.update();
    sensor.getRawValues(&ir, &red);
   tft.setCursor(0,80);
if(ir>7500||red>4000){
    Serial.print(ir-10000);
    Serial.print('\t');
    Serial.println(red-15000);

}
        tft.fillRect(0, 80, 100, 8, ST7735_BLACK);*/
        tft.setCursor(0,40);
        tft.setTextColor(ST7735_BLACK);
        tft.print("Beat!");
        tft.setTextColor(ST7735_WHITE);
    tft.drawBitmap(125,45,logo2_bmp,24,21, ST7735_BLACK);
    tft.drawBitmap(120,40,logo3_bmp,32,32, ST7735_WHITE);        
    }
}
