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

  Serial.begin(9600);
}

/*float variance = 0;
void loop() {
  float baseHue = float(analogRead(POT_HUE)) / 1023;
  float hueVariance = float(analogRead(POT_HUEVAR)) / 1023;

  byte saturation = analogRead(POT_SAT) >> 2;

  float curVariance = variance;
  for (word i = 0; i < NUM_LEDS; i++) {
    // First convert curVariance to a square wave
    float hue = curVariance <= 0.5 ? curVariance*2 : 1.0 - curVariance*2;
    // Then add hueVariance, scaled by curVariance, to baseHue
    hue = baseHue + hueVariance * hue;
    // Wrap around the color wheel as needed
    while (hue > 1.0) {
      hue--;
    }
    // Convert to byte
    hue *= 255;
    
    if (saturation) {
      leds[i] = CHSV(hue, saturation, 255);
    } else {
      // Special case: if saturation is zero (all white), use hue for brightness
      leds[i] = CHSV(0, 0, hue);
    }
    curVariance += 0.05;
    while (curVariance > 1.0) {
      curVariance--;
    }
  }
  variance += (float(analogRead(POT_SPEED)) / 1023 - 0.5) * 1 / FPS;
  FastLED.setBrightness(analogRead(POT_BRIGHT) >> 2);
  FastLED.show();
  FastLED.delay(1000/FPS);
}
*/



int variance = 0;
boolean doprints = false;

void loop() {
  // Base hue, 14 bits
  word baseHue = analogRead(POT_HUE) << 4;
  // How far away we go from that hue, 14 bits (to allow for things greater than it)
  int hueVariance = analogRead(POT_HUEVAR) << 4;
  // What distance it takes to cycle back to the base hue
  word hueWidth = sqrt(analogRead(POT_HUEWID)) * hueVariance / 255;
  if (doprints) {
    Serial.print(">>> basehue ");
    Serial.print(baseHue);
    Serial.print(" huewid ");
    Serial.print(hueWidth);
    Serial.print(" huevar ");
    Serial.print(hueVariance);
  }

  byte saturation = analogRead(POT_SAT) >> 2;
  byte hue;
  // START HERE: this is blinky and stuff when moved; start it with zero every time and it's fine
  //word curVariance = variance % (hueVariance<<1);
  while (variance < 0) variance += hueVariance*2;
  while (hueVariance > 0 && variance > (hueVariance*2)) variance -= hueVariance*2;
  int curVariance = variance;
  for (int i = 0; i < NUM_LEDS; i++) {
    hue = (baseHue + ((curVariance < hueVariance) ? (curVariance) : ((hueVariance << 1) - curVariance)) - 1) >> 6;
    //hue = (baseHue + ((curVariance < hueVariance) ? (curVariance) : ((hueVariance << 1) - curVariance)) - 1) >> 6;
    //hue = ((variance < hueVariance) ? (variance) : ((hueVariance << 1) - variance)); // 5 shifts from analog
    //hue = (baseHue + (variance < hueVariance) ? (variance) : ((hueVariance << 1) - variance)) >> 7; // 5 shifts from analog
    //hue = (baseHue + (((variance < hueVariance) ? (variance) : ((hueVariance << 1) - variance)) << 2)) >> 9;
    if (i == 0 && doprints) {
      Serial.print(" curvar ");
      Serial.print(curVariance);
      Serial.print(" thing ");
      Serial.print(((curVariance < hueVariance) ? (curVariance) : ((hueVariance << 1) - curVariance)));
      Serial.print(" hue ");
      Serial.println(hue);
    }
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
      leds[i] = CHSV(hue, saturation, 255);
    } else {
      // Special case: if saturation is zero (all white), use hue for brightness
      leds[i] = CHSV(0, 0, hue);
    }
    //Serial.println(curVariance);
    curVariance += hueWidth;
    while (curVariance > hueVariance*2) curVariance -= hueVariance*2;
  }
  word spd = analogRead(POT_SPEED);
  if (spd < 500 || spd > 524) {
    int thing = (analogRead(POT_SPEED) - 512) * 200 / FPS;
    variance += thing;
    if (doprints) Serial.println(thing);
  }
  //Serial.println("\n");
  FastLED.setBrightness(analogRead(POT_BRIGHT) >> 2);
  FastLED.show();
  FastLED.delay(1000/FPS);
}




/*void loop() {
  // Base hue
  word baseHue = analogRead(POT_HUE) << 6;
  // How far away we go from that hue
  word hueVariance = analogRead(POT_HUEVAR) << 6;
  // What distance it takes to cycle back to the base hue
  word hueWidth = analogRead(POT_HUEWID) << 3;
  
  byte saturation = analogRead(POT_SAT) / 4;

  word variance = 0;
  bool rising = true;
  for (int i = 0; i < NUM_LEDS; i++) {
    if (saturation) {
      leds[i] = CHSV((baseHue+variance) >> 8, saturation, 255);
    } else {
      // Special case: if saturation is zero (all white), use hue for brightness
      leds[i] = CHSV(0, 0, (baseHue+variance) >> 8);
    }
    if (rising) {
      variance += hueWidth;
      if (variance > hueVariance) {
        variance -= hueVariance;
        rising = false;
      }
    } else {
      variance -= hueWidth;
    }
    variance += rising ? hueWidth : -hueWidth;
    if (variance > hueVariance) {
      variance -= hueVariance;
    }
  }
  FastLED.setBrightness(analogRead(POT_BRIGHT) / 4);
  FastLED.show();
  FastLED.delay(1000/FPS);
}*/


/*
void loop() {
  // Base hue
  word baseHue = analogRead(POT_HUE) << 6;
  // How far away we go from that hue
  word maxHue = baseHue + analogRead(POT_HUEVAR) << 6;
  // What distance it takes to cycle back to the base hue
  word hueWidth = analogRead(POT_HUEWID) << 3;
  
  byte saturation = analogRead(POT_SAT) / 4;

  word hue = baseHue;
  for (int i = 0; i < NUM_LEDS; i++) {
    if (saturation) {
      leds[i] = CHSV(hue >> 8, saturation, 255);
    } else {
      // Special case: if saturation is zero (all white), use hue for brightness
      leds[i] = CHSV(0, 0, hue >> 8);
    }
    hue = constrain_reverse(hue + hueWidth, baseHue, maxHue);
  }
  FastLED.setBrightness(analogRead(POT_BRIGHT) / 4);
  FastLED.show();
  FastLED.delay(1000/FPS);
}*/


