#define LED_BACKLIGHT_INO
#include "Arduino.h"
#include "ALB-Communication.h"
#include "LEDAccessoryBoard.h"

byte MachineState = MACHINE_STATE_OFF;
unsigned long MachineStateChangedTime = 0;
unsigned long CurrentTime = 0;

boolean LastButtonState[3] = {false, false, false};
unsigned long LastButtonPressTime[3] = {0, 0, 0};
unsigned long LastFlipper[2] = {0, 0};

int SettingsMode = SETTINGS_MODE_OFF;

byte LastALBMessage = 0;
byte LastALBMessageParameter = 0;
byte LastALBMessageRed = 0;
byte LastALBMessageGreen = 0;
byte LastALBMessageBlue = 0;
short LastALBMessageDuration = 0;
boolean UseWatchdogTimer = false;
unsigned long LastALBMessageSeenTime = 0;
unsigned long WatchDogTimerStartTime = 0;

AccessoryLampBoard ALBCommunicationHelper;

void setup() {

  // control buttons 
  pinMode(14, INPUT_PULLUP); // switch 0
  pinMode(15, INPUT_PULLUP); // switch 1
  pinMode(16, INPUT_PULLUP); // switch 2

  // WS2812 LED control lines
  pinMode(STRIP_1_CONTROL, OUTPUT); // Strip 1
  pinMode(STRIP_2_CONTROL, OUTPUT); // Strip 2
  pinMode(STRIP_3_CONTROL, OUTPUT); // Strip 3
  pinMode(STRIP_4_CONTROL, OUTPUT); // Strip 4
  pinMode(STRIP_5_CONTROL, OUTPUT); // Strip 5

  // 5050 control lines
  pinMode(8, OUTPUT); // Red
  pinMode(9, OUTPUT); // Green
  pinMode(10, OUTPUT); // Blue

  // solenoid inputs
  pinMode(A0, INPUT); // 5V Power
  pinMode(A1, INPUT_PULLUP); // Left Flipper
  pinMode(A2, INPUT_PULLUP); // Right Flipper
  pinMode(A3, INPUT_PULLUP); 
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  pinMode(A6, INPUT_PULLUP);
  pinMode(A7, INPUT_PULLUP);
  pinMode(A8, INPUT_PULLUP);
  pinMode(A9, INPUT_PULLUP);
  pinMode(A10, INPUT_PULLUP);
  pinMode(A11, INPUT_PULLUP);
  pinMode(A12, INPUT_PULLUP);
  pinMode(A13, INPUT_PULLUP);
  pinMode(A14, INPUT_PULLUP);
  pinMode(A15, INPUT_PULLUP);

  // The game specific .c file will initialize whatever
  // strips it needs and the RGB output
  InitializeAllStrips();
  InitializeRGBValues();


//  Serial.begin(115200);
//  Serial.write("Start\n");

  if (LightingNeedsToWatchI2C()) {
    UseWatchdogTimer = true;
    ALBCommunicationHelper.InitIncomingCommunication(IncomingI2CDeviceAddress(), IncomingALBMessageHandler);
//    Serial.write("Initializing incoming I2C\n");
  }

  ReadSettings();

}


void IncomingALBMessageHandler(byte *message) {

  // double check message authenticity
  if (message[0]==ALB_HEADER_BYTE_1 && message[1]==ALB_HEADER_BYTE_2) {
    switch (message[3]) {
      case ALB_COMMAND_ENABLE_LAMPS:
        MachineState = MACHINE_STATE_GAME_MODE;
        MachineStateChangedTime = CurrentTime;
//        Serial.write("Got a message to enable lamps\n");
        break;
      case ALB_COMMAND_DISABLE_LAMPS:
        MachineState = MACHINE_STATE_OFF;
        MachineStateChangedTime = CurrentTime;
//        Serial.write("Got a message to disable lamps\n");
        break;
      case ALB_COMMAND_WATCHDOG_TIMER_RESET:
        WatchDogTimerStartTime = millis();
        break;
      case ALB_COMMAND_STOP_ALL_ANIMATIONS:
        LastALBMessage = message[3];
        LastALBMessageParameter = 0;
        LastALBMessageRed = 0;
        LastALBMessageGreen = 0;
        LastALBMessageDuration = 0;
        LastALBMessageBlue = 0;
        LastALBMessageSeenTime = CurrentTime;
//        Serial.write("Stop all\n");
        HandleI2CMessage(CurrentTime, LastALBMessage, LastALBMessageParameter, LastALBMessageRed, LastALBMessageGreen, LastALBMessageBlue, LastALBMessageDuration);
        break;
      case ALB_COMMAND_PLAY_ANIMATION:
      case ALB_COMMAND_LOOP_ANIMATION:
      case ALB_COMMAND_STOP_ANIMATION:
      case ALB_COMMAND_ALL_LAMPS_OFF:
      case ALB_COMMAND_STOP_ANIMATION_BY_AREA:
        LastALBMessage = message[3];
        LastALBMessageParameter = message[4];
        LastALBMessageRed = 0;
        LastALBMessageGreen = 0;
        LastALBMessageDuration = 0;
        LastALBMessageBlue = 0;
        LastALBMessageSeenTime = CurrentTime;
        HandleI2CMessage(CurrentTime, LastALBMessage, LastALBMessageParameter, LastALBMessageRed, LastALBMessageGreen, LastALBMessageBlue, LastALBMessageDuration);
        break;
      case ALB_COMMAND_REQUEST_SETTING:
        // The client has asked for a setting value, so we prepare it to send back
        ALBCommunicationHelper.SetRequestedSettingValue(GetSettingValue(message[4]));
        break;
      case ALB_COMMAND_PLAY_WITH_COLOR:
      case ALB_COMMAND_LOOP_WITH_COLOR:
      case ALB_COMMAND_ADJUST_SETTING:
        LastALBMessage = message[3];
        LastALBMessageParameter = message[4];
        LastALBMessageRed = message[5];
        LastALBMessageGreen = message[6];
        LastALBMessageBlue = message[7];
        LastALBMessageDuration = 0;
        LastALBMessageSeenTime = CurrentTime;
        HandleI2CMessage(CurrentTime, LastALBMessage, LastALBMessageParameter, LastALBMessageRed, LastALBMessageGreen, LastALBMessageBlue, LastALBMessageDuration);
        break;
      case ALB_COMMAND_PLAY_WITH_COLOR_AND_DURATION:
      case ALB_COMMAND_LOOP_WITH_COLOR_AND_DURATION:
        LastALBMessage = message[3];
        LastALBMessageParameter = message[4];
        LastALBMessageRed = message[5];
        LastALBMessageGreen = message[6];
        LastALBMessageBlue = message[7];
        LastALBMessageDuration = ((short)message[8]) * 256;
        LastALBMessageDuration += message[9];
        LastALBMessageSeenTime = CurrentTime;
//        Serial.write("Play/Loop with color and duration\n");
        HandleI2CMessage(CurrentTime, LastALBMessage, LastALBMessageParameter, LastALBMessageRed, LastALBMessageGreen, LastALBMessageBlue, LastALBMessageDuration);
        break;
    }
        
  }

}


void SetMachineStateBasedOn5V() {
  if (LightingNeedsToCheckMachineFor5V()) {
    boolean machineVoltageOn = (PINK & 0x20)?true:false;
    if (machineVoltageOn && MachineState==MACHINE_STATE_OFF && CurrentTime>1000) {
      MachineStateChangedTime = CurrentTime;
      MachineState = MACHINE_STATE_ATTRACT_MODE;
    } else if (!machineVoltageOn && MachineState!=MACHINE_STATE_OFF) {
      LastFlipper[0] = 0;
      LastFlipper[1] = 0;
    }
  } else {
    if (MachineStateChangedTime==0) {
      MachineStateChangedTime = CurrentTime;
      MachineState = MACHINE_STATE_ATTRACT_MODE;
    }
  }  
}


void UpdateMachineStateBasedOnFlippers(unsigned long timeBeforeAttract) {
  if (LightingNeedsToCheckMachineForFlipperActivity()==false) {
    if (MachineState==MACHINE_STATE_ATTRACT_MODE) MachineState = MACHINE_STATE_GAME_MODE;
    return;  
  }

  LastFlipper[0] = ((PINK & 0x40)==0x00) ? CurrentTime : LastFlipper[0];
  LastFlipper[1] = ((PINK & 0x80)==0x00) ? CurrentTime : LastFlipper[1];

  if ( (LastFlipper[0] || LastFlipper[1]) && (CurrentTime<(LastFlipper[0]+timeBeforeAttract) || CurrentTime<(LastFlipper[1]+timeBeforeAttract))) {    
    // A flipper was seen in the "timeBeforeAttract" interval
    if (MachineState==MACHINE_STATE_ATTRACT_MODE) {
      MachineState = MACHINE_STATE_GAME_MODE;
      MachineStateChangedTime = CurrentTime;
    }
  } else {
    if (MachineState==MACHINE_STATE_GAME_MODE) {
      MachineState = MACHINE_STATE_ATTRACT_MODE;
      MachineStateChangedTime = CurrentTime;
    }
  }
  
}


void UpdateMachineStateBasedOnWatchdogTimer() {
  if (UseWatchdogTimer) {
    if (CurrentTime>(WatchDogTimerStartTime + ALB_WATCHDOG_TIMER_DURATION)) {
      if (MachineState!=MACHINE_STATE_OFF) {
        MachineStateChangedTime = CurrentTime;
        MachineState = MACHINE_STATE_OFF;
//        Serial.write("Too long since watchdog -- turning off\n");
      }
    } else if (MachineState==MACHINE_STATE_OFF) {
      MachineStateChangedTime = CurrentTime;
      MachineState = MACHINE_STATE_ATTRACT_MODE;
//      Serial.write("Saw watchdog -- turning on\n");
    }
  }
}


unsigned long LastInputSeenTime[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void CheckInputs() {

  // Note:
  // This function uses all port definitions to read the lines
  // instead of digitalRead() because it's so much faster to 
  // read directly from ports. 
  if ( (PINF & 0x01)==0x00 ) LastInputSeenTime[13] = CurrentTime;
  if ( (PINF & 0x02)==0x00 ) LastInputSeenTime[15] = CurrentTime;
  if ( (PINF & 0x04)==0x00 ) LastInputSeenTime[12] = CurrentTime;
  if ( (PINF & 0x08)==0x00 ) LastInputSeenTime[14] = CurrentTime;
  if ( (PINF & 0x10)==0x00 ) LastInputSeenTime[11] = CurrentTime;
  if ( (PINF & 0x20)==0x00 ) LastInputSeenTime[6] = CurrentTime;
  if ( (PINF & 0x40)==0x00 ) LastInputSeenTime[10] = CurrentTime;
  if ( (PINF & 0x80)==0x00 ) LastInputSeenTime[5] = CurrentTime;
  
  if ( (PINK & 0x01)==0x00 ) LastInputSeenTime[9] = CurrentTime;
  if ( (PINK & 0x02)==0x00 ) LastInputSeenTime[4] = CurrentTime;
  if ( (PINK & 0x04)==0x00 ) LastInputSeenTime[8] = CurrentTime;
  if ( (PINK & 0x08)==0x00 ) LastInputSeenTime[3] = CurrentTime;
  if ( (PINK & 0x10)==0x00 ) LastInputSeenTime[7] = CurrentTime;

  // These are flippers and power, but collect them anyway...
  if ( (PINK & 0x20)==0x00 ) LastInputSeenTime[0] = CurrentTime;
  if ( (PINK & 0x40)==0x00 ) LastInputSeenTime[1] = CurrentTime;
  if ( (PINK & 0x80)==0x00 ) LastInputSeenTime[2] = CurrentTime;
}


void CheckControlButtons() {
  for (byte count=0; count<3; count++) {
    if (digitalRead(14+count)==0 && CurrentTime>(LastButtonPressTime[count]+250)) {
      if (LastButtonState[count]==false) {
        LastButtonState[count] = true;
        LastButtonPressTime[count] = CurrentTime;
      }
    } else {
      LastButtonState[count] = false;      
    }
  }

  if (LastButtonState[2]) {
    SettingsMode = AdvanceSettingsMode(SettingsMode);
    WriteSettings();
    LastButtonState[2] = false;
  }

  if (LastButtonState[0]) {
    if (SettingsMode!=SETTINGS_MODE_OFF) IncreaseBrightness(SettingsMode);
    LastButtonState[0] = false;
  }
  
  if (LastButtonState[1]) {
    if (SettingsMode!=SETTINGS_MODE_OFF) DecreaseBrightness(SettingsMode);
    LastButtonState[1] = false;
  }
  
}

unsigned long LastReportTime = 0;

void loop() {
  CurrentTime = millis();
  unsigned long currentFrame = (CurrentTime/LED_UPDATE_FRAME_PERIOD_IN_MS);

  // Check the control buttons 
  CheckControlButtons();

  if (SettingsMode==SETTINGS_MODE_OFF) { 
    // Set the machine state based on 5V sensor
    // or default it to "MACHINE_STATE_ATTRACT_MODE"
    // if no 5V sensor is used
    SetMachineStateBasedOn5V();
  
    // Update the machine state based on Flipper sensors
    // or default it to "MACHINE_STATE_GAME_MODE"
    // if they're not used. The parameter tells the function
    // how long to wait after last flipper before we
    // go back to attract mode (in milliseconds).
    UpdateMachineStateBasedOnFlippers(50000);

    // If this device is controlled by I2C, then
    // the watchdog timer dictates on/off state
    UpdateMachineStateBasedOnWatchdogTimer();
  
    // Check all the inputs for animation cues 
    CheckInputs();
    
    // Set the 5050 channels based on machine state & inputs
    UpdateRGBBasedOnInputs(LastInputSeenTime, CurrentTime, currentFrame, MachineState, MachineStateChangedTime);
    UpdateRGBBasedOnI2C(  LastALBMessageSeenTime, LastALBMessage, LastALBMessageParameter, 
                          LastALBMessageRed, LastALBMessageGreen, LastALBMessageBlue, LastALBMessageDuration,
                          CurrentTime, currentFrame, MachineState, MachineStateChangedTime);
  
    // Update the WS2812 Strips based on machine state & inputs
    UpdateStripsBasedOnInputs(LastInputSeenTime, CurrentTime, currentFrame, MachineState, MachineStateChangedTime);
    UpdateStripsBasedOnI2C( LastALBMessageSeenTime, LastALBMessage, LastALBMessageParameter, 
                            LastALBMessageRed, LastALBMessageGreen, LastALBMessageBlue, LastALBMessageDuration,
                            CurrentTime, currentFrame, MachineState, MachineStateChangedTime);
  } else {
    // Show animations for brightness control
    ShowSettingsMode(SettingsMode, currentFrame);
  }

/*
  if (CurrentTime>(LastReportTime+1000)) {
    char buf[128];
    sprintf(buf, "Time = %lu\n", CurrentTime);
    Serial.write(buf);
    LastReportTime = CurrentTime;
  }
*/  

}
