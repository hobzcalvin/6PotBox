#include <FastLED.h>

/*
 * Potentiometer Layout:
 * 
 *   A4=hue
 *                A5=sat
 *   A8=hueVar
 *                A7=speed
 *   A9=hueWidth
 *                A6=bright
 *       
 *     ||  <-- cable out  
 *     ||
 */


#define POT_HUE     A4
#define POT_HUEVAR  A8
#define POT_HUEWID  A9
#define POT_SAT     A5
#define POT_SPEED   A7
#define POT_BRIGHT  A6

#define LED_PIN     13

#define DATA_PIN    17
#define CHIPSET     WS2811
#define COLOR_ORDER RGB
#define LED_SETTINGS CHIPSET, DATA_PIN, COLOR_ORDER

#define FPS 60
#define NUM_LEDS 100
CRGB leds[NUM_LEDS];

#define MIDDLE_TOL  12


uint32_t last_ms;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  FastLED.addLeds<LED_SETTINGS>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);

  fill_solid(leds, NUM_LEDS, 0xFF0000);
  digitalWrite(LED_PIN, HIGH);
  FastLED.show();
  delay(500);
  fill_solid(leds, NUM_LEDS, 0x00FF00);
  digitalWrite(LED_PIN, LOW);
  FastLED.show();
  delay(500);
  fill_solid(leds, NUM_LEDS, 0x0000FF);
  digitalWrite(LED_PIN, HIGH);
  FastLED.show();
  delay(500);
  fill_solid(leds, NUM_LEDS, 0);
  digitalWrite(LED_PIN, LOW);
  FastLED.show();
  delay(500);

  //Serial.begin(9600);

  last_ms = millis();
}


int variance = 0;
boolean doprints = false;
uint16_t noiseIn = 0;

void loop() {
  
  // Base hue, 14 bits
  word baseHue = analogRead(POT_HUE) << 4;
  // How far away we go from that hue, 14 bits (to allow for things greater than it)
  int hueVariance = analogRead(POT_HUEVAR) << 4;
  // What distance it takes to cycle back to the base hue
  word hueWidth = sqrt16(analogRead(POT_HUEWID)) * hueVariance / 255;

  // START HERE:
  // When hueVariance is zero, speed does...something
  // 
  word brightsat = analogRead(POT_BRIGHT);
  byte saturation;
  if (brightsat < 512) {
    FastLED.setBrightness(brightsat >> 1);
    saturation = 255;
  } else {
    FastLED.setBrightness(255);
    saturation = (1023 - brightsat) >> 1;
  }
  uint16_t noNoise = 255 - (analogRead(POT_SAT) >> 2);
  
  int spd = analogRead(POT_SPEED);
  int speed_adder = 0;
  if (spd < 512 - MIDDLE_TOL) {
    speed_adder = 512 - MIDDLE_TOL - spd;
  } else if (spd > 512 + MIDDLE_TOL) {
    speed_adder = 512 + MIDDLE_TOL - spd;
  }
  noiseIn += speed_adder / (FPS/4);

  /*if (doprints) {
    Serial.print(">>> basehue ");
    Serial.print(baseHue);
    Serial.print(" huewid ");
    Serial.print(hueWidth);
    Serial.print(" huevar ");
    Serial.print(hueVariance);
  }*/
  
  while (variance < 0) variance += hueVariance*2;
  while (hueVariance > 0 && variance > (hueVariance*2)) variance -= hueVariance*2;
  int curVariance = variance;
  byte hue;
  for (int i = 0; i < NUM_LEDS; i++) {
    hue = (baseHue + ((curVariance < hueVariance) ? (curVariance) : ((hueVariance << 1) - curVariance)) - 1) >> 6;
    //hue = (baseHue + ((curVariance < hueVariance) ? (curVariance) : ((hueVariance << 1) - curVariance)) - 1) >> 6;
    //hue = ((variance < hueVariance) ? (variance) : ((hueVariance << 1) - variance)); // 5 shifts from analog
    //hue = (baseHue + (variance < hueVariance) ? (variance) : ((hueVariance << 1) - variance)) >> 7; // 5 shifts from analog
    //hue = (baseHue + (((variance < hueVariance) ? (variance) : ((hueVariance << 1) - variance)) << 2)) >> 9;
    /*if (i == 0 && doprints) {
      Serial.print(" curvar ");
      Serial.print(curVariance);
      Serial.print(" thing ");
      Serial.print(((curVariance < hueVariance) ? (curVariance) : ((hueVariance << 1) - curVariance)));
      Serial.print(" hue ");
      Serial.println(hue);
    }*/
    /*Serial.print(variance);
    Serial.print('=');
    Serial.print(hue);
    Serial.print('=');
    hue = baseHue + hue;
    Serial.print(hue);
    Serial.print('=');
    hue = hue >> 6;
    Serial.print(hue);*/
    //Serial.print(',');
    if (saturation) {
      /*uint16_t ms = millis() >> 1;
      byte brt = inoise8(ms, i<<7);//inoise8(variance >> 4, uint16_t(i) << 4);*/

      byte brt = inoise8(noiseIn, i << 4, 0);
      byte srt = inoise8(noiseIn, i << 4, 0xF000);
      /*if (i == 0) {
        Serial.print(noNoise);
        Serial.print(' ');
        Serial.print(noiseIn);
        Serial.print(' ');
        Serial.println(brt);
      }*/
      leds[i] = CHSV(hue, min(max(noNoise, uint16_t(srt)*2), saturation), max(noNoise, uint16_t(brt)));
    } else {
      // Special case: if saturation is zero (all white), use hue for brightness
      leds[i] = CHSV(0, 0, hue);
    }
    //Serial.println(curVariance);
    curVariance += hueWidth;
    while (curVariance > hueVariance*2) curVariance -= hueVariance*2;
  }
  // Increment/decrement variance for next time
  variance += speed_adder * 200 / FPS;
  //Serial.println("\n");
  FastLED.show();
  FastLED.delay(1000/FPS);
}
