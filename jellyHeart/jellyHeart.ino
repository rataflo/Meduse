
/*
 * Sketch for module integrated in jellyfish.
 * 2 rings of neopixel (12 pixell & 16 pixel)
 * Attiny 85.
 * IR led. NEC protocol.
 * Florent Galès 2019.
 * Licence Rien à branler / Do what the fuck you want license.
 */

#include <Adafruit_NeoPixel.h>
#include <IRremote.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define LED_PIN     6 //1 = Physical pin 6 on attiny85
#define REMOTE_PIN 7 // 0 = Physical pin 5 on attiny85
#define NUMPIXELS      28

// Neopixel
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
// Remote
IRrecv irrecv(REMOTE_PIN);
decode_results results;

// Brightness
#define BRIGTHNESS 50
// Refresh delay in ms
#define DELAY 100
// rgb transition value between palette colors.
#define OFFSETTRANS 3

#define TAILLE_PALETTE 5
typedef struct
{
  byte r;
  byte g;
  byte b;
} palette;

// Store palette
const palette PALETTE[TAILLE_PALETTE]  = {
  {132, 46, 27},
  {38, 196, 238},
  {84, 114, 174},
  {244, 102, 27},
  {0, 51, 102}
};

// Working variable.
palette transpalette;
// Current item in palette
byte currColor = 0;

// Pulse variables;
#define PULSE_DURATION 600
long startPulseMillis = 0;
long lastMillis = 0;
palette prePulsePalette;

void setup() {
  Serial.begin(9600);
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket

  // load palette from eeprom
  
  // Init working palette
  transpalette = PALETTE[0];
  
  // remote receiver
  irrecv.enableIRIn();
    
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(BRIGTHNESS);
}

void loop() {
  long currentMillis = millis();
  /*
  offset = offset == TAILLE_PALETTE - 1 ? 0 : offset + 1;
  //if (irrecv.decode(&results)){
    byte offsetPalette = offset;
    for(int i=0;i<NUMPIXELS;i++){
      Serial.println(PALETTE[offsetPalette].r);
      pixels.setPixelColor(i, pixels.Color(PALETTE[offsetPalette].r, PALETTE[offsetPalette].g, PALETTE[offsetPalette].b)); // Moderately bright green color.
      offsetPalette = offsetPalette == TAILLE_PALETTE - 1 ? 0 : offsetPalette + 1;
    }
    pixels.show(); // This sends the updated pixel color to the hardware.
    //irrecv.resume();
  //}
  */
  if(irrecv.decode(&results)){
    Serial.println("receive");
    if(startPulseMillis == 0){
      startPulseMillis = currentMillis;
      prePulsePalette = transpalette;
    }
    startPulseMillis = startPulseMillis == 0 ? currentMillis : startPulseMillis;
    irrecv.resume();
  }

  // No pulse we refresh periodically.
  if(startPulseMillis == 0 && currentMillis > lastMillis + DELAY){
    singleTransition(currentMillis);
    lastMillis = currentMillis;
  } else if(startPulseMillis > 0){ // we refresh a every loop during pulse.
    singleTransition(currentMillis);
  }
  
}

void singleTransition(long currentMillis){

  // if we dispaly a pulse
  if(startPulseMillis > 0){
    
    // Ascending pulse
    if(currentMillis - startPulseMillis < PULSE_DURATION / 2){
      transpalette.r = transpalette.r + 10  >= 255 ? 255 : transpalette.r + 10;
      transpalette.g = transpalette.g - 10 <= 0 ? 0 : transpalette.g - 10;
      transpalette.b = transpalette.b - 10 <= 0 ? 0 : transpalette.b - 10;
      
    } else if(currentMillis - startPulseMillis > PULSE_DURATION / 2 && currentMillis - startPulseMillis < PULSE_DURATION){ // descending pulse
      transpalette.r = transpalette.r - 10  <= prePulsePalette.r ? prePulsePalette.r : transpalette.r - 10;
      transpalette.g = transpalette.g + 10 >= prePulsePalette.g ? prePulsePalette.g : transpalette.g + 10;
      transpalette.b = transpalette.b + 10 >= prePulsePalette.b ? prePulsePalette.b : transpalette.b + 10;
      
    } else {// end of pulse
      transpalette = prePulsePalette;
      startPulseMillis = 0;
    }
    Serial.print("R=");
    Serial.print(transpalette.r);
    Serial.print("G=");
    Serial.print(transpalette.g);
    Serial.print("B=");
    Serial.println(transpalette.b);
    
  } else { // No pulse to display.
  
    if(transpalette.r < PALETTE[currColor].r){
      transpalette.r = transpalette.r + OFFSETTRANS == 255 || transpalette.r + OFFSETTRANS > PALETTE[currColor].r ? PALETTE[currColor].r : transpalette.r + OFFSETTRANS;
    }else if(transpalette.r > PALETTE[currColor].r){
      transpalette.r = transpalette.r - OFFSETTRANS == 0 || transpalette.r - OFFSETTRANS < PALETTE[currColor].r  ? PALETTE[currColor].r : transpalette.r - OFFSETTRANS;
    }
  
    if(transpalette.g < PALETTE[currColor].g){
      transpalette.g = transpalette.g + OFFSETTRANS == 255 || transpalette.g + OFFSETTRANS > PALETTE[currColor].g ? PALETTE[currColor].g : transpalette.g + OFFSETTRANS;
    }else if(transpalette.g > PALETTE[currColor].g){
      transpalette.g = transpalette.g - OFFSETTRANS == 0 || transpalette.g - OFFSETTRANS < PALETTE[currColor].g  ? PALETTE[currColor].g : transpalette.g - OFFSETTRANS;
    }
  
    if(transpalette.b < PALETTE[currColor].b){
      transpalette.b = transpalette.b + OFFSETTRANS == 255 || transpalette.b + OFFSETTRANS > PALETTE[currColor].b ? PALETTE[currColor].b : transpalette.b + OFFSETTRANS;
    }else if(transpalette.b > PALETTE[currColor].b){
      transpalette.b = transpalette.b - OFFSETTRANS == 0 || transpalette.b - OFFSETTRANS < PALETTE[currColor].b  ? PALETTE[currColor].b : transpalette.b - OFFSETTRANS;
    }
  }

  for(int i=0;i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(transpalette.r, transpalette.g, transpalette.b));
  }
  
  pixels.show();
  if(startPulseMillis == 0 && transpalette.r == PALETTE[currColor].r && transpalette.g == PALETTE[currColor].g && transpalette.b == PALETTE[currColor].b){
    currColor = currColor == TAILLE_PALETTE - 1 ? 0 : currColor + 1;
  }
}

void circleTransition(){
  for(int i=0;i<NUMPIXELS;i++){
    if(transpalette.r < PALETTE[currColor].r){
      transpalette.r = transpalette.r + OFFSETTRANS == 255 || transpalette.r + OFFSETTRANS > PALETTE[currColor].r ? PALETTE[currColor].r : transpalette.r + OFFSETTRANS;
    }else if(transpalette.r > PALETTE[currColor].r){
      transpalette.r = transpalette.r - OFFSETTRANS == 0 || transpalette.r - OFFSETTRANS < PALETTE[currColor].r  ? PALETTE[currColor].r : transpalette.r - OFFSETTRANS;
    }
  
    if(transpalette.g < PALETTE[currColor].g){
      transpalette.g = transpalette.g + OFFSETTRANS == 255 || transpalette.g + OFFSETTRANS > PALETTE[currColor].g ? PALETTE[currColor].g : transpalette.g + OFFSETTRANS;
    }else if(transpalette.g > PALETTE[currColor].g){
      transpalette.g = transpalette.g - OFFSETTRANS == 0 || transpalette.g - OFFSETTRANS < PALETTE[currColor].g  ? PALETTE[currColor].g : transpalette.g - OFFSETTRANS;
    }
  
    if(transpalette.b < PALETTE[currColor].b){
      transpalette.b = transpalette.b + OFFSETTRANS == 255 || transpalette.b + OFFSETTRANS > PALETTE[currColor].b ? PALETTE[currColor].b : transpalette.b + OFFSETTRANS;
    }else if(transpalette.b > PALETTE[currColor].b){
      transpalette.b = transpalette.b - OFFSETTRANS == 0 || transpalette.b - OFFSETTRANS < PALETTE[currColor].b  ? PALETTE[currColor].b : transpalette.b - OFFSETTRANS;
    }

  
    pixels.setPixelColor(i, pixels.Color(transpalette.r, transpalette.g, transpalette.b));
    
    if(transpalette.r == PALETTE[currColor].r && transpalette.g == PALETTE[currColor].g && transpalette.b == PALETTE[currColor].b){
      currColor = currColor == TAILLE_PALETTE - 1 ? 0 : currColor + 1;
    }
  }
  pixels.show(); 
}
