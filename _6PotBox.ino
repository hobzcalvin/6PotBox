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
#define POT_NOISE     A5
#define POT_SPEED   A7
#define POT_BRIGHTSAT  A6

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

  random16_set_seed(analogRead(POT_SPEED));
  //Serial.begin(9600);

  last_ms = millis();
}


int variance = 0;
boolean doprints = false;
uint16_t noiseIn = 0;
boolean reversed = false;

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
  word brightsat = analogRead(POT_BRIGHTSAT);
  byte saturation;
  if (brightsat < 512) {
    FastLED.setBrightness(brightsat >> 1);
    saturation = 255;
  } else {
    FastLED.setBrightness(255);
    saturation = (1023 - brightsat) >> 1;
  }
  uint16_t noNoise;
  uint16_t reverseProb;
  int potNoise = analogRead(POT_NOISE);
  if (potNoise > 511) {
    noNoise = 255 - byte(potNoise >> 1);
    reverseProb = 0;
  } else {
    noNoise = 255;
    reverseProb = 511 - potNoise;
  }
  
  int spd = analogRead(POT_SPEED);
  int speed_adder = 0;
  if (spd < 512 - MIDDLE_TOL) {
    speed_adder = 512 - MIDDLE_TOL - spd;
  } else if (spd > 512 + MIDDLE_TOL) {
    speed_adder = 512 + MIDDLE_TOL - spd;
  }
  uint16_t target = reverseProb * abs(speed_adder) * 5 / FPS;
  if (reverseProb > 0 && random16() < target) {
    /*Serial.print("REVERSE! ");
    Serial.println(target);*/
    reversed = !reversed;
  }
  if (reversed) {
    speed_adder *= -1;
  }
  noiseIn += speed_adder / (FPS/4);

  // START HERE: IDEA: for other side of noise pot, increase likelihood of direction change!

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
      if (noNoise < 255) {
        byte brt = inoise8(noiseIn, i << 4, 0);
        byte srt = inoise8(noiseIn, i << 4, 0xF000);
        leds[i] = CHSV(hue, min(max(noNoise, uint16_t(srt)*2), saturation), max(noNoise, uint16_t(brt)));
      } else {
        leds[i] = CHSV(hue, saturation, 255);
      }
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
