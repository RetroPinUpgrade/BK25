#include "Arduino.h"
#define SMALLEST_CODESIZE
#include "LEDAccessoryBoard.h"
#include "ALB-Communication.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <EEPROM.h>



/***************************************************************
 *  Set up these definitions based on install
 */
#define STRIP_1_NUMBER_OF_PIXELS    38
#define STRIP_2_NUMBER_OF_PIXELS    14
#define STRIP_3_NUMBER_OF_PIXELS    0
#define STRIP_4_NUMBER_OF_PIXELS    0
#define STRIP_5_NUMBER_OF_PIXELS    0

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel StripOfLEDs1 = Adafruit_NeoPixel(STRIP_1_NUMBER_OF_PIXELS, STRIP_1_CONTROL, NEO_BRG + NEO_KHZ800);
Adafruit_NeoPixel StripOfLEDs2 = Adafruit_NeoPixel(STRIP_2_NUMBER_OF_PIXELS, STRIP_2_CONTROL, NEO_BRG + NEO_KHZ800);

// Connector 1 (pins from top to bottom)
// 1:  A12 (D7)  - 
// 2:  A11 (D3)  - 
// 3:  A10 (D8)  - 
// 4:  A9  (D4)  - 
// 5:  A8  (D9)  - 
// 6:  A7  (D5)  - 
// 7:  KEY
// 8:  A6  (D10) - 
// 9:  A5  (D6)  - 
//
// Connector 2  (pin from top to bottom)
// 1:  A4  (D11)  - 
// 2:  A3  (D14)  - 
// 3:  Key
// 4:  A2  (D12)  - 
// 5:  A1  (D15)  - 
// 6:  A0  (D13)  - 


// The LightingNeedsToWatchI2C() function should return either:
//    false = No, we're not expecting i2c communication
//    true  = Yes, watch i2c port for commands
boolean LightingNeedsToWatchI2C() {
  return true;
}

byte IncomingI2CDeviceAddress() {
  return 8;
}


// The LightingNeedsToCheckMachineFor5V() function should return either:
//    false = No, nothing is hooked to the 5V sensor
//    true  = Yes, check the 5V sensor to see if the machine is on
boolean LightingNeedsToCheckMachineFor5V() {
  return false;
}

// The LightingNeedsToCheckMachineForFlipperActivity() function should return either:
//    false = No, the flipper sensing lines shouldn't dictate the machine state
//    true  = Yes, flipper activity should tell us if the machine is in attract or game play
boolean LightingNeedsToCheckMachineForFlipperActivity() {
  return false;
}


void InitializeRGBValues() {
  // not used in this game
  analogWrite(NON_ADDRESSABLE_RGB_RED_PIN, 0);
  analogWrite(NON_ADDRESSABLE_RGB_GREEN_PIN, 0);
  analogWrite(NON_ADDRESSABLE_RGB_BLUE_PIN, 0);
}


byte BrightnessTopper = 255;
byte BrightnessSpeaker = 255;
byte BrightnessBackglass = 255;
byte BrightnessStadium = 255;
byte BrightnessUnderCab = 255;
byte BrightnessGI[5] = {255, 255, 255, 255, 255};

byte PaletteReds[3] = {255, 200, 150};
byte PaletteGreens[3] = {0, 100, 0};
byte PaletteBlues[3] = {0, 0, 200};

byte GIReds[5] = {250, 150, 255, 255, 255};
byte GIGreens[5] = {0, 27, 255, 255, 255};
byte GIBlues[5] = {0, 0, 105, 255, 255};

boolean GIInitialized = false;

#define EEPROM_CONFIRMATION_UL                0x4C454430
#define EEPROM_INIT_CONFIRMATION_ADDRESS      200
#define EEPROM_BRIGHTNESS_TOPPER              225
#define EEPROM_BRIGHTNESS_SPEAKER             226
#define EEPROM_BRIGHTNESS_BACKGLASS           227
#define EEPROM_BRIGHTNESS_STADIUM             228
#define EEPROM_BRIGHTNESS_UNDERCAB            229
#define EEPROM_BRIGHTNESS_GI0                 230
#define EEPROM_BRIGHTNESS_GI1                 231
#define EEPROM_BRIGHTNESS_GI2                 232
#define EEPROM_BRIGHTNESS_GI3                 233
#define EEPROM_BRIGHTNESS_GI4                 234

boolean ReadSettings() {

  unsigned long testVal =   (((unsigned long)EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS+3))<<24) | 
                            ((unsigned long)(EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS+2))<<16) | 
                            ((unsigned long)(EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS+1))<<8) | 
                            ((unsigned long)(EEPROM.read(EEPROM_INIT_CONFIRMATION_ADDRESS)));
  if (testVal!=EEPROM_CONFIRMATION_UL) {
    // Write the defaults compiled above
    WriteSettings();
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+3, EEPROM_CONFIRMATION_UL>>24);
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+2, 0xFF & (EEPROM_CONFIRMATION_UL>>16));
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+1, 0xFF & (EEPROM_CONFIRMATION_UL>>8));
    EEPROM.write(EEPROM_INIT_CONFIRMATION_ADDRESS+0, 0xFF & (EEPROM_CONFIRMATION_UL));
  }               

  BrightnessTopper = EEPROM.read(EEPROM_BRIGHTNESS_TOPPER);
  BrightnessSpeaker = EEPROM.read(EEPROM_BRIGHTNESS_SPEAKER);
  BrightnessBackglass = EEPROM.read(EEPROM_BRIGHTNESS_BACKGLASS);
  BrightnessStadium = EEPROM.read(EEPROM_BRIGHTNESS_STADIUM);
  BrightnessUnderCab = EEPROM.read(EEPROM_BRIGHTNESS_UNDERCAB);
  BrightnessGI[0] = EEPROM.read(EEPROM_BRIGHTNESS_GI0);
  BrightnessGI[1] = EEPROM.read(EEPROM_BRIGHTNESS_GI1);
  BrightnessGI[2] = EEPROM.read(EEPROM_BRIGHTNESS_GI2);
  BrightnessGI[3] = EEPROM.read(EEPROM_BRIGHTNESS_GI3);
  BrightnessGI[4] = EEPROM.read(EEPROM_BRIGHTNESS_GI4);

  return true;
}

boolean WriteSettings() {

  EEPROM.write(EEPROM_BRIGHTNESS_TOPPER, BrightnessTopper);
  EEPROM.write(EEPROM_BRIGHTNESS_SPEAKER, BrightnessSpeaker);
  EEPROM.write(EEPROM_BRIGHTNESS_BACKGLASS, BrightnessBackglass);
  EEPROM.write(EEPROM_BRIGHTNESS_STADIUM, BrightnessStadium);
  EEPROM.write(EEPROM_BRIGHTNESS_UNDERCAB, BrightnessUnderCab);
  EEPROM.write(EEPROM_BRIGHTNESS_GI0, BrightnessGI[0]);
  EEPROM.write(EEPROM_BRIGHTNESS_GI1, BrightnessGI[1]);
  EEPROM.write(EEPROM_BRIGHTNESS_GI2, BrightnessGI[2]);
  EEPROM.write(EEPROM_BRIGHTNESS_GI3, BrightnessGI[3]);
  EEPROM.write(EEPROM_BRIGHTNESS_GI4, BrightnessGI[4]);

  return true;
}



/***************************************************************
 *  This function shows the general illumination version of the
 *  backlight (with no animation)
 */

#define MACHINE_STATE_TRANSITION_OFF_TO_ATTRACT      250
#define MACHINE_STATE_TRANSITION_ATTRACT_TO_GAME     251
#define MACHINE_STATE_TRANSITION_GAME_TO_ATTRACT     252

byte CurrentLightingState = MACHINE_STATE_OFF;




boolean RGBHasBeenShown = true;

boolean UpdateRGBBasedOnInputs(unsigned long  *lastInputSeenTime, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {

  // This accessory doesn't use RGB lamps or inputs
  (void)lastInputSeenTime;
  (void)currentTime;
  (void)currentAnimationFrame;
  (void)machineState;
  (void)machineStateChangedTime;
  return false;
}

#define SET_COLOR_0                               0
#define SET_COLOR_1                               1
#define SET_COLOR_2                               2
 
#define SET_TOPPER_BRIGHTNESS                     3
#define SET_SPEAKER_BRIGHTNESS                    4
#define SET_BACKGLASS_BRIGHTNESS                  5
#define SET_STADIUM_BRIGHTNESS                    6
#define SET_UNDERCAB_BRIGHTNESS                   7
#define SET_GI_BRIGHTNESS_0                       8
#define SET_GI_BRIGHTNESS_1                       9
#define SET_GI_BRIGHTNESS_2                       10
#define SET_GI_BRIGHTNESS_3                       11
#define SET_GI_BRIGHTNESS_4                       12

#define TOPPER_PULSE_COLOR_0                      15
#define TOPPER_PULSE_COLOR_1                      16
#define TOPPER_PULSE_COLOR_2                      17
#define TOPPER_FLASH_COLOR_0                      18
#define TOPPER_FLASH_COLOR_1                      19
#define TOPPER_FLASH_COLOR_2                      20
#define TOPPER_LOOP_COLOR_0                       21
#define TOPPER_LOOP_COLOR_1                       22
#define TOPPER_LOOP_COLOR_2                       23
#define TOPPER_LEFT_FLASH_COLOR_0                 24
#define TOPPER_LEFT_FLASH_COLOR_1                 25
#define TOPPER_LEFT_FLASH_COLOR_2                 26
#define TOPPER_RIGHT_FLASH_COLOR_0                27
#define TOPPER_RIGHT_FLASH_COLOR_1                28
#define TOPPER_RIGHT_FLASH_COLOR_2                29
#define TOPPER_LEFT_TO_RIGHT_COLOR_0              30
#define TOPPER_LEFT_TO_RIGHT_COLOR_1              31
#define TOPPER_LEFT_TO_RIGHT_COLOR_2              32
#define TOPPER_RIGHT_TO_LEFT_COLOR_0              33
#define TOPPER_RIGHT_TO_LEFT_COLOR_1              34
#define TOPPER_RIGHT_TO_LEFT_COLOR_2              35
#define TOPPER_SPARKLE_COLOR_0                    36
#define TOPPER_SPARKLE_COLOR_1                    37
#define TOPPER_SPARKLE_COLOR_2                    38
#define TOPPER_FIRE                               39
#define TOPPER_CANDY_PULSE                        40
#define TOPPER_LIGHTNING_0                        41
#define TOPPER_LIGHTNING_1                        42
#define TOPPER_LIGHTNING_2                        43
#define TOPPER_LIGHTNING_3                        44
#define TOPPER_LIGHTNING_4                        45
#define TOPPER_BIG_LIGHTNING_0                    46
#define TOPPER_BIG_LIGHTNING_1                    47

#define LEFT_TO_RIGHT_SPEAKER_COLOR_0             50
#define LEFT_TO_RIGHT_SPEAKER_COLOR_1             51
#define LEFT_TO_RIGHT_SPEAKER_COLOR_2             52
#define RIGHT_TO_LEFT_SPEAKER_COLOR_0             53
#define RIGHT_TO_LEFT_SPEAKER_COLOR_1             54
#define RIGHT_TO_LEFT_SPEAKER_COLOR_2             55
#define BOTH_SPEAKERS_TWO_COLOR_0_1_PULSE         56

#define LEFT_SPEAKER_PULSE_COLOR_0                60
#define LEFT_SPEAKER_PULSE_COLOR_1                61
#define LEFT_SPEAKER_PULSE_COLOR_2                62
#define LEFT_SPEAKER_FLASH_COLOR_0                63
#define LEFT_SPEAKER_FLASH_COLOR_1                64
#define LEFT_SPEAKER_FLASH_COLOR_2                65
#define LEFT_SPEAKER_LOOP_CW_COLOR_0              66
#define LEFT_SPEAKER_LOOP_CW_COLOR_1              67
#define LEFT_SPEAKER_LOOP_CW_COLOR_2              68
#define LEFT_SPEAKER_LOOP_CCW_COLOR_0             69
#define LEFT_SPEAKER_LOOP_CCW_COLOR_1             70
#define LEFT_SPEAKER_LOOP_CCW_COLOR_2             71

#define RIGHT_SPEAKER_PULSE_COLOR_0               75
#define RIGHT_SPEAKER_PULSE_COLOR_1               76
#define RIGHT_SPEAKER_PULSE_COLOR_2               77
#define RIGHT_SPEAKER_FLASH_COLOR_0               78
#define RIGHT_SPEAKER_FLASH_COLOR_1               79
#define RIGHT_SPEAKER_FLASH_COLOR_2               80
#define RIGHT_SPEAKER_LOOP_CW_COLOR_0             81
#define RIGHT_SPEAKER_LOOP_CW_COLOR_1             82
#define RIGHT_SPEAKER_LOOP_CW_COLOR_2             83
#define RIGHT_SPEAKER_LOOP_CCW_COLOR_0            84
#define RIGHT_SPEAKER_LOOP_CCW_COLOR_1            85
#define RIGHT_SPEAKER_LOOP_CCW_COLOR_2            86

#define STADIUM_SET_COLOR_0                       90
#define STADIUM_SET_COLOR_1                       91
#define STADIUM_SET_COLOR_2                       92
#define STADIUM_PULSE_COLOR_0                     93
#define STADIUM_PULSE_COLOR_1                     94
#define STADIUM_PULSE_COLOR_2                     95

#define UNDERCAB_SET_COLOR_0                      120

#define BACKGLASS_SET_SCENE_0                     170

#define GI_0_SET_COLOR                            210
#define GI_0_PULSE_COLOR                          211
#define GI_0_FLICKER_COLOR                        212
#define GI_1_SET_COLOR                            218
#define GI_1_PULSE_COLOR                          219
#define GI_1_FLICKER_COLOR                        220
#define GI_2_SET_COLOR                            226
#define GI_2_PULSE_COLOR                          227
#define GI_2_FLICKER_COLOR                        228
#define GI_2_FLICKER_START_PIXEL                  229
#define GI_2_FLICKER_END_PIXEL                    230
#define GI_3_SET_COLOR                            232
#define GI_4_SET_COLOR                            240

#define CONTROL_MESSAGE_UNSET                     255

#define TOP_START_PIXEL             10
#define TOP_NUM_PIXELS_PER_ROW      14
#define TOP_NUM_ROWS                2
#define LEFT_SPEAKER_START          0
#define LEFT_SPEAKER_NUM_LAMPS      5
#define RIGHT_SPEAKER_START         5
#define RIGHT_SPEAKER_NUM_LAMPS     5


void HandleControlParameter(byte controlParameter, byte red=0, byte green=0, byte blue=0, byte duration=0) {
  switch(controlParameter) {
    case SET_COLOR_0:
      PaletteReds[0] = red;
      PaletteGreens[0] = green;
      PaletteBlues[0] = blue;
      break;
    case SET_COLOR_1:
      PaletteReds[1] = red;
      PaletteGreens[1] = green;
      PaletteBlues[1] = blue;
      break;
    case SET_COLOR_2:
      PaletteReds[2] = red;
      PaletteGreens[2] = green;
      PaletteBlues[2] = blue;
      break;
    case SET_TOPPER_BRIGHTNESS:
      BrightnessTopper = red;
      break;
    case SET_SPEAKER_BRIGHTNESS:
      BrightnessSpeaker = red;
      break;
    case SET_BACKGLASS_BRIGHTNESS:
      BrightnessBackglass = red;
      break;
    case SET_GI_BRIGHTNESS_0:
      BrightnessGI[0] = red;
      break;
    case SET_GI_BRIGHTNESS_1:
      BrightnessGI[1] = red;
      break;
    case SET_GI_BRIGHTNESS_2:
      BrightnessGI[2] = red;
      break;
    case SET_GI_BRIGHTNESS_3:
      BrightnessGI[3] = red;
      break;
    case SET_GI_BRIGHTNESS_4:
      BrightnessGI[4] = red;
      break;
    case SET_STADIUM_BRIGHTNESS:
      BrightnessStadium = red;
      break;
    case SET_UNDERCAB_BRIGHTNESS:
      BrightnessUnderCab = red;
      break;
  }

  (void)duration;
}

struct ControlMessage {
  byte Area;
  unsigned long SetTime;
  unsigned long AnimationStartTime;
  byte ID;
  byte Parameter;
  byte Red;
  byte Green;
  byte Blue;
  short Duration;
};

//#define MESSAGE_AREA_PARAMETER        0
#define MESSAGE_AREA_TOPPER           1
#define MESSAGE_AREA_SPEAKER          2
#define MESSAGE_AREA_LEFT_SPEAKER     3
#define MESSAGE_AREA_RIGHT_SPEAKER    4
#define MESSAGE_AREA_STADIUM          5
#define MESSAGE_AREA_UNDERCAB         6
#define MESSAGE_AREA_BACKGLASS        7
#define MESSAGE_AREA_GI_0             8
#define MESSAGE_AREA_GI_1             9
#define MESSAGE_AREA_GI_2             10
#define MESSAGE_AREA_GI_3             11
#define MESSAGE_AREA_GI_4             12
#define TOTAL_NUM_AREAS               12

ControlMessage MessagesByArea[TOTAL_NUM_AREAS];
ControlMessage StoredLoops[TOTAL_NUM_AREAS];

void InitRememberedMessages() {
  for (byte count=0; count<TOTAL_NUM_AREAS; count++) {
    MessagesByArea[count].Area = count+1;
    MessagesByArea[count].SetTime = 0;
    MessagesByArea[count].AnimationStartTime = 0;
    MessagesByArea[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
    MessagesByArea[count].Parameter = CONTROL_MESSAGE_UNSET;
    MessagesByArea[count].Red = 0;
    MessagesByArea[count].Green = 0;
    MessagesByArea[count].Blue = 0;
    MessagesByArea[count].Duration = 0;
    memcpy(&StoredLoops[count], &MessagesByArea[count], sizeof(StoredLoops[count]));
  }
//  Serial.write("Cleared remembered messages\n");
}


void InitializeAllStrips() {
  StripOfLEDs1.begin();
  StripOfLEDs1.show(); // Initialize all pixels to 'off'
  InitRememberedMessages();
}




byte GetMessageArea(byte messageParameter) {
  if (messageParameter>=TOPPER_PULSE_COLOR_0 && messageParameter<LEFT_TO_RIGHT_SPEAKER_COLOR_0) return MESSAGE_AREA_TOPPER;
  if (messageParameter>=LEFT_TO_RIGHT_SPEAKER_COLOR_0 && messageParameter<LEFT_SPEAKER_PULSE_COLOR_0) return MESSAGE_AREA_SPEAKER;
  if (messageParameter>=LEFT_SPEAKER_PULSE_COLOR_0 && messageParameter<RIGHT_SPEAKER_PULSE_COLOR_0) return MESSAGE_AREA_LEFT_SPEAKER;
  if (messageParameter>=RIGHT_SPEAKER_PULSE_COLOR_0 && messageParameter<STADIUM_SET_COLOR_0) return MESSAGE_AREA_RIGHT_SPEAKER;
  if (messageParameter>=STADIUM_SET_COLOR_0 && messageParameter<UNDERCAB_SET_COLOR_0) return MESSAGE_AREA_STADIUM;
  if (messageParameter>=UNDERCAB_SET_COLOR_0 && messageParameter<BACKGLASS_SET_SCENE_0) return MESSAGE_AREA_UNDERCAB;
  if (messageParameter>=BACKGLASS_SET_SCENE_0 && messageParameter<GI_0_SET_COLOR) return MESSAGE_AREA_BACKGLASS;
  if (messageParameter>=GI_0_SET_COLOR && messageParameter<GI_1_SET_COLOR) return MESSAGE_AREA_GI_0;
  if (messageParameter>=GI_1_SET_COLOR && messageParameter<GI_2_SET_COLOR) return MESSAGE_AREA_GI_1;
  if (messageParameter>=GI_2_SET_COLOR && messageParameter<GI_3_SET_COLOR) return MESSAGE_AREA_GI_2;
  if (messageParameter>=GI_3_SET_COLOR && messageParameter<GI_4_SET_COLOR) return MESSAGE_AREA_GI_3;
  if (messageParameter>=GI_4_SET_COLOR && messageParameter<CONTROL_MESSAGE_UNSET) return MESSAGE_AREA_GI_4;

  return 0;
}


boolean PulseColorOnTop(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>30) return false;

  int brightness = 255;
  if (animationFrame<=15) {
    brightness = 17 * animationFrame;
  } else {
    brightness = (17*(30-animationFrame));
  }

  byte startPixel = TOP_START_PIXEL;
  byte endPixel = TOP_START_PIXEL + (TOP_NUM_PIXELS_PER_ROW * TOP_NUM_ROWS);

  for (byte count=startPixel; count<endPixel; count++) {
    int adjRed = (brightness * red) / 255;
    int adjGreen = (brightness * green) / 255;
    int adjBlue = (brightness * blue) / 255;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
  }

  if (animationFrame==30) return false;

  return true;
}


boolean PulseSpeaker(int animationFrame, byte red, byte green, byte blue, boolean left) {
  if (animationFrame>30) return false;

  int brightness = 255;
  if (animationFrame<=15) {
    brightness = 17 * animationFrame;
  } else {
    brightness = (17*(30-animationFrame));
  }

  byte startPixel = LEFT_SPEAKER_START;
  byte endPixel = LEFT_SPEAKER_START + LEFT_SPEAKER_NUM_LAMPS;
  if (!left) {
    startPixel = RIGHT_SPEAKER_START;
    endPixel = RIGHT_SPEAKER_START + RIGHT_SPEAKER_NUM_LAMPS;
  }

  for (byte count=startPixel; count<endPixel; count++) {
    int adjRed = (brightness * red) / 255;
    int adjGreen = (brightness * green) / 255;
    int adjBlue = (brightness * blue) / 255;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
  }

  if (animationFrame==30) return false;

  return true;  
}


boolean BothSpeakersPulse(int animationFrame, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2) {
  if (animationFrame>30) return false;

  int brightness = 255;
  if (animationFrame<=15) {
    brightness = 17 * animationFrame;
  } else {
    brightness = (17*(30-animationFrame));
  }

/*
  if (animationFrame==0) {
    char buf[256];
    sprintf(buf, "RGB = %d,%d,%d RGB2 = %d,%d,%d\n", r1, g1, b1, r2, g2, b2);
    Serial.write(buf);
  }
*/

  byte startPixel = LEFT_SPEAKER_START;

  int adjRed1 = (brightness * r1) / 255;
  int adjGreen1 = (brightness * g1) / 255;
  int adjBlue1 = (brightness * b1) / 255;
  int adjRed2 = (brightness * r2) / 255;
  int adjGreen2 = (brightness * g2) / 255;
  int adjBlue2 = (brightness * b2) / 255;

  StripOfLEDs1.setPixelColor((animationFrame)%LEFT_SPEAKER_NUM_LAMPS + startPixel, StripOfLEDs1.Color( adjRed1, adjGreen1, adjBlue1 ));
  StripOfLEDs1.setPixelColor((animationFrame+LEFT_SPEAKER_NUM_LAMPS/2)%LEFT_SPEAKER_NUM_LAMPS + startPixel, StripOfLEDs1.Color( adjRed2, adjGreen2, adjBlue2 ));

  startPixel = RIGHT_SPEAKER_START;

  StripOfLEDs1.setPixelColor((animationFrame)%RIGHT_SPEAKER_NUM_LAMPS + startPixel, StripOfLEDs1.Color( adjRed1, adjGreen1, adjBlue1 ));
  StripOfLEDs1.setPixelColor((animationFrame+RIGHT_SPEAKER_NUM_LAMPS/2)%RIGHT_SPEAKER_NUM_LAMPS + startPixel, StripOfLEDs1.Color( adjRed2, adjGreen2, adjBlue2 ));

  if (animationFrame==30) return false;
  return true;
}


byte LastPulsePolarity = 0;
boolean Pulse2ColorsOnTop(int animationFrame, byte r1, byte g1, byte b1, byte r2, byte g2, byte b2) {
  if (animationFrame>30) {
    return false;
  }

  int brightness = 255;
  if (animationFrame<=15) {
    brightness = 17 * animationFrame;
  } else {
    brightness = 255 - (17*animationFrame);
  }

  if (LastPulsePolarity) {
    byte tempr = r1;
    byte tempg = g1;
    byte tempb = b1;
    r1 = r2;
    g1 = g2;
    b1 = b2;
    r2 = tempr;
    g2 = tempg;
    b2 = tempb;
  }

  byte startPixel = TOP_START_PIXEL;
  byte endPixel = TOP_START_PIXEL + (TOP_NUM_PIXELS_PER_ROW * TOP_NUM_ROWS);
  byte halfwayPixel = (startPixel + endPixel)/2;

  for (byte count=startPixel; count<endPixel; count++) {
    if (count<(halfwayPixel)) {
      int adjRed = (brightness * r1) / 255;
      int adjGreen = (brightness * g1) / 255;
      int adjBlue = (brightness * b1) / 255;
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
    } else {
      int adjRed = ((255-brightness) * r2) / 255;
      int adjGreen = ((255-brightness) * g2) / 255;
      int adjBlue = ((255-brightness) * b2) / 255;
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
    }
  }

  if (animationFrame==30) {
    LastPulsePolarity ^= 1;
    return false;
  }
  return true;
}

int RandomFlashSeed = 0;
boolean RandomFlashColorOnTop(int animationFrame, byte red, byte green, byte blue, boolean flashSide) {

  if (animationFrame>30) return false;
  
  if (RandomFlashSeed==0) {
    RandomFlashSeed = (millis()%15);
  }

  boolean flash1On = false;
  boolean flash2On = false;
  if (animationFrame==RandomFlashSeed || animationFrame==(RandomFlashSeed + 6) || animationFrame==(RandomFlashSeed * 2)) {
    flash1On = true;
  }  
  if (animationFrame==(RandomFlashSeed+4) || animationFrame==(RandomFlashSeed + 9) || animationFrame==((RandomFlashSeed * 2)/3)) {
    flash2On = true;
  }

  byte startPixel = TOP_START_PIXEL;
  byte endPixel = TOP_START_PIXEL + (TOP_NUM_PIXELS_PER_ROW * TOP_NUM_ROWS);
  byte halfwayPixel = (startPixel + endPixel)/2;

  for (byte count=startPixel; count<endPixel; count++) {
    if (count<(halfwayPixel)) {
      if ((flash1On && flashSide==0) || (flash2On && flashSide==1)) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
    } else {
      if ((flash2On && flashSide==0) || (flash1On && flashSide==1)) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
    }
  }

  if (animationFrame==30) {
    RandomFlashSeed = 0;
    return false;
  }

  return true;
}


boolean RandomFlashColorOnSpeaker(int animationFrame, byte red, byte green, byte blue, boolean left) {

  if (animationFrame>30) return false;
  
  if (RandomFlashSeed==0) {
    RandomFlashSeed = ((millis() + left)%15);
  }

  boolean flash1On = false;
  if (animationFrame==RandomFlashSeed || animationFrame==(RandomFlashSeed + 6) || animationFrame==(RandomFlashSeed * 2)) {
    flash1On = true;
  }

  byte startPixel = LEFT_SPEAKER_START;
  byte endPixel = LEFT_SPEAKER_START + LEFT_SPEAKER_NUM_LAMPS;
  if (!left) {
    startPixel = RIGHT_SPEAKER_START;
    endPixel = RIGHT_SPEAKER_START + RIGHT_SPEAKER_NUM_LAMPS;
  }

  for (byte count=startPixel; count<endPixel; count++) {
    if (flash1On) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
  }

  if (animationFrame==30) {
    RandomFlashSeed = 0;
    return false;
  }

  return true;
}

boolean BigFlashOnTop(int animationFrame, byte red, byte green, byte blue, boolean longerFlash) {

  if (!longerFlash && animationFrame>45) return false;
  if (animationFrame>75) return false;
  
  if (RandomFlashSeed==0) {
    RandomFlashSeed = (millis()%10);
  }

  boolean flashBack = false;
  boolean flashFront = false;
  boolean flashLeft = false;
  boolean flashRight = false;

  if ( (animationFrame%12)==0 ) flashBack = true;
  if ( (animationFrame%17)==0 ) flashFront = true;
  if ( (animationFrame%13)==0 ) flashLeft = true;
  if ( (animationFrame%RandomFlashSeed)==0 ) flashRight = true;

  byte startPixel = TOP_START_PIXEL;
  byte endPixel = TOP_START_PIXEL + (TOP_NUM_PIXELS_PER_ROW * TOP_NUM_ROWS);
  byte halfwayPixel = (startPixel + endPixel)/2;

  if (flashBack) {
    for (byte count=startPixel; count<halfwayPixel; count++) {
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
    }
  } else if (flashFront) {
    for (byte count=halfwayPixel; count<endPixel; count++) {
      StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( red, green, blue ));
    }
  } else if (flashRight) {
    for (byte row=0; row<TOP_NUM_ROWS; row++) {
      for (byte count=0; count<(TOP_NUM_PIXELS_PER_ROW/2); count++) {
        if ((row%2)==0) StripOfLEDs1.setPixelColor(count + TOP_START_PIXEL + row*TOP_NUM_PIXELS_PER_ROW, StripOfLEDs1.Color( red, green, blue ));
        else StripOfLEDs1.setPixelColor(count + TOP_START_PIXEL + row*TOP_NUM_PIXELS_PER_ROW + (TOP_NUM_PIXELS_PER_ROW/2), StripOfLEDs1.Color( red, green, blue ));
      }
    }
  } else if (flashLeft) {
    for (byte row=0; row<TOP_NUM_ROWS; row++) {
      for (byte count=0; count<(TOP_NUM_PIXELS_PER_ROW/2); count++) {
        if ((row%2)==1) StripOfLEDs1.setPixelColor(count + TOP_START_PIXEL + row*TOP_NUM_PIXELS_PER_ROW, StripOfLEDs1.Color( red, green, blue ));
        else StripOfLEDs1.setPixelColor(count + TOP_START_PIXEL + row*TOP_NUM_PIXELS_PER_ROW + (TOP_NUM_PIXELS_PER_ROW/2), StripOfLEDs1.Color( red, green, blue ));
      }
    }
  }


  if (!longerFlash && animationFrame==45) {
    RandomFlashSeed = 0;
    return false;
  } else if (longerFlash && animationFrame==75) {
    RandomFlashSeed = 0;
    return false;    
  }

  return true;
}

boolean LoopColorOnTop(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>=(TOP_NUM_PIXELS_PER_ROW*2)) return false;

  if ( (TOP_NUM_ROWS%2)==0) {
    byte lastRowStartPixel = TOP_START_PIXEL + (TOP_NUM_ROWS-1)*TOP_NUM_PIXELS_PER_ROW;
    StripOfLEDs1.setPixelColor( (animationFrame%TOP_NUM_PIXELS_PER_ROW) + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
    StripOfLEDs1.setPixelColor( lastRowStartPixel + (animationFrame%TOP_NUM_PIXELS_PER_ROW), StripOfLEDs1.Color( red, green, blue ));    
  } else {
    byte endPixel = TOP_START_PIXEL + (TOP_NUM_ROWS)*TOP_NUM_PIXELS_PER_ROW - 1;
    StripOfLEDs1.setPixelColor( (animationFrame%TOP_NUM_PIXELS_PER_ROW) + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
    StripOfLEDs1.setPixelColor( endPixel - (animationFrame%TOP_NUM_PIXELS_PER_ROW), StripOfLEDs1.Color( red, green, blue ));    
  }

/*
  if (animationFrame<TOP_NUM_PIXELS_PER_ROW) {
    for (byte count=0; count<TOP_NUM_PIXELS_PER_ROW; count++) {
      if (animationFrame==count) {
        StripOfLEDs1.setPixelColor(count + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
      }
    }
  } else {
    if ( (TOP_NUM_ROWS%2)==0) {
      byte lastRowStartPixel = TOP_START_PIXEL + (TOP_NUM_ROWS-1)*TOP_NUM_PIXELS_PER_ROW;
      
      for (byte count=0; count<TOP_NUM_PIXELS_PER_ROW; count++) {
        if ((animationFrame - TOP_NUM_PIXELS_PER_ROW)==count) {
          StripOfLEDs1.setPixelColor(lastRowStartPixel + count, StripOfLEDs1.Color( red, green, blue ));
        }
      }
    } else {
      byte endPixel = TOP_START_PIXEL + (TOP_NUM_ROWS)*TOP_NUM_PIXELS_PER_ROW - 1;
      for (byte count=0; count<TOP_NUM_PIXELS_PER_ROW; count++) {
        if ((animationFrame - TOP_NUM_PIXELS_PER_ROW)==count) {
          StripOfLEDs1.setPixelColor(endPixel - count, StripOfLEDs1.Color( red, green, blue ));
        }
      }
    }
  }
*/      
  
  return true;
}


boolean LoopSpeaker(int animationFrame, byte red, byte green, byte blue, boolean left, boolean clockwise, int duration) {

  int maxFrames = 0;
  int totalPixels = 0;
  
  if (left) {
    if (duration==0) maxFrames = (LEFT_SPEAKER_NUM_LAMPS*2); 
    else maxFrames = (duration/LED_UPDATE_FRAME_PERIOD_IN_MS);
    totalPixels = LEFT_SPEAKER_NUM_LAMPS;
  } else {
    if (duration==0) maxFrames = (RIGHT_SPEAKER_NUM_LAMPS*2);
    else maxFrames = (duration/LED_UPDATE_FRAME_PERIOD_IN_MS);
    totalPixels = RIGHT_SPEAKER_NUM_LAMPS;
  }
  if (left && animationFrame>=maxFrames) return false;
  if (!left && animationFrame>=maxFrames) return false;

  byte startPixel = LEFT_SPEAKER_START;
  byte endPixel = LEFT_SPEAKER_START + LEFT_SPEAKER_NUM_LAMPS;

  if (!left) {
    startPixel = RIGHT_SPEAKER_START;
    endPixel = RIGHT_SPEAKER_START + RIGHT_SPEAKER_NUM_LAMPS;
  }

  if (clockwise) {
    StripOfLEDs1.setPixelColor(endPixel - (animationFrame/2+1)%totalPixels, StripOfLEDs1.Color( red, green, blue ));
  } else {
    StripOfLEDs1.setPixelColor(startPixel + (animationFrame/2)%totalPixels, StripOfLEDs1.Color( red, green, blue ));
  }
  
  return true;
}

boolean LeftToRightSweepOnTop(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>=TOP_NUM_PIXELS_PER_ROW) return false;

  for (byte row=0; row<TOP_NUM_ROWS; row++) {
    for (byte count=0; count<TOP_NUM_PIXELS_PER_ROW; count++) {
      boolean showPixel = false;
      if ( (row%2)==0 ) {
        if (((TOP_NUM_PIXELS_PER_ROW - 1) - count)==animationFrame) showPixel = true;
      } else {
        if (count==animationFrame) showPixel = true;
      }
      if (showPixel) StripOfLEDs1.setPixelColor(row*TOP_NUM_PIXELS_PER_ROW + count + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
    }
  }

  return true;
}

boolean RightToLeftSweepOnTop(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>=TOP_NUM_PIXELS_PER_ROW) return false;

  for (byte row=0; row<TOP_NUM_ROWS; row++) {
    for (byte count=0; count<TOP_NUM_PIXELS_PER_ROW; count++) {
      boolean showPixel = false;
      if ( (row%2)==1 ) {
        if (((TOP_NUM_PIXELS_PER_ROW - 1) - count)==animationFrame) showPixel = true;
      } else {
        if (count==animationFrame) showPixel = true;
      }
      if (showPixel) StripOfLEDs1.setPixelColor(row*TOP_NUM_PIXELS_PER_ROW + count + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
    }
  }

  return true;
}

boolean LeftFlashOnTop(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>5) return false;

  if (animationFrame==0 || animationFrame==2) {
    for (byte row=0; row<TOP_NUM_ROWS; row++) {
      for (byte count=0; count<(TOP_NUM_PIXELS_PER_ROW/2); count++) {
        if ((row%2)==1) StripOfLEDs1.setPixelColor(count + row*TOP_NUM_PIXELS_PER_ROW + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
        else StripOfLEDs1.setPixelColor(count + row*TOP_NUM_PIXELS_PER_ROW + (TOP_NUM_PIXELS_PER_ROW/2) + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
      }
    }
  }

  if (animationFrame==5) {
    return false;
  }
  return true;
}

boolean RightFlashOnTop(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>5) return false;

  if (animationFrame==0 || animationFrame==2) {
    for (byte row=0; row<TOP_NUM_ROWS; row++) {
      for (byte count=0; count<(TOP_NUM_PIXELS_PER_ROW/2); count++) {
        if ((row%2)==0) StripOfLEDs1.setPixelColor(count + row*TOP_NUM_PIXELS_PER_ROW + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
        else StripOfLEDs1.setPixelColor(count + row*TOP_NUM_PIXELS_PER_ROW + (TOP_NUM_PIXELS_PER_ROW/2) + TOP_START_PIXEL, StripOfLEDs1.Color( red, green, blue ));
      }
    }
  }

  if (animationFrame==5) {
    return false;
  }
  return true;
}


boolean LeftToRightSpeaker(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>30) return false;
  
  int leftBrightness = 0;
  int rightBrightness = 0;
  if (animationFrame<=15) {
    leftBrightness = 17*animationFrame;
    rightBrightness = 0;    
  } else {
    leftBrightness = 0;
    rightBrightness = 255 - (17*animationFrame);
  }

  byte startPixel = LEFT_SPEAKER_START;
  byte endPixel = LEFT_SPEAKER_START + LEFT_SPEAKER_NUM_LAMPS;

  for (byte count=startPixel; count<endPixel; count++) {
    int adjRed = (leftBrightness * red) / 255;
    int adjGreen = (leftBrightness * green) / 255;
    int adjBlue = (leftBrightness * blue) / 255;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
  }

  startPixel = RIGHT_SPEAKER_START;
  endPixel = RIGHT_SPEAKER_START + RIGHT_SPEAKER_NUM_LAMPS;
  for (byte count=startPixel; count<endPixel; count++) {
    int adjRed = (rightBrightness * red) / 255;
    int adjGreen = (rightBrightness * green) / 255;
    int adjBlue = (rightBrightness * blue) / 255;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
  }

  return true;
}

boolean RightToLeftSpeaker(int animationFrame, byte red, byte green, byte blue) {
  if (animationFrame>30) return false;

  animationFrame = 30-animationFrame;
  int leftBrightness = 0;
  int rightBrightness = 0;
  if (animationFrame<=15) {
    leftBrightness = 17*animationFrame;
    rightBrightness = 0;    
  } else {
    leftBrightness = 0;
    rightBrightness = 255 - (17*animationFrame);
  }

  byte startPixel = LEFT_SPEAKER_START;
  byte endPixel = LEFT_SPEAKER_START + LEFT_SPEAKER_NUM_LAMPS;

  for (byte count=startPixel; count<endPixel; count++) {
    int adjRed = (leftBrightness * red) / 255;
    int adjGreen = (leftBrightness * green) / 255;
    int adjBlue = (leftBrightness * blue) / 255;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
  }

  startPixel = RIGHT_SPEAKER_START;
  endPixel = RIGHT_SPEAKER_START + RIGHT_SPEAKER_NUM_LAMPS;
  for (byte count=startPixel; count<endPixel; count++) {
    int adjRed = (rightBrightness * red) / 255;
    int adjGreen = (rightBrightness * green) / 255;
    int adjBlue = (rightBrightness * blue) / 255;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( adjRed, adjGreen, adjBlue ));
  }

  return true;
}


boolean Sparkle(int animationFrame) {
  if (animationFrame>10) return false;

  byte startPixel = TOP_START_PIXEL;
  byte endPixel = TOP_START_PIXEL + (TOP_NUM_PIXELS_PER_ROW * TOP_NUM_ROWS);

  for (byte count=startPixel; count<endPixel; count++) {
    byte brightness = micros()%255;
    if ((brightness%4)==0) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( brightness, brightness, 0 ));
    else if ((brightness%4)==1) StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( 0, brightness/2, brightness ));
  }

  if (animationFrame==10) {
    return false;
  }
  return true;
}

boolean FireOnTop(int animationFrame, byte red, byte green, byte blue, int redFactor, int greenFactor, int blueFactor) {
  if (animationFrame>60) return false;
  if (redFactor<1) redFactor = 1;
  if (greenFactor<1) greenFactor = 1;
  if (blueFactor<1) blueFactor = 1;

  byte startPixel = TOP_START_PIXEL;
  byte endPixel = TOP_START_PIXEL + (TOP_NUM_PIXELS_PER_ROW * TOP_NUM_ROWS);

  for (byte count=startPixel; count<endPixel; count++) {
    int brightness = micros()%255;
    int newRed = ((brightness/redFactor) * red)/254;
    int newGreen = ((brightness/greenFactor) * green)/254;
    int newBlue = ((brightness/blueFactor) * blue)/254;
    StripOfLEDs1.setPixelColor(count, StripOfLEDs1.Color( newRed, newGreen, newBlue ));
  }

  if (animationFrame==60) {
    return false;
  }
  return true;
}

#define GI_2_RIGHT_RAIL_START         4 
#define GI_2_RIGHT_RAIL_NUM_PIXELS    5
#define GI_2_LEFT_RAIL_START          9 
#define GI_2_LEFT_RAIL_NUM_PIXELS     5

boolean SetGIColor(byte red, byte green, byte blue, byte GIArea) {

  if (GIArea==0) {
    StripOfLEDs2.setPixelColor(0, StripOfLEDs2.Color( red, green, blue ));
    StripOfLEDs2.setPixelColor(1, StripOfLEDs2.Color( red, green, blue ));
  } else if (GIArea==1) {
    StripOfLEDs2.setPixelColor(2, StripOfLEDs2.Color( red, green, blue ));
    StripOfLEDs2.setPixelColor(3, StripOfLEDs2.Color( red, green, blue ));
  } else if (GIArea==2) {
    for (byte count=0; count<GI_2_RIGHT_RAIL_NUM_PIXELS; count++) {
      StripOfLEDs2.setPixelColor(GI_2_RIGHT_RAIL_START+count, StripOfLEDs2.Color( red, green, blue ));
    }
    for (byte count=0; count<GI_2_LEFT_RAIL_NUM_PIXELS; count++) {
      StripOfLEDs2.setPixelColor(GI_2_LEFT_RAIL_START+count, StripOfLEDs2.Color( red, green, blue ));
    }
  }
  
  return true;
}


boolean PulseGIColor(byte animationFrame, byte red, byte green, byte blue, byte GIArea, short duration ) {

  byte maxFrames = 30;
  if (duration) {
    maxFrames = (duration/LED_UPDATE_FRAME_PERIOD_IN_MS);
  }
  if (animationFrame>maxFrames) return false;

  int brightness = 255;
  if (animationFrame<=(maxFrames/2)) {
    brightness = ((int)animationFrame * (int)255 * 2) / maxFrames;
  } else {
    brightness = ((maxFrames - (int)animationFrame) * (int)255 * 2) / maxFrames;
  }

  byte startPixel;
  byte endPixel;

  if (GIArea==0) {
    startPixel = 0;
    endPixel = 1;
  } else if (GIArea==1) {
    startPixel = 2;
    endPixel = 3;
  } else if (GIArea==2) {
    startPixel = GI_2_RIGHT_RAIL_START;
    endPixel = GI_2_RIGHT_RAIL_START + GI_2_RIGHT_RAIL_NUM_PIXELS + GI_2_LEFT_RAIL_NUM_PIXELS;
  }

  int adjRed = (brightness * red) / 255;
  int adjGreen = (brightness * green) / 255;
  int adjBlue = (brightness * blue) / 255;

  for (byte count=startPixel; count<=endPixel; count++) {
    StripOfLEDs2.setPixelColor(count, StripOfLEDs2.Color( adjRed, adjGreen, adjBlue ));
  }

  return true;
}

boolean FlickerGIColor(byte animationFrame, byte red, byte green, byte blue, byte GIArea, short duration) {

  byte maxFrames = 30;
  if (duration) {
    maxFrames = (duration/LED_UPDATE_FRAME_PERIOD_IN_MS);
  }
  if (animationFrame>maxFrames) return false;

  byte startPixel;
  byte endPixel;

  if (GIArea==0) {
    startPixel = 0;
    endPixel = 1;
  } else if (GIArea==1) {
    startPixel = 2;
    endPixel = 3;
  } else if (GIArea==2) {
    startPixel = GI_2_RIGHT_RAIL_START;
    endPixel = GI_2_RIGHT_RAIL_START + GI_2_RIGHT_RAIL_NUM_PIXELS + GI_2_LEFT_RAIL_NUM_PIXELS;
  }

  for (byte count=startPixel; count<=endPixel; count++) {
    if ( ((count+animationFrame)%3)==0 ) StripOfLEDs2.setPixelColor(count, StripOfLEDs2.Color( red, green, blue ));
  }

  return true;
}

boolean FlickerGIColorOnPixel(byte animationFrame, byte red, byte green, byte blue, byte GIArea, short duration, byte pixelNum) {
  byte maxFrames = 30;
  if (duration) {
    maxFrames = (duration/LED_UPDATE_FRAME_PERIOD_IN_MS);
  }
  if (animationFrame>maxFrames) return false;

  if (GIArea==0) {
  } else if (GIArea==1) {
    pixelNum += 2;
  } else if (GIArea==2) {
    pixelNum += GI_2_RIGHT_RAIL_START;
  }

  //for (byte count=0; count<14; count++) StripOfLEDs2.setPixelColor(count, StripOfLEDs2.Color( red, green, blue ));
  if ( (animationFrame%5)>3 ) {
    StripOfLEDs2.setPixelColor(pixelNum, StripOfLEDs2.Color( red, green, blue ));
    StripOfLEDs2.setPixelColor(pixelNum+1, StripOfLEDs2.Color( red, green, blue ));
  }
  else {
    StripOfLEDs2.setPixelColor(pixelNum, StripOfLEDs2.Color( 0, 0, 0 ));
    StripOfLEDs2.setPixelColor(pixelNum+1, StripOfLEDs2.Color( 0, 0, 0 ));
  }

  return true;
}


boolean CalculateResultingColor(byte &red, byte &green, byte &blue, byte brightness, byte preset) {

  if (brightness==0) {
    red = 0;
    green = 0;
    blue = 0;
    return false;
  }
  
  // If no color is given, load a preset
  if (red==0 && green==0 && blue==0 && preset<3) {
    red = PaletteReds[preset];
    green = PaletteGreens[preset];
    blue = PaletteBlues[preset];
  } else if (preset==3) {
    red = 255;
    green = 255;
    blue = 255;
  }

  if (brightness<255) {
    red = (byte)(((int)red * (int)brightness) / (int)255);
    green = (byte)(((int)green * (int)brightness) / (int)255);
    blue = (byte)(((int)blue * (int)brightness) / (int)255);
  }

  if (red==0 && green==0 && blue==0) return false;
  return true;
}


unsigned long LoopStartTime = 0;
boolean PlayTopperAnimation(byte animationNum, unsigned long animationStartTime, unsigned long currentTime, byte red=0, byte green=0, byte blue=0, short duration=0) {

  int animationFrame = (currentTime-animationStartTime)/LED_UPDATE_FRAME_PERIOD_IN_MS;

  switch (animationNum) {
    case TOPPER_PULSE_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 0)) {
        return PulseColorOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_PULSE_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 1)) {
        return PulseColorOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_PULSE_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return PulseColorOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_FLASH_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 0)) {
        return RandomFlashColorOnTop(animationFrame, red, green, blue, animationNum%2);
      }
      break;
    case TOPPER_FLASH_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 1)) {
        return RandomFlashColorOnTop(animationFrame, red, green, blue, animationNum%2);
      }
      break;
    case TOPPER_FLASH_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return RandomFlashColorOnTop(animationFrame, red, green, blue, animationNum%2);
      }
      break;
    case TOPPER_LOOP_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 0)) {
        return LoopColorOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LOOP_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 1)) {
        return LoopColorOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LOOP_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return LoopColorOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LEFT_FLASH_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 0)) {
        return LeftFlashOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LEFT_FLASH_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 1)) {
        return LeftFlashOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LEFT_FLASH_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return LeftFlashOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_RIGHT_FLASH_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 0)) {
        return RightFlashOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_RIGHT_FLASH_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 1)) {
        return RightFlashOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_RIGHT_FLASH_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return RightFlashOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LEFT_TO_RIGHT_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 0)) {
        return LeftToRightSweepOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LEFT_TO_RIGHT_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 1)) {
        return LeftToRightSweepOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_LEFT_TO_RIGHT_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return LeftToRightSweepOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_RIGHT_TO_LEFT_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 0)) {
        return LeftToRightSweepOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_RIGHT_TO_LEFT_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 1)) {
        return RightToLeftSweepOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_RIGHT_TO_LEFT_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return RightToLeftSweepOnTop(animationFrame, red, green, blue);
      }
      break;
    case TOPPER_SPARKLE_COLOR_0:
    case TOPPER_SPARKLE_COLOR_1:
    case TOPPER_SPARKLE_COLOR_2:
      return Sparkle(animationFrame);
      break;
    case TOPPER_FIRE:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return FireOnTop(animationFrame, red, green, blue, 1, 2, 1);
      }      
      break;
    case TOPPER_CANDY_PULSE:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 2)) {
        return Pulse2ColorsOnTop(animationFrame, red, green, blue, 255-red, 255-green, 255-blue);
      }
      break;
    case TOPPER_LIGHTNING_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 3)) {
        return RandomFlashColorOnTop(animationFrame, red, green, blue, animationNum%2);
      }
      break;
    case TOPPER_LIGHTNING_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 3)) {
        return RandomFlashColorOnTop(animationFrame, red/2, green/2, blue/2, animationNum%2);
      }
      break;
    case TOPPER_LIGHTNING_2:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 3)) {
        return RandomFlashColorOnTop(animationFrame, red/3, green/3, blue/3, animationNum%2);
      }
      break;
    case TOPPER_LIGHTNING_3:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 3)) {
        return RandomFlashColorOnTop(animationFrame, red/4, green/4, blue/4, animationNum%2);
      }
      break;
    case TOPPER_LIGHTNING_4:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 3)) {
        return RandomFlashColorOnTop(animationFrame, red/5, green/5, blue/5, animationNum%2);
      }
      break;
    case TOPPER_BIG_LIGHTNING_0:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 3)) {
        return BigFlashOnTop(animationFrame, red, green, blue, false);
      }
      break;
    case TOPPER_BIG_LIGHTNING_1:
      if (CalculateResultingColor(red, green, blue, BrightnessTopper, 3)) {
        return BigFlashOnTop(animationFrame, red, green, blue, true);
      }
      break;
  }

  (void)duration;
  // false means we don't need more time
  return false;
}


boolean PlaySpeakerAnimation(byte animationNum, unsigned long animationStartTime, unsigned long currentTime, byte red=0, byte green=0, byte blue=0, short duration=0) {

  int animationFrame = (currentTime-animationStartTime)/LED_UPDATE_FRAME_PERIOD_IN_MS;

  switch (animationNum) {
    case LEFT_SPEAKER_PULSE_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return PulseSpeaker(animationFrame, red, green, blue, true);
      }
      break;
    case LEFT_SPEAKER_PULSE_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return PulseSpeaker(animationFrame, red, green, blue, true);
      }
      break;
    case LEFT_SPEAKER_PULSE_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return PulseSpeaker(animationFrame, red, green, blue, true);
      }
      break;
    case LEFT_SPEAKER_FLASH_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return RandomFlashColorOnSpeaker(animationFrame, red, green, blue, true);
      }
      break;
    case LEFT_SPEAKER_FLASH_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return RandomFlashColorOnSpeaker(animationFrame, red, green, blue, true);
      }
      break;
    case LEFT_SPEAKER_FLASH_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return RandomFlashColorOnSpeaker(animationFrame, red, green, blue, true);
      }
      break;
    case LEFT_SPEAKER_LOOP_CW_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return LoopSpeaker(animationFrame, red, green, blue, true, true, duration);
      }
      break;
    case LEFT_SPEAKER_LOOP_CW_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return LoopSpeaker(animationFrame, red, green, blue, true, true, duration);
      }
      break;
    case LEFT_SPEAKER_LOOP_CW_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return LoopSpeaker(animationFrame, red, green, blue, true, true, duration);
      }
      break;
    case LEFT_SPEAKER_LOOP_CCW_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return LoopSpeaker(animationFrame, red, green, blue, true, false, duration);
      }
      break;
    case LEFT_SPEAKER_LOOP_CCW_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return LoopSpeaker(animationFrame, red, green, blue, true, false, duration);
      }
      break;
    case LEFT_SPEAKER_LOOP_CCW_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return LoopSpeaker(animationFrame, red, green, blue, true, false, duration);
      }
      break;

    case RIGHT_SPEAKER_PULSE_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return PulseSpeaker(animationFrame, red, green, blue, false);
      }
      break;
    case RIGHT_SPEAKER_PULSE_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return PulseSpeaker(animationFrame, red, green, blue, false);
      }
      break;
    case RIGHT_SPEAKER_PULSE_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return PulseSpeaker(animationFrame, red, green, blue, false);
      }
      break;
    case RIGHT_SPEAKER_FLASH_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return RandomFlashColorOnSpeaker(animationFrame, red, green, blue, false);
      }
      break;
    case RIGHT_SPEAKER_FLASH_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return RandomFlashColorOnSpeaker(animationFrame, red, green, blue, false);
      }
      break;
    case RIGHT_SPEAKER_FLASH_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return RandomFlashColorOnSpeaker(animationFrame, red, green, blue, false);
      }
      break;
    case RIGHT_SPEAKER_LOOP_CW_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return LoopSpeaker(animationFrame, red, green, blue, false, true, duration);
      }
      break;
    case RIGHT_SPEAKER_LOOP_CW_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return LoopSpeaker(animationFrame, red, green, blue, false, true, duration);
      }
      break;
    case RIGHT_SPEAKER_LOOP_CW_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return LoopSpeaker(animationFrame, red, green, blue, false, true, duration);
      }
      break;
    case RIGHT_SPEAKER_LOOP_CCW_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return LoopSpeaker(animationFrame, red, green, blue, false, false, duration);
      }
      break;
    case RIGHT_SPEAKER_LOOP_CCW_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return LoopSpeaker(animationFrame, red, green, blue, false, false, duration);
      }
      break;
    case RIGHT_SPEAKER_LOOP_CCW_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return LoopSpeaker(animationFrame, red, green, blue, false, false, duration);
      }
      break;
    case LEFT_TO_RIGHT_SPEAKER_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return LeftToRightSpeaker(animationFrame, red, green, blue);
      }
      break;
    case LEFT_TO_RIGHT_SPEAKER_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return LeftToRightSpeaker(animationFrame, red, green, blue);
      }
      break;
    case LEFT_TO_RIGHT_SPEAKER_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return LeftToRightSpeaker(animationFrame, red, green, blue);
      }
      break;
    case RIGHT_TO_LEFT_SPEAKER_COLOR_0:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0)) {
        return LeftToRightSpeaker(animationFrame, red, green, blue);
      }
      break;
    case RIGHT_TO_LEFT_SPEAKER_COLOR_1:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 1)) {
        return RightToLeftSpeaker(animationFrame, red, green, blue);
      }
      break;
    case RIGHT_TO_LEFT_SPEAKER_COLOR_2:
      if (CalculateResultingColor(red, green, blue, BrightnessSpeaker, 2)) {
        return RightToLeftSpeaker(animationFrame, red, green, blue);
      }
      break;
    case BOTH_SPEAKERS_TWO_COLOR_0_1_PULSE:
      byte r2=0, g2=0, b2=0;
      CalculateResultingColor(red, green, blue, BrightnessSpeaker, 0);
      CalculateResultingColor(r2, g2, b2, BrightnessSpeaker, 1);
      return BothSpeakersPulse(animationFrame, red, green, blue, r2, g2, b2);
      break;
  }
  
  (void)duration;
  // false means we don't need more time
  return false;
}


boolean PlayGIAnimation(byte animationNum, unsigned long animationStartTime, unsigned long currentTime, byte red=0, byte green=0, byte blue=0, short duration=0) {

  int animationFrame = (currentTime-animationStartTime)/LED_UPDATE_FRAME_PERIOD_IN_MS;

  switch (animationNum) {
    case GI_0_SET_COLOR:
      GIReds[0] = red;
      GIGreens[0] = green;
      GIBlues[0] = blue;
      if (CalculateResultingColor(red, green, blue, BrightnessGI[0], 0)) {
        return SetGIColor(red, green, blue, 0);
      }
      break;
    case GI_0_PULSE_COLOR:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[0], 0)) {
        return PulseGIColor(animationFrame, red, green, blue, 0, duration);
      }
      break;
    case GI_0_FLICKER_COLOR:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[0], 0)) {
        return FlickerGIColor(animationFrame, red, green, blue, 0, duration);
      }
      break;
    case GI_1_SET_COLOR:
      GIReds[1] = red;
      GIGreens[1] = green;
      GIBlues[1] = blue;
      if (CalculateResultingColor(red, green, blue, BrightnessGI[1], 0)) {
        return SetGIColor(red, green, blue, 1);
      }
      break;
    case GI_1_PULSE_COLOR:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[1], 0)) {
        return PulseGIColor(animationFrame, red, green, blue, 1, duration);
      }
      break;
    case GI_1_FLICKER_COLOR:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[1], 0)) {
        return FlickerGIColor(animationFrame, red, green, blue, 1, duration);
      }
      break;
    case GI_2_SET_COLOR:
      GIReds[2] = red;
      GIGreens[2] = green;
      GIBlues[2] = blue;
      if (CalculateResultingColor(red, green, blue, BrightnessGI[2], 0)) {
        return SetGIColor(red, green, blue, 2);
      }
      break;
    case GI_2_PULSE_COLOR:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[2], 0)) {
        return PulseGIColor(animationFrame, red, green, blue, 2, duration);
      }
      break;
    case GI_2_FLICKER_COLOR:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[2], 0)) {
        return FlickerGIColor(animationFrame, red, green, blue, 2, duration);
      }
      break;
    case GI_2_FLICKER_START_PIXEL:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[2], 0)) {
        byte otherRed = GIReds[2];
        byte otherGreen = GIGreens[2];
        byte otherBlue = GIBlues[2];
        if (CalculateResultingColor(otherRed, otherGreen, otherBlue, BrightnessGI[2], 0)) {
          SetGIColor(otherRed, otherGreen, otherBlue, 2);
        }
        return FlickerGIColorOnPixel(animationFrame, red, green, blue, 2, duration, 0);
      }
      break;
    case GI_2_FLICKER_END_PIXEL:
      if (CalculateResultingColor(red, green, blue, BrightnessGI[2], 0)) {
        byte otherRed = GIReds[2];
        byte otherGreen = GIGreens[2];
        byte otherBlue = GIBlues[2];
        if (CalculateResultingColor(otherRed, otherGreen, otherBlue, BrightnessGI[2], 0)) {
          SetGIColor(otherRed, otherGreen, otherBlue, 2);
        }
        return FlickerGIColorOnPixel(animationFrame, red, green, blue, 2, duration, (GI_2_RIGHT_RAIL_NUM_PIXELS + GI_2_LEFT_RAIL_NUM_PIXELS - 2));
      }
      break;
  }
  
  (void)duration;
  // false means we don't need more time
  return false;
}


boolean PlayStadiumAnimation(byte animationNum, unsigned long animationStartTime, unsigned long currentTime, byte red=0, byte green=0, byte blue=0, short duration=0) {

  int animationFrame = (currentTime-animationStartTime)/LED_UPDATE_FRAME_PERIOD_IN_MS;

  (void)animationFrame;
  (void)animationNum;
  (void)animationStartTime;
  (void)currentTime;
  (void)red;
  (void)green;
  (void)blue;
  (void)duration;
  // false means we don't need more time
  return false;
}

boolean PlayUndercabAnimation(byte animationNum, unsigned long animationStartTime, unsigned long currentTime, byte red=0, byte green=0, byte blue=0, short duration=0) {

  int animationFrame = (currentTime-animationStartTime)/LED_UPDATE_FRAME_PERIOD_IN_MS;

  (void)animationFrame;
  (void)animationNum;
  (void)animationStartTime;
  (void)currentTime;
  (void)red;
  (void)green;
  (void)blue;
  (void)duration;
  // false means we don't need more time
  return false;
}

boolean PlayBackglassAnimation(byte animationNum, unsigned long animationStartTime, unsigned long currentTime, byte red=0, byte green=0, byte blue=0, short duration=0) {

  int animationFrame = (currentTime-animationStartTime)/LED_UPDATE_FRAME_PERIOD_IN_MS;

  (void)animationFrame;
  (void)animationNum;
  (void)animationStartTime;
  (void)currentTime;
  (void)red;
  (void)green;
  (void)blue;
  (void)duration;
  // false means we don't need more time
  return false;
}


boolean StripHasBeenShown = true;

boolean UpdateStripsBasedOnInputs(unsigned long  *lastInputSeenTime, unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {
  // This accessory doesn't use inputs
  (void)lastInputSeenTime;
  (void)currentTime;
  (void)currentAnimationFrame;
  (void)machineState;
  (void)machineStateChangedTime;
  return false;
}


boolean UpdateRGBBasedOnI2C(    unsigned long lastMessageSeenTime, byte lastMessage, byte lastParameter, 
                                byte lastRed, byte lastGreen, byte lastBlue, short lastDuration, 
                                unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {

  // This accessory doesn't use RGB lamps
  (void)lastMessageSeenTime;
  (void)lastMessage;
  (void)lastParameter;
  (void)lastRed;
  (void)lastGreen;
  (void)lastBlue;
  (void)lastDuration;
  (void)currentTime;
  (void)currentAnimationFrame;
  (void)machineState;
  (void)machineStateChangedTime;
  return false;
}

unsigned long LastParameterHandledTime = 0;
unsigned long LastAnimationFrameHandled = 0;

byte GetSettingValue( byte settingNum ) {
  switch (settingNum) {
    case SET_TOPPER_BRIGHTNESS:
      return BrightnessTopper;
      break;
    case SET_SPEAKER_BRIGHTNESS:
      return BrightnessSpeaker;
      break;
    case SET_BACKGLASS_BRIGHTNESS:
      return BrightnessBackglass;
      break;
    case SET_STADIUM_BRIGHTNESS:
      return BrightnessStadium;
      break;
    case SET_UNDERCAB_BRIGHTNESS:
      return BrightnessUnderCab;
      break;
    case SET_GI_BRIGHTNESS_0:
      return BrightnessGI[0];
      break;
    case SET_GI_BRIGHTNESS_1:
      return BrightnessGI[1];
      break;
    case SET_GI_BRIGHTNESS_2:
      return BrightnessGI[2];
      break;
    case SET_GI_BRIGHTNESS_3:
      return BrightnessGI[3];
      break;
    case SET_GI_BRIGHTNESS_4:
      return BrightnessGI[4];
      break;
  }

  return 0;
}


boolean HandleI2CMessage(   unsigned long lastMessageSeenTime, byte lastMessage, byte lastParameter,
                            byte lastRed, byte lastGreen, byte lastBlue, short lastDuration ) {

  if (lastMessage==ALB_COMMAND_STOP_ALL_ANIMATIONS || lastMessage==ALB_COMMAND_ALL_LAMPS_OFF) {
    InitRememberedMessages();
    if (StripHasBeenShown) {
      StripOfLEDs1.clear();
      StripOfLEDs1.show();
      StripHasBeenShown = false;
//      Serial.write("Clearing strip for ALB_COMMAND_STOP_ALL_ANIMATIONS or ALB_COMMAND_ALL_LAMPS_OFF.\n");
    }
    return true;
  }

  if (lastMessage==ALB_COMMAND_ADJUST_SETTING) {
    // The last message was a control parameter, so we have to update
    // our parameters if we haven't already
    if (lastMessageSeenTime>LastParameterHandledTime) {
      LastParameterHandledTime = lastMessageSeenTime;
      HandleControlParameter(lastParameter, lastRed, lastGreen, lastBlue, lastDuration);
    }
    return true;
  }

  if (lastMessage==ALB_COMMAND_STOP_ANIMATION_BY_AREA) {
    for (byte count=0; count<TOTAL_NUM_AREAS; count++) {
      // if we find this message, stop it
      if (MessagesByArea[count].Area==lastParameter) {
        MessagesByArea[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
        MessagesByArea[count].Parameter = CONTROL_MESSAGE_UNSET;        
      }
      if (StoredLoops[count].Area==lastParameter) {
        StoredLoops[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
        StoredLoops[count].Parameter = CONTROL_MESSAGE_UNSET;        
      }      
    }
    return true;
  }
  
  byte messageArea = GetMessageArea(lastParameter);
  for (byte count=0; count<TOTAL_NUM_AREAS; count++) {
    if (  lastMessage==ALB_COMMAND_PLAY_ANIMATION || lastMessage==ALB_COMMAND_LOOP_ANIMATION ||
          lastMessage==ALB_COMMAND_PLAY_WITH_COLOR || lastMessage==ALB_COMMAND_LOOP_WITH_COLOR ||
          lastMessage==ALB_COMMAND_PLAY_WITH_COLOR_AND_DURATION || lastMessage==ALB_COMMAND_LOOP_WITH_COLOR_AND_DURATION ) {
      
      if (MessagesByArea[count].Area==messageArea) {
        if (lastMessageSeenTime > MessagesByArea[count].SetTime) {
          if (  MessagesByArea[count].ID==ALB_COMMAND_LOOP_ANIMATION ||
                MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR ||
                MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR_AND_DURATION    ) {
            // if we already have an animation looping in this area, we need to store it for later
            memcpy(&StoredLoops[count], &MessagesByArea[count], sizeof(StoredLoops[count]));
          }
          MessagesByArea[count].SetTime = lastMessageSeenTime;
          MessagesByArea[count].AnimationStartTime = 0;
          MessagesByArea[count].ID = lastMessage;
          MessagesByArea[count].Parameter = lastParameter; 
          MessagesByArea[count].Red = lastRed; 
          MessagesByArea[count].Green = lastGreen; 
          MessagesByArea[count].Blue = lastBlue; 
          MessagesByArea[count].Duration = lastDuration;  
//          char buf[256];
//          sprintf(buf, "Accepting message %d for area %d\n", lastParameter, messageArea);
//          Serial.write(buf);         
        }
      }
    } else if (lastMessage==ALB_COMMAND_STOP_ANIMATION) {
      // if we find this message, stop it
      if (MessagesByArea[count].Parameter==lastParameter) {
        MessagesByArea[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
        MessagesByArea[count].Parameter = CONTROL_MESSAGE_UNSET;        
      }
      if (StoredLoops[count].Parameter==lastParameter) {
        StoredLoops[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
        StoredLoops[count].Parameter = CONTROL_MESSAGE_UNSET;        
      }
    }
  }

  return true;
}


void SetGIDefaults(int GIArea = 0xFF) {
  byte red, green, blue;  

  for (byte count=0; count<5; count++) {
    if (GIArea==0xFF || GIArea==count) {
      red = GIReds[count];
      green = GIGreens[count];
      blue = GIBlues[count];
      CalculateResultingColor(red, green, blue, BrightnessGI[count], 0);
      SetGIColor(red, green, blue, count);
    }
  }
}


boolean UpdateStripsBasedOnI2C(   unsigned long lastMessageSeenTime, byte lastMessage, byte lastParameter, 
                                  byte lastRed, byte lastGreen, byte lastBlue, short lastDuration, 
                                  unsigned long currentTime, unsigned long currentAnimationFrame, byte machineState, unsigned long machineStateChangedTime) {

  (void)currentAnimationFrame;

  if (machineState==MACHINE_STATE_OFF) {
    if (CurrentLightingState!=MACHINE_STATE_OFF) InitRememberedMessages();
    CurrentLightingState = MACHINE_STATE_OFF;
    
    if (StripHasBeenShown) {
      StripOfLEDs1.clear();
      StripOfLEDs1.show();
      StripHasBeenShown = false;
    }
    if (GIInitialized) {
      StripOfLEDs2.clear();
      StripOfLEDs2.show();
    }
    GIInitialized = false;
    return false;
  }
  CurrentLightingState = machineState;

  if (!GIInitialized) {
    GIInitialized = true;
    StripOfLEDs2.clear();
    SetGIDefaults();
    StripOfLEDs2.show();
  }

  if (currentAnimationFrame!=LastAnimationFrameHandled) {
    LastAnimationFrameHandled = currentAnimationFrame;
    StripOfLEDs1.clear();
    StripOfLEDs2.clear();
    boolean strip1Rendered = false;
    boolean strip2Rendered = false;
    boolean strip3Rendered = false;
    boolean strip4Rendered = false;
    boolean strip5Rendered = false;
   
    // Advance animations for all 5 areas
    for (byte count=0; count<TOTAL_NUM_AREAS; count++) {
      boolean animationRendered = false;
  
      if (    MessagesByArea[count].ID==ALB_COMMAND_PLAY_ANIMATION || MessagesByArea[count].ID==ALB_COMMAND_LOOP_ANIMATION ||
              MessagesByArea[count].ID==ALB_COMMAND_PLAY_WITH_COLOR || MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR ||
              MessagesByArea[count].ID==ALB_COMMAND_PLAY_WITH_COLOR_AND_DURATION || MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR_AND_DURATION      ) {
  
        if (MessagesByArea[count].AnimationStartTime==0) MessagesByArea[count].AnimationStartTime = currentTime;
        switch (MessagesByArea[count].Area) {
          case MESSAGE_AREA_TOPPER:
            animationRendered = PlayTopperAnimation(  MessagesByArea[count].Parameter, MessagesByArea[count].AnimationStartTime, currentTime,
                                                      MessagesByArea[count].Red, MessagesByArea[count].Green, MessagesByArea[count].Blue,
                                                      MessagesByArea[count].Duration );
            strip1Rendered |= animationRendered;                                                    
            break;
          case MESSAGE_AREA_SPEAKER:
          case MESSAGE_AREA_LEFT_SPEAKER:
          case MESSAGE_AREA_RIGHT_SPEAKER:
            animationRendered = PlaySpeakerAnimation( MessagesByArea[count].Parameter, MessagesByArea[count].AnimationStartTime, currentTime,
                                                      MessagesByArea[count].Red, MessagesByArea[count].Green, MessagesByArea[count].Blue,
                                                      MessagesByArea[count].Duration );
            strip1Rendered |= animationRendered;                                                  
            break;
          case MESSAGE_AREA_STADIUM:
            animationRendered = PlayStadiumAnimation( MessagesByArea[count].Parameter, MessagesByArea[count].AnimationStartTime, currentTime,
                                                      MessagesByArea[count].Red, MessagesByArea[count].Green, MessagesByArea[count].Blue,
                                                      MessagesByArea[count].Duration );
            strip5Rendered |= animationRendered;                                                    
            break;
          case MESSAGE_AREA_UNDERCAB:
            animationRendered = PlayUndercabAnimation(  MessagesByArea[count].Parameter, MessagesByArea[count].AnimationStartTime, currentTime,
                                                        MessagesByArea[count].Red, MessagesByArea[count].Green, MessagesByArea[count].Blue,
                                                        MessagesByArea[count].Duration );
            strip3Rendered |= animationRendered;                                                    
            break;
          case MESSAGE_AREA_BACKGLASS:
            animationRendered = PlayBackglassAnimation( MessagesByArea[count].Parameter, MessagesByArea[count].AnimationStartTime, currentTime,
                                                        MessagesByArea[count].Red, MessagesByArea[count].Green, MessagesByArea[count].Blue,
                                                        MessagesByArea[count].Duration );
            strip4Rendered |= animationRendered;                                                    
            break;
          case MESSAGE_AREA_GI_0:
          case MESSAGE_AREA_GI_1:
          case MESSAGE_AREA_GI_2:
          case MESSAGE_AREA_GI_3:
          case MESSAGE_AREA_GI_4:
            animationRendered = PlayGIAnimation(  MessagesByArea[count].Parameter, MessagesByArea[count].AnimationStartTime, currentTime,
                                                  MessagesByArea[count].Red, MessagesByArea[count].Green, MessagesByArea[count].Blue,
                                                  MessagesByArea[count].Duration );
            strip2Rendered |= animationRendered;                                                    
            break;
        }

        if (MessagesByArea[count].Area<MESSAGE_AREA_GI_0) {
          if (  ( MessagesByArea[count].ID==ALB_COMMAND_LOOP_ANIMATION || 
                  MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR || 
                  MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR_AND_DURATION ) && !animationRendered) {
            MessagesByArea[count].AnimationStartTime = 0;
            animationRendered = true;
          }
          if (!animationRendered) {
            if (StoredLoops[count].Parameter!=CONTROL_MESSAGE_UNSET) {
              // if the ALB_COMMAND_PLAY_ANIMATION is done and there's a waiting loop,
              // we need to revive it
              unsigned long mostRecentSetTime = MessagesByArea[count].SetTime;
              memcpy(&MessagesByArea[count], &StoredLoops[count], sizeof(MessagesByArea[count]));
              MessagesByArea[count].SetTime = mostRecentSetTime;
              StoredLoops[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
              StoredLoops[count].Parameter = CONTROL_MESSAGE_UNSET;
            } else {
              // If the animation is done and there's not a loop to restore
              // then blank this out.                
              MessagesByArea[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
              MessagesByArea[count].Parameter = CONTROL_MESSAGE_UNSET;
            }
          }
        } else {
          if (!animationRendered) {
            if (  ( MessagesByArea[count].ID==ALB_COMMAND_LOOP_ANIMATION || 
                    MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR || 
                    MessagesByArea[count].ID==ALB_COMMAND_LOOP_WITH_COLOR_AND_DURATION ) && !animationRendered) {
              MessagesByArea[count].AnimationStartTime = 0;
              animationRendered = true;
            } else {
              SetGIDefaults(MessagesByArea[count].Area - MESSAGE_AREA_GI_0);
              MessagesByArea[count].ID = ALB_COMMAND_ALL_LAMPS_OFF;
              MessagesByArea[count].Parameter = CONTROL_MESSAGE_UNSET;
            }
          }
        }
      } else if (MessagesByArea[count].Area >= MESSAGE_AREA_GI_0) {
        SetGIDefaults(MessagesByArea[count].Area - MESSAGE_AREA_GI_0);
      }
    }
    
    StripOfLEDs1.show();
    StripOfLEDs2.show();
    if (strip1Rendered) StripHasBeenShown = true;    
  }
  
  //(void)machineState;
  (void)machineStateChangedTime;
  (void)lastMessageSeenTime;
  (void)lastMessage;
  (void)lastParameter;
  (void)lastRed;
  (void)lastGreen;
  (void)lastBlue;
  (void)lastDuration;
  return true;
}


byte AdvanceSettingsMode(byte oldSettingsMode) {
  LoopStartTime = 0;
  oldSettingsMode += 1;
  if (oldSettingsMode>5) {
    oldSettingsMode = SETTINGS_MODE_OFF;
    StripOfLEDs1.clear();
    StripOfLEDs2.clear();
    StripOfLEDs1.show();
    StripOfLEDs2.show();
  }
  return oldSettingsMode;
}

unsigned long LastTimeAnimationTriggered = 0;
boolean ShowSettingsMode(byte settingsMode, unsigned long currentAnimationFrame) {
  if (settingsMode==SETTINGS_MODE_OFF) return false;

  (void)currentAnimationFrame;
  if (LoopStartTime==0) LoopStartTime = millis();

  StripOfLEDs1.clear();
  StripOfLEDs2.clear();

  boolean animationStillRunning = false;

  switch (settingsMode) {
    case 1:
      animationStillRunning = PlayTopperAnimation(TOPPER_PULSE_COLOR_0, LoopStartTime, millis(), BrightnessTopper, BrightnessTopper, BrightnessTopper);
      break;
    case 2:
      animationStillRunning = PlaySpeakerAnimation(LEFT_SPEAKER_PULSE_COLOR_0, LoopStartTime, millis(), BrightnessSpeaker, BrightnessSpeaker, BrightnessSpeaker);
      animationStillRunning |= PlaySpeakerAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, LoopStartTime, millis(), BrightnessSpeaker, BrightnessSpeaker, BrightnessSpeaker);
      break;
    case 3:
      PlayGIAnimation(GI_0_SET_COLOR, LoopStartTime, millis(), GIReds[0], GIGreens[0], GIBlues[0]);
      break;
    case 4:
      PlayGIAnimation(GI_1_SET_COLOR, LoopStartTime, millis(), GIReds[1], GIGreens[1], GIBlues[1]);
      break;
    case 5:
      PlayGIAnimation(GI_2_SET_COLOR, LoopStartTime, millis(), GIReds[2], GIGreens[2], GIBlues[2]);
      break;
  }

  if (!animationStillRunning) {
    LoopStartTime = 0;
  }
  StripOfLEDs1.show();
  StripOfLEDs2.show();

  return true;
}



boolean IncreaseBrightness(byte settingsMode) {

  if (settingsMode==1) {
    if (BrightnessTopper<230) BrightnessTopper += 25;
    else BrightnessTopper = 255;    
  } else if (settingsMode==2) {
    if (BrightnessSpeaker<230) BrightnessSpeaker += 25;
    else BrightnessSpeaker = 255;
  } else if (settingsMode==3) {
    if (BrightnessGI[0]<230) BrightnessGI[0] += 25;
    else BrightnessGI[0] = 255;
  } else if (settingsMode==4) {
    if (BrightnessGI[1]<230) BrightnessGI[1] += 25;
    else BrightnessGI[1] = 255;
  } else if (settingsMode==5) {
    if (BrightnessGI[2]<230) BrightnessGI[2] += 25;
    else BrightnessGI[2] = 255;
  }

  /*
  else if (settingsMode==5) {
    if (BrightnessGI[2]<230) BrightnessGI[2] += 25;
    else BrightnessGI[2] = 255;
  } else if (settingsMode==6) {
    if (BrightnessGI[3]<230) BrightnessGI[3] += 25;
    else BrightnessGI[3] = 255;
  } else if (settingsMode==7) {
    if (BrightnessGI[4]<230) BrightnessGI[4] += 25;
    else BrightnessGI[4] = 255;
  } else if (settingsMode==8) {
    if (BrightnessBackglass<230) BrightnessBackglass += 25;
    else BrightnessBackglass = 255;
  } else if (settingsMode==9) {
    if (BrightnessStadium<230) BrightnessStadium += 25;
    else BrightnessStadium = 255;
  } else if (settingsMode==10) {
    if (BrightnessUnderCab<230) BrightnessUnderCab += 25;
    else BrightnessUnderCab = 255;
  }
  */

  return true;
}

boolean DecreaseBrightness(byte settingsMode) {

  if (settingsMode==1) {
    if (BrightnessTopper>25) BrightnessTopper -= 25;
    else BrightnessTopper = 0;    
  } else if (settingsMode==2) {
    if (BrightnessSpeaker>25) BrightnessSpeaker -= 25;
    else BrightnessSpeaker = 0;
  } else if (settingsMode==3) {
    if (BrightnessGI[0]>25) BrightnessGI[0] -= 25;
    else BrightnessGI[0] = 0;
  } else if (settingsMode==4) {
    if (BrightnessGI[1]>25) BrightnessGI[1] -= 25;
    else BrightnessGI[1] = 0;
  } else if (settingsMode==5) {
    if (BrightnessGI[2]>25) BrightnessGI[2] -= 25;
    else BrightnessGI[2] = 0;
  }
  
  /*
  else if (settingsMode==8) {
    if (BrightnessGI[2]>25) BrightnessGI[2] -= 25;
    else BrightnessGI[2] = 0;
  } else if (settingsMode==9) {
    if (BrightnessGI[3]>25) BrightnessGI[3] -= 25;
    else BrightnessGI[3] = 0;
  } else if (settingsMode==10) {
    if (BrightnessGI[4]>25) BrightnessGI[4] -= 25;
    else BrightnessGI[4] = 0;
  } else if (settingsMode==3) {
    if (BrightnessBackglass>25) BrightnessBackglass -= 25;
    else BrightnessBackglass = 0;
  } else if (settingsMode==4) {
    if (BrightnessStadium>25) BrightnessStadium -= 25;
    else BrightnessStadium = 0;
  } else if (settingsMode==5) {
    if (BrightnessUnderCab>25) BrightnessUnderCab -= 25;
    else BrightnessUnderCab = 0;
  }
  */

  return true;
}
