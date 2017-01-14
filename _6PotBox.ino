#include <FastLED.h>


/*
 * Potentiometer Layout:
 * 
 *   A9=hue
 *                A4=noise
 *   A8=hueVar
 *                A5=speed
 *   A7=hueWidth
 *                A6=brightsat
 *       
 *     ||  <-- cable out  
 *     ||
 */

#define POT_HUE     A9
#define POT_HUEVAR  A8
#define POT_HUEWID  A7
#define POT_NOISE     A4
#define POT_SPEED   A5
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

#define DEBUG false

uint32_t last_ms;

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
    Serial.println("HELLO WORLD!");
  }

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

  last_ms = millis();
}


int32_t variance = 0;
uint16_t noiseIn = 0;
boolean reversed = false;

void loop() {
/*
  Serial.print(analogRead(POT_HUE));
  Serial.print("\t");
  Serial.print(analogRead(POT_HUEVAR));
  Serial.print("\t");
  Serial.print(analogRead(POT_HUEWID));
  Serial.print("\t");
  Serial.print(analogRead(POT_NOISE));
  Serial.print("\t");
  Serial.print(analogRead(POT_SPEED));
  Serial.print("\t");
  Serial.println(analogRead(POT_BRIGHTSAT));
  return;
*/
  
  
  // Base hue, 14 bits
  word baseHue = analogRead(POT_HUE) << 4;
  //if (DEBUG) { Serial.print(baseHue); Serial.print("\t"); }
  // How far away we go from that hue, 14 bits (to allow for things greater than it)
  int hueVariance = analogRead(POT_HUEVAR) << 4;
  //if (DEBUG) { Serial.print(hueVariance); Serial.print("\t"); }
  // What distance it takes to cycle back to the base hue
  word hueWidth = sqrt16(analogRead(POT_HUEWID)) * hueVariance / 255;
  //if (DEBUG) { Serial.print(hueWidth); Serial.print("\t"); }

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
  if (DEBUG) { Serial.print("pn"); Serial.print(potNoise); }
  if (potNoise > 511) {
    noNoise = 255 - byte(potNoise >> 1);
    reverseProb = 0;
    if (DEBUG) { Serial.print("\tnnz"); Serial.print(noNoise); }
  } else {
    noNoise = 255;
    reverseProb = 511 - potNoise;
    if (DEBUG) { Serial.print("\trev"); Serial.print(reverseProb); }
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


  //if (DEBUG) { Serial.print(variance); Serial.print("=>"); }
  int hack = 0;
  while (variance < 0 && hack++ < 50) variance += hueVariance*2;
  while (hueVariance > 0 && variance > (hueVariance*2) && hack++ < 50) variance -= hueVariance*2;
  //if (DEBUG) { Serial.print(variance); Serial.println(); }
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
      if (potNoise > 511) {
        int hueplay = int(inoise8(noiseIn*4, i << 4, 0)) - 127;
        if (DEBUG && !i) { Serial.print("\thp?"); Serial.print(hueplay); }
        if (hueplay > 0 && hueplay > (255-noNoise)) {
          hueplay = 255 - noNoise;
        } else if (hueplay < 0 && -hueplay > (255-noNoise)) {
          hueplay = -(255 - noNoise);
        }
        uint16_t srt = inoise8(noiseIn, i << 4, 0xF000) * 2 - 100;
        srt = max(noNoise, srt);
        srt = min(srt, saturation);
        if (DEBUG && !i) {
          Serial.print("\thp"); Serial.print(hueplay);
          Serial.print("\th"); Serial.print(hue);
          Serial.print("\thh"); Serial.print((int(hue) + hueplay + 256) % 256);
          Serial.print("\tsrt"); Serial.println(srt);
        }
        leds[i] = CHSV((int(hue) + hueplay + 256) % 256, srt, 255);
      } else {
        if (DEBUG && !i) { Serial.println(); }
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
