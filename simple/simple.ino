// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <boarddefs.h>
#include <IRremote.h>
#include <IRremoteInt.h>
#include <ir_Lego_PF_BitStreamEncoder.h>
#include <FastLED.h>

const int RECV_PIN = 4;
IRrecv irrecv(RECV_PIN);
decode_results results;

#define LED_PIN     3
#define NUM_LEDS    28
#define BRIGHTNESS  125
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
CRGB leds[NUM_LEDS];

uint8_t brightness = BRIGHTNESS;

// Head. If control is for both head and body we use head variables.
CRGBPalette16 currentPaletteHead;
TBlendType    currentBlendingHead = LINEARBLEND;
byte speedLedHead = 0;
unsigned long lastMillisHead = 0;
byte redHead = 0;
byte greenHead = 0;
byte blueHead = 0;
byte numPaletteHead = 0;
byte driftLedHead = 0;
static uint8_t colorIndexHead = 0;

// Body
CRGBPalette16 currentPaletteBody;
TBlendType    currentBlendingBody = LINEARBLEND;
byte speedLedBody = 0;
unsigned long lastMillisBody = 0;
byte redBody = 0;
byte greenBody = 0;
byte blueBody = 0;
byte numPaletteBody = 0;
byte driftLedBody = 0;
static uint8_t colorIndexBody = 0;

// Menus
int sequence = 0;
byte choixCouleur = 0;
unsigned long lastMenu = 0;
unsigned long lastRemote = 0;
byte menuPalette = 0; // 1 = change speed, 2 = change drift led.
byte controlMode = 0; // Choose to control head, bottom or both.
byte lightAll = 2; // 2 = light only 1/2 led, 1 = All led.
bool changeBrightness = false;

void setup() {

  delay(1000); // power-up safety delay

  // remote receiver
  irrecv.enableIRIn();
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.begin(9600);
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (irrecv.decode(&results)){
    if(results.value != 0xFFFFFFFF){// long press, we keep the old value pressed.
      lastMenu = results.value; // last key pressed.
    } 
    irrecv.resume();

    if(currentMillis > lastRemote + 200){// button threshold
      lastRemote = currentMillis;
      if(lastMenu == 0xFFA857){ // 8 : Change brightness
        changeBrightness = changeBrightness ? false : true;
        menuPalette = 0;
        choixCouleur = 0;
        Serial.println("Brightness");
      }
      else if(lastMenu == 0xFF906F){ // 9 : Toggle between all led lighted or only 1/2.
        lightAll = lightAll == 2 ? 1 : 2;
        changeBrightness = false;
        Serial.println("1/2 Led");
      }
      else if(lastMenu == 0xFF9867){ // 0 : control head & bottom on the same led pattern
        controlMode = 0;
        changeBrightness = false;
        Serial.println("CTRL BODY & HEAD");
        
      }else if(lastMenu == 0xFF6897){ // * : control head 
        controlMode = 1;
        changeBrightness = false;
        Serial.println("CTRL HEAD");
        
      }else if(lastMenu == 0xFFB04F){ // # : control body 
        controlMode = 2;
        changeBrightness = false;
        Serial.println("CTRL BODY");
        
      }else if(lastMenu == 0xFFA25D){ // 1 : choose palette mode
        Serial.println("PALETTE MODE");
        changeBrightness = false;
        sequence = 1;
        menuPalette = 0;
        changePalette();
        
      } else if(lastMenu == 0xFF629D){ // 2 : choose color mode
        Serial.println("COLOR MODE");
        sequence = 2;
        changeBrightness = false;
        
      } else if(lastMenu == 0xFF22DD){ // 4
        changeBrightness = false;
        if(sequence == 1){//speed
          Serial.println("CHANGE SPEED");
          menuPalette = 1;
        } else if(sequence == 2){
          Serial.println("CHANGE RED");
          choixCouleur = 1; // Change red color
        }
        
      } else if(lastMenu == 0xFF02FD){ // 5
        changeBrightness = false;
        if(sequence == 1){//drift led
          Serial.println("CHANGE DRIFT LED");
          menuPalette = 2;
        } else if(sequence == 2){
          Serial.println("CHANGE GREEN");
          choixCouleur = 2; // Change green color
        }
        
      } else if(lastMenu == 0xFFC23D){ // 6
        changeBrightness = false;
        if(sequence == 1){//blend type
          Serial.println("CHANGE BLEND TYPE");
          menuPalette = 3;
          toggleBlend();
        } else if(sequence == 2){
          Serial.println("CHANGE BLUE");
          choixCouleur = 3; // Change blue color
        }   
      } else if(lastMenu == 0xFF18E7){ // UP
        if(sequence == 1 && !changeBrightness){
          if(menuPalette == 1){
            changeSpeed(true);
          }else if(menuPalette == 2){
            changeDriftLed(true);
          }
          //Serial.print("SPEED=");Serial.print(speedLed);Serial.print(" DRIFT=");Serial.print(driftLed);Serial.print(" BLEND=");Serial.println(currentBlending);
        } else if(sequence == 2 && !changeBrightness){
          changeColor(true);
        }
        if(changeBrightness){
          brightness = brightness + 1;
          FastLED.setBrightness(brightness);
          Serial.print("BRIGHTNESS=");Serial.println(brightness);
        }
        
      } else if(lastMenu == 0xFF4AB5){ // DOWN
        if(sequence == 1 && !changeBrightness){
          if(menuPalette == 1){
            changeSpeed(false);
          }else if(menuPalette == 2){
            changeDriftLed(false);
          }
          //Serial.print("SPEED=");Serial.print(speedLed);Serial.print(" DRIFT=");Serial.print(driftLed);Serial.print(" BLEND=");Serial.println(currentBlending);
        } else if(sequence == 2 && !changeBrightness){
          changeColor(false);
        }
  
        if(changeBrightness){
          brightness = brightness - 1;
          FastLED.setBrightness(brightness);
          Serial.print("BRIGHTNESS=");Serial.println(brightness);
        }
      } 
    }
    
  }

  
  if(sequence == 1){
    
    fillFromPalette(currentMillis);
  }
  else if( sequence == 2){
    fillColor();
  }
  
}

void show(){
  // the first 12 pixel are GRB and we light only 1/2 leds
  for( int i = 0; i < 12; i++) {
    byte red = leds[i].r;
    leds[i].r = leds[i].g;
    leds[i].g = red;
  }
  FastLED.show();
}

void fillColor(){
  //Serial.println(colorValue);
  for(int i = 0; i< NUM_LEDS; i++) {
    if((i % lightAll) == 0){
      leds[i].r = i >= 12 ? redBody : redHead;
      leds[i].g = i >= 12 ? greenBody : greenHead;
      leds[i].b = i >= 12 ? blueBody : blueHead;

      if(i < 12){
        // First 12 led are GRB.
        byte red = leds[i].r;
        leds[i].r = leds[i].g;
        leds[i].g = red;
      }
    } else {
      leds[i] = CRGB::Black;
    }
  }
  if(irrecv.isIdle()){
    FastLED.show();
  }
}

void fillFromPalette(unsigned long currentMillis){
  for( int i = 0; i < NUM_LEDS; i++) {
    if((i % lightAll) == 0){
      if(i < 12 && currentMillis > lastMillisHead + speedLedHead){
        leds[i] = ColorFromPalette( currentPaletteHead, colorIndexHead, brightness, currentBlendingHead);
        colorIndexHead += driftLedHead;
        // First 12 led are GRB.
        byte red = leds[i].r;
        leds[i].r = leds[i].g;
        leds[i].g = red;
      }
      if(i >=12 && currentMillis > lastMillisBody + speedLedBody){
        leds[i] = ColorFromPalette( currentPaletteBody, colorIndexBody, brightness, currentBlendingBody);
        colorIndexBody += driftLedBody;
      } 
    } else {
      leds[i] = CRGB::Black;
    }
  }
  
  if(currentMillis > lastMillisBody + speedLedBody || currentMillis > lastMillisHead + speedLedHead){
    if(irrecv.isIdle()){
      FastLED.show();
    }
  }
  
  if(currentMillis > lastMillisBody + speedLedBody){
    lastMillisBody = currentMillis;
    colorIndexBody++; // One step in the palette array.
  } 
  if(currentMillis > lastMillisHead + speedLedHead) {
    lastMillisHead = currentMillis;
    colorIndexHead++; // One step in the palette array.
  }
}

void changePalette()
{
  CRGBPalette16 paletteChoose;
  byte numPalette = 0;
  // start over
  if(controlMode == 2){
    numPaletteBody = numPaletteBody == 8 ? 1 : numPaletteBody + 1;
    numPalette = numPaletteBody;
  } else {
    numPaletteHead = numPaletteHead == 8 ? 1 : numPaletteHead + 1;
    numPalette = numPaletteHead;
  }
  
  Serial.print("PALETTE=");
  switch (numPalette){
    case 1:
      paletteChoose = RainbowColors_p;
      Serial.println("RainbowColors_p");         
      break;
    case 2:
      paletteChoose = RainbowStripeColors_p;
      Serial.println("RainbowStripeColors_p");   
      break;
    case 3:
      paletteChoose = CloudColors_p;
      Serial.println("CloudColors_p");             
      break;
    case 4:
      paletteChoose = PartyColors_p;
      Serial.println("PartyColors_p");         
      break;
    case 5:
      paletteChoose = LavaColors_p;
      Serial.println("LavaColors_p");          
      break;
    case 6:
      paletteChoose = ForestColors_p;
      Serial.println("ForestColors_p");            
      break;
    case 7:
      paletteChoose = HeatColors_p;
      Serial.println("HeatColors_p");         
      break;  
    case 8:
      paletteChoose = OceanColors_p;
      Serial.println("OceanColors_p");           
      break; 
         
  }

  if(controlMode == 2){
    currentPaletteBody = paletteChoose;
    colorIndexBody = 0;
  } else if(controlMode == 1){
    currentPaletteHead = paletteChoose;
    colorIndexHead = 0;
  } else {
    currentPaletteBody = paletteChoose;
    currentPaletteHead = paletteChoose;
    colorIndexBody = 0;
    colorIndexHead = 0;
  }
}

void toggleBlend()
{
  TBlendType blendChoose;
  blendChoose = controlMode == 2 ? currentBlendingBody : currentBlendingHead;
  blendChoose = blendChoose == NOBLEND ? LINEARBLEND : NOBLEND;
  
  if(controlMode == 1){
    currentBlendingHead = blendChoose;
  }else if(controlMode == 2){
    currentBlendingBody = blendChoose;
  } else {
    currentBlendingHead = blendChoose;
    currentBlendingBody = blendChoose;
  }
  Serial.print("BLEND="); Serial.println(blendChoose == NOBLEND ? "NOBLEND" : "LINEARBLEND");
}

void changeSpeed(bool bUp)
{
  byte speedChange = controlMode == 2 ? speedLedBody : speedLedHead;
  // If increase speed.
  if(bUp){
    speedChange = speedChange == 0 ? 0 : speedChange - 1;
  } else {
    speedChange = speedChange == 255 ? 255 : speedChange + 1;
  }

  if(controlMode == 1){
    speedLedHead = speedChange;
  }else if(controlMode == 2){
    speedLedBody = speedChange;
  } else {
    speedLedHead = speedChange;
    speedLedBody = speedChange;
  }

  Serial.print("SPEED="); Serial.println(speedChange);
}

void changeDriftLed(bool bUp)
{
  byte driftChange = controlMode == 2 ? driftLedBody : driftLedHead;
   // If increase drift led.
  if(bUp){
    driftChange = driftChange == 255 ? 255 : driftChange + 1;
  } else {
    driftChange = driftChange == 0 ? 0 : driftChange - 1;
  }

  if(controlMode == 1){
    driftLedHead = driftChange;
  }else if(controlMode == 2){
    driftLedBody = driftChange;
  } else {
    driftLedHead = driftChange;
    driftLedBody = driftChange;
  }
  Serial.print("DRIFT="); Serial.println(driftChange);
}

void changeColor(bool bUp)
{
  byte red = controlMode == 2 ? redBody : redHead;
  byte green = controlMode == 2 ? greenBody : greenHead;
  byte blue = controlMode == 2 ? blueBody : blueHead;

  if(bUp){
    red = choixCouleur == 1 ? red + 1 : red;
    green = choixCouleur == 2 ? green + 1 : green;
    blue = choixCouleur == 3 ? blue + 1 : blue;
  } else {
    red = choixCouleur == 1 ? red - 1 : red;
    green = choixCouleur == 2 ? green - 1 : green;
    blue = choixCouleur == 3 ? blue - 1 : blue;
  }
  Serial.print("R=");Serial.print(red);Serial.print(" G=");Serial.print(green);Serial.print(" B=");Serial.println(blue);

  if(controlMode == 1){
    redHead = red;
    greenHead = green;
    blueHead = blue;
  }else if(controlMode == 2){
    redBody = red;
    greenBody = green;
    blueBody = blue;
  } else {
    redHead = red;
    greenHead = green;
    blueHead = blue;

    redBody = red;
    greenBody = green;
    blueBody = blue;
  }
}
