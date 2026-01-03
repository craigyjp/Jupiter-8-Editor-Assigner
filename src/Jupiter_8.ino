#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include <USBHost_t36.h>
#include "MidiCC.h"
#include "Constants.h"
#include "Parameters.h"
#include "PatchMgr.h"
#include "Button.h"
#include "HWControls.h"
#include "EepromMgr.h"
#include "Settings.h"
#include <map>  // Include the map library

std::map<int, int> voiceAssignment;

#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page
#define PERFORMANCE_RECALL 10
#define PERFORMANCE_SAVE 11
#define PERFORMANCE_EDIT 12
#define PERFORMANCE_NAMING 13
#define PERFORMANCE_DELETE 14
#define PERFORMANCE_DELETEMSG 15

unsigned int state = PARAMETER;

uint32_t int_ref_on_flexible_mode = 0b00001001000010100000000000000000;  // { 0000 , 1001 , 0000 , 1010000000000000 , 0000 }

uint32_t sample_data1 = 0b00000000000000000000000000000000;
uint32_t sample_data2 = 0b00000000000000000000000000000000;
uint32_t sample_data3 = 0b00000000000000000000000000000000;
uint32_t sample_data4 = 0b00000000000000000000000000000000;
uint32_t channel_a = 0b00000010000000000000000000000000;
uint32_t channel_b = 0b00000010000100000000000000000000;
uint32_t channel_c = 0b00000010001000000000000000000000;
uint32_t channel_d = 0b00000010001100000000000000000000;
uint32_t channel_e = 0b00000010010000000000000000000000;
uint32_t channel_f = 0b00000010010100000000000000000000;
uint32_t channel_g = 0b00000010011000000000000000000000;
uint32_t channel_h = 0b00000010011100000000000000000000;

enum PlayMode {
  WHOLE = 0,
  DUAL = 1,
  SPLIT = 2
};

struct Performance {
  int performanceNo;
  int upperPatchNo;
  int lowerPatchNo;
  String name;
  PlayMode mode;  // ← Back to enum type!
};

#include "ST7735Display.h"

boolean cardStatus = false;

struct VoiceAndNote {
  int note;
  int velocity;
  unsigned long timeOn;
  bool sustained;  // Sustain flag
  bool keyDown;
  double noteFreq;  // Note frequency
  int position;
  bool noteOn;
};

struct VoiceAndNote voices[NO_OF_VOICES] = {
  { -1, -1, 0, false, false, 0, -1, false },
  { -1, -1, 0, false, false, 0, -1, false },
  { -1, -1, 0, false, false, 0, -1, false },
  { -1, -1, 0, false, false, 0, -1, false },
  { -1, -1, 0, false, false, 0, -1, false },
  { -1, -1, 0, false, false, 0, -1, false },
  { -1, -1, 0, false, false, 0, -1, false },
  { -1, -1, 0, false, false, 0, -1, false }
};

// Tracks exactly which note each voice currently plays
int voiceToNoteLower[4] = { -1, -1, -1, -1 };
int voiceToNoteUpper[4] = { -1, -1, -1, -1 };


boolean voiceOn[NO_OF_VOICES] = { false, false, false, false, false, false, false, false };
int prevNote = 0;  //Initialised to middle value
bool notes[128] = { 0 }, initial_loop = 1;
int8_t noteOrder[40] = { 0 }, orderIndx = { 0 };

bool notesWhole[128], notesLower[128], notesUpper[128];
byte noteOrderWhole[40], noteOrderLower[40], noteOrderUpper[40];
int orderIndxWhole = 0, orderIndxLower = 0, orderIndxUpper = 0;

int voiceAssignmentLower[128];
int voiceAssignmentUpper[128];

CircularBuffer<Performance, PERFORMANCES_LIMIT> performances;
Performance currentPerformance;


//USB HOST MIDI Class Compliant
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
MIDIDevice midi1(myusb);


//MIDI 5 Pin DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);   // main MIDI in and out
MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, MIDI6);  // MIDI out to voices
MIDI_CREATE_INSTANCE(HardwareSerial, Serial7, MIDI7);  // MIDI out to display (not connected)

int count = 0;  //For MIDI Clk Sync
int DelayForSH3 = 50;
int patchNo = 0;
int patchNoU = 0;
int patchNoL = 0;
int voiceToReturn = -1;                 //Initialise
unsigned long earliestTime = millis();  //For voice allocation - initialise to now
unsigned long buttonDebounce = 0;

void pollAllMCPs();

void initButtons();

int getEncoderSpeed(int id);

void setup() {

  chordHoldActive = false;
  chordHoldWaitingForNotes = false;
  chordHoldCount = 0;

  SPI.begin();
  Wire.begin();           // Join the I2C bus as Master
  Wire.setClock(400000);  // Set I2C speed to 400 kHz

  mcp1.begin(0);
  delay(10);
  mcp2.begin(1);
  delay(10);
  mcp3.begin(2);
  delay(10);
  mcp4.begin(3);
  delay(10);
  mcp5.begin(4);
  delay(10);
  mcp6.begin(5);
  delay(10);
  mcp7.begin(6);
  delay(10);
  mcp8.begin(8);
  delay(10);

  initButtons();

  mcp1.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp1.pinMode(8, OUTPUT);   // pin 8 = GPB0 of MCP2301X
  mcp1.pinMode(9, OUTPUT);   // pin 9 = GPB1 of MCP2301X
  mcp1.pinMode(10, OUTPUT);  // pin 10 = GPB2 of MCP2301X
  mcp1.pinMode(11, OUTPUT);  // pin 11 = GPB3 of MCP2301X
  mcp1.pinMode(12, OUTPUT);  // pin 12 = GPB4 of MCP2301X
  mcp1.pinMode(13, OUTPUT);  // pin 13 = GPB5 of MCP2301X
  mcp1.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp1.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp2.pinMode(1, OUTPUT);   // pin 1 = GPA1 of MCP2301X
  mcp2.pinMode(3, OUTPUT);   // pin 3 = GPA3 of MCP2301X
  mcp2.pinMode(5, OUTPUT);   // pin 5 = GPA5 of MCP2301X
  mcp2.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp2.pinMode(9, OUTPUT);   // pin 9 = GPB1 of MCP2301X
  mcp2.pinMode(11, OUTPUT);  // pin 11 = GPB3 of MCP2301X
  mcp2.pinMode(13, OUTPUT);  // pin 13 = GPB5 of MCP2301X
  mcp2.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp3.pinMode(4, OUTPUT);   // pin 4 = GPA4 of MCP2301X
  mcp3.pinMode(5, OUTPUT);   // pin 5 = GPA5 of MCP2301X
  mcp3.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp3.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp3.pinMode(12, OUTPUT);  // pin 12 = GPB4 of MCP2301X
  mcp3.pinMode(13, OUTPUT);  // pin 13 = GPB5 of MCP2301X
  mcp3.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp3.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp4.pinMode(0, OUTPUT);   // pin 0 = GPA0 of MCP2301X
  mcp4.pinMode(1, OUTPUT);   // pin 1 = GPA1 of MCP2301X
  mcp4.pinMode(2, OUTPUT);   // pin 2 = GPA2 of MCP2301X
  mcp4.pinMode(3, OUTPUT);   // pin 3 = GPA3 of MCP2301X
  mcp4.pinMode(4, OUTPUT);   // pin 4 = GPA4 of MCP2301X
  mcp4.pinMode(5, OUTPUT);   // pin 5 = GPA5 of MCP2301X
  mcp4.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp4.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp4.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  // mcp5.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  // mcp5.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  // mcp6.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  // mcp6.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  // mcp6.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  // mcp6.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp7.pinMode(4, OUTPUT);   // pin 4 = GPA4 of MCP2301X
  mcp7.pinMode(5, OUTPUT);   // pin 5 = GPA5 of MCP2301X
  mcp7.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp7.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp7.pinMode(8, OUTPUT);   // pin 8 = GPB0 of MCP2301X
  mcp7.pinMode(9, OUTPUT);   // pin 9 = GPB1 of MCP2301X
  mcp7.pinMode(10, OUTPUT);  // pin 10 = GPB2 of MCP2301X
  mcp7.pinMode(11, OUTPUT);  // pin 11 = GPB3 of MCP2301X
  mcp7.pinMode(13, OUTPUT);  // pin 13 = GPB7 of MCP2301X

  mcp8.pinMode(1, OUTPUT);   // pin 1 = GPA1 of MCP2301X
  mcp8.pinMode(2, OUTPUT);   // pin 2 = GPA2 of MCP2301X
  mcp8.pinMode(9, OUTPUT);   // pin 9 = GPB1 of MCP2301X
  mcp8.pinMode(10, OUTPUT);  // pin 10 = GPB2 of MCP2301X

  setupDisplay();
  setUpSettings();
  setupHardware();

  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE1));
  digitalWrite(DAC_CS1, LOW);
  delayMicroseconds(1);
  SPI.transfer32(int_ref_on_flexible_mode);
  digitalWrite(DAC_CS1, HIGH);
  SPI.endTransaction();

  for (int i = 0; i < 128; i++) {
    voiceAssignmentLower[i] = -1;
    voiceAssignmentUpper[i] = -1;
  }

  for (int i = 0; i < 4; i++) {
    voiceToNoteLower[i] = -1;
    voiceToNoteUpper[i] = -1;
  }


  cardStatus = SD.begin(BUILTIN_SDCARD);
  if (cardStatus) {
    Serial.println("SD card is connected");
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
    loadPerformances();
    if (performances.size() == 0 && patches.size() > 0) {
      Performance defaultPerf = {
        1,
        patches.first().patchNo,
        patches.first().patchNo,
        "Default"
      };
      performances.push(defaultPerf);
      savePerformance("perf001", defaultPerf);
      loadPerformances();  // reload to ensure it's in the buffer
    }
  } else {
    Serial.println("SD card is not connected or unusable");
    reinitialiseToPanel();
    showPatchPage("No SD", "conn'd / usable", "", "");
  }

  //Read MIDI Channel from EEPROM
  midiChannel = getMIDIChannel();
  Serial.println("MIDI Ch:" + String(midiChannel) + " (0 is Omni On)");

  //USB HOST MIDI Class Compliant
  delay(400);  //Wait to turn on USB Host
  myusb.begin();
  midi1.setHandleControlChange(editControlChange);
  midi1.setHandleNoteOff(myNoteOff);
  midi1.setHandleNoteOn(myNoteOn);
  midi1.setHandlePitchChange(DinHandlePitchBend);
  midi1.setHandleAfterTouch(myAfterTouch);
  Serial.println("USB HOST MIDI Class Compliant Listening");

  //USB Client MIDI
  usbMIDI.setHandleControlChange(editControlChange);
  usbMIDI.setHandleProgramChange(myProgramChange);
  usbMIDI.setHandleAfterTouchChannel(myAfterTouch);
  usbMIDI.setHandlePitchChange(DinHandlePitchBend);
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleNoteOff(myNoteOff);
  Serial.println("USB Client MIDI Listening");

  //MIDI 5 Pin DIN
  MIDI.begin();
  MIDI.setHandleControlChange(editControlChange);
  MIDI.setHandleProgramChange(myProgramChange);
  MIDI.setHandleAfterTouchChannel(myAfterTouch);
  MIDI.setHandlePitchBend(DinHandlePitchBend);
  MIDI.setHandleNoteOn(myNoteOn);
  MIDI.setHandleNoteOff(myNoteOff);
  MIDI.turnThruOn(midi::Thru::Mode::Off);
  Serial.println("MIDI In DIN Listening");

  MIDI7.begin();
  MIDI7.turnThruOn(midi::Thru::Mode::Off);

  MIDI6.begin();
  MIDI6.turnThruOn(midi::Thru::Mode::Off);

  //Read Aftertouch from EEPROM, this can be set individually by each patch.
  upperData[P_AfterTouchDest] = getAfterTouchU();
  lowerData[P_AfterTouchDest] = getAfterTouchL();

  splitPoint = getSplitPoint();
  splitPoint = (splitPoint + 36);

  splitTrans = getSplitTrans();
  setTranspose(splitTrans);

  //Read Encoder Direction from EEPROM
  encCW = getEncoderDir();

  // Read the encoders accelerate
  accelerate = getEncoderAccelerate();

  //setupDisplay();
  delay(500);

  MIDI6.sendProgramChange(0, 1);
  MIDI7.sendProgramChange(0, 1);

  patchNoU = 1;
  patchNoL = 1;
  upperSW = false;
  lowerSW = true;
  //updatekeyboardMode(0);
  updateupperSW(0);
  updatelowerSW(0);
  updateplayMode(0);
  recallPatch(patchNoL);  //Load first patch
}

void pollAllMCPs() {

  for (int j = 0; j < numMCPs; j++) {
    uint16_t gpioAB = allMCPs[j]->readGPIOAB();

    for (auto &button : allButtons) {
      if (button->getMcp() == allMCPs[j]) {
        button->feedInput(gpioAB);
      }
    }
  }
}

void initButtons() {
  for (auto &button : allButtons) {
    button->begin();
  }
}

int getEncoderSpeed(int id) {
  if (id < 1 || id > numEncoders) return 1;

  unsigned long now = millis();
  unsigned long dt = now - lastTransition[id];

  // Linear acceleration mapping
  float minMult = 1.0f;
  float maxMult = 10.0f;
  float minDt = 30.0f;   // Fastest spins
  float maxDt = 350.0f;  // Slowest for any acceleration

  float mult;
  if (dt < minDt)
    mult = maxMult;
  else if (dt > maxDt)
    mult = minMult;
  else
    mult = maxMult - (maxMult - minMult) * ((dt - minDt) / (maxDt - minDt));

  // Optional: smooth multiplier for less jumpy response
  float alpha = 0.5f;  // 0.0 = no smoothing, 1.0 = max smoothing
  lastSpeed[id] = alpha * mult + (1.0f - alpha) * lastSpeed[id];

  lastTransition[id] = now;
  return (int)(lastSpeed[id] + 0.5f);
}

void mainButtonChanged(Button *btn, bool released) {

  switch (btn->id) {

    case DUAL_BUTTON:
      if (!released) {
        dual_button = true;
        dualmode = true;
        myControlChange(midiChannel, CCdual_button, dual_button);
      }
      break;

    case SPLIT_BUTTON:
      if (!released) {
        split_button = true;
        splitmode = true;
        myControlChange(midiChannel, CCsplit_button, split_button);
      }
      break;

    case WHOLE_BUTTON:
      if (!released) {
        whole_button = true;
        wholemode = true;
        myControlChange(midiChannel, CCwhole_button, whole_button);
      }
      break;

    case PANEL_LOWER_BUTTON:
      if (!released) {
        lowerSW = true;
        myControlChange(midiChannel, CClowerSW, lowerSW);
      }
      break;

    case PANEL_UPPER_BUTTON:
      if (!released) {
        upperSW = true;
        myControlChange(midiChannel, CCupperSW, upperSW);
      }
      break;

    case PORTAMENTO_BUTTON:
      if (!released) {
        glideSW = glideSW + 1;
        if (glideSW > 2) {
          glideSW = 0;
        }
        myControlChange(midiChannel, CCglideSW, glideSW);
      }
      break;

    case VCO_BEND_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcoBendSW, 1);
      }
      break;

    case VCF_BEND_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcfBendSW, 1);
      }
      break;

    case VCO_MOD_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcoModSW, 1);
      }
      break;

    case VCF_MOD_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcfModSW, 1);
      }
      break;

    case VCO2_SYNC_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvco2Sync, 1);
      }
      break;

    case ENV1_INVERT_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCenv1InvertSW, 1);
      }
      break;

    case ENV2_KEYFOLLOW_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCenv2KeyFollowSW, 1);
      }
      break;

    case CHORUS_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCchorus, 1);
      }
      break;

    case VCF_SLOPE_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcfSlopeSW, 1);
      }
      break;

    case VCF_ENV_SRC_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcfEgSelectSW, 1);
      }
      break;

    case VCO2_RANGE_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvco2RangeSW, 1);
      }
      break;

    case VCO_MOD_DEST_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcoModSelSW, 1);
      }
      break;

    case VCO_PWM_SRC_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCPWMModSW, 1);
      }
      break;

    case VCA_MOD_DEPTH_BUTTON:
      if (!released) {
        myControlChange(midiChannel, CCvcaModSW, 1);
      }
      break;
  }
}

void recallPerformance(const Performance &perf) {
  currentPerformance = perf;
  playMode = perf.mode;

  switch (playMode) {
    case WHOLE:
      recallPatch(perf.lowerPatchNo);
      patchNo = perf.lowerPatchNo;
      refreshPatchDisplayFromState();
      break;
    case DUAL:
    case SPLIT:
      recallPatch(perf.upperPatchNo);
      recallPatch(perf.lowerPatchNo);
      patchNo = perf.lowerPatchNo;
      refreshPatchDisplayFromState();
      break;
  }
}

void refreshPatchDisplayFromState() {
  showPatchPage(
    currentPgmNumU,
    currentPatchNameU,
    currentPgmNumL,
    currentPatchNameL);
}

String getModeName(PlayMode mode) {
  switch (mode) {
    case WHOLE: return "Whole";
    case DUAL: return "Dual";
    case SPLIT: return "Split";
    default: return "-";
  }
}


void loadPerformances() {
  performances.clear();
  File dir = SD.open("/performances");

  if (!dir || !dir.isDirectory()) {
    Serial.println("/performances not found or is not a directory");
    return;
  }

  while (true) {
    File file = dir.openNextFile();
    if (!file) break;

    if (file.isDirectory()) {
      file.close();
      continue;
    }

    String dataLine = file.readStringUntil('\n');
    file.close();

    if (dataLine.length() > 0) {
      int comma1 = dataLine.indexOf(',');
      int comma2 = dataLine.indexOf(',', comma1 + 1);
      int comma3 = dataLine.indexOf(',', comma2 + 1);

      if (comma1 == -1 || comma2 == -1 || comma3 == -1) continue;

      int upper = dataLine.substring(0, comma1).toInt();
      int lower = dataLine.substring(comma1 + 1, comma2).toInt();
      String name = dataLine.substring(comma2 + 1, comma3);
      int mode = dataLine.substring(comma3 + 1).toInt();

      int perfNo = performances.size() + 1;
      performances.push({ perfNo, upper, lower, name, (PlayMode)mode });
    }
  }

  if (performances.size() == 0) {
    Performance defaultPerf = { 1, 1, 1, "Default", WHOLE };
    savePerformance("perf001", defaultPerf);
    loadPerformances();  // try again
  }
}

void savePerformance(const char *fileName, const Performance &perf) {
  String path = "/performances/" + String(fileName);

  if (SD.exists(path.c_str())) {
    SD.remove(path.c_str());
  }

  File file = SD.open(path.c_str(), FILE_WRITE);
  if (file) {
    file.print(perf.upperPatchNo);
    file.print(",");
    file.print(perf.lowerPatchNo);
    file.print(",");
    file.print(perf.name);
    file.print(",");
    file.println((int)perf.mode);  // Save playMode as an integer (0, 1, 2)
    file.close();
  } else {
    Serial.print("Failed to save performance: ");
    Serial.println(path);
  }
}

void editControlChange(byte channel, byte control, byte value) {
  int newvalue = value;
  myControlChange(channel, control, newvalue);
}

int mod(int a, int b) {
  int r = a % b;
  return r < 0 ? r + b : r;
}

void setTranspose(int splitTrans) {
  switch (splitTrans) {
    case 0:
      lowerTranspose = -24;
      oldsplitTrans = splitTrans;
      break;

    case 1:
      lowerTranspose = -12;
      oldsplitTrans = splitTrans;
      break;

    case 2:
      lowerTranspose = 0;
      oldsplitTrans = splitTrans;
      break;

    case 3:
      lowerTranspose = 12;
      oldsplitTrans = splitTrans;
      break;

    case 4:
      lowerTranspose = 24;
      oldsplitTrans = splitTrans;
      break;
  }
}

// Mono lower & uppper

void commandTopNoteLower() {
  int topNote = -1;
  for (int i = 0; i < 128; i++)
    if (notesLower[i]) topNote = i;

  if (topNote >= 0)
    assignVoice(topNote, noteVel, 0);
  else
    releaseVoice(noteMsg, 0);
}

void commandBottomNoteLower() {
  int bottomNote = -1;
  for (int i = 127; i >= 0; i--)
    if (notesLower[i]) bottomNote = i;

  if (bottomNote >= 0)
    assignVoice(bottomNote, noteVel, 0);
  else
    releaseVoice(noteMsg, 0);
}

void commandLastNoteLower() {
  for (int i = 0; i < 40; i++) {
    int8_t idx = noteOrderLower[mod(orderIndxLower - i, 40)];
    if (notesLower[idx]) {
      assignVoice(idx, noteVel, 0);
      return;
    }
  }
  releaseVoice(noteMsg, 0);
}

void commandTopNoteUpper() {
  int topNote = -1;
  for (int i = 0; i < 128; i++)
    if (notesUpper[i]) topNote = i;

  if (topNote >= 0)
    assignVoice(topNote, noteVel, 4);
  else
    releaseVoice(noteMsg, 4);
}

void commandBottomNoteUpper() {
  int bottomNote = -1;
  for (int i = 127; i >= 0; i--)
    if (notesUpper[i]) bottomNote = i;

  if (bottomNote >= 0)
    assignVoice(bottomNote, noteVel, 4);
  else
    releaseVoice(noteMsg, 4);
}

void commandLastNoteUpper() {
  for (int i = 0; i < 40; i++) {
    int8_t idx = noteOrderUpper[mod(orderIndxUpper - i, 40)];
    if (notesUpper[idx]) {
      assignVoice(idx, noteVel, 4);
      return;
    }
  }
  releaseVoice(noteMsg, 4);
}

// Unison lower and upper

void commandTopNoteUniLower() {
  int topNote = -1;
  for (int i = 0; i < 128; i++)
    if (notesLower[i]) topNote = i;

  if (topNote >= 0)
    for (int v = 0; v < 4; v++) assignVoice(topNote, noteVel, v);
  else
    for (int v = 0; v < 4; v++) releaseVoice(noteMsg, v);
}

void commandBottomNoteUniLower() {
  int bottomNote = -1;
  for (int i = 127; i >= 0; i--)
    if (notesLower[i]) bottomNote = i;

  if (bottomNote >= 0)
    for (int v = 0; v < 4; v++) assignVoice(bottomNote, noteVel, v);
  else
    for (int v = 0; v < 4; v++) releaseVoice(noteMsg, v);
}

void commandLastNoteUniLower() {
  for (int i = 0; i < 40; i++) {
    int8_t idx = noteOrderLower[mod(orderIndxLower - i, 40)];
    if (notesLower[idx]) {
      for (int v = 0; v < 4; v++) assignVoice(idx, noteVel, v);
      return;
    }
  }
  for (int v = 0; v < 4; v++) releaseVoice(noteMsg, v);
}

void commandTopNoteUniUpper() {
  int topNote = -1;
  for (int i = 0; i < 128; i++)
    if (notesUpper[i]) topNote = i;

  if (topNote >= 0)
    for (int v = 4; v < 8; v++) assignVoice(topNote, noteVel, v);
  else
    for (int v = 4; v < 8; v++) releaseVoice(noteMsg, v);
}

void commandBottomNoteUniUpper() {
  int bottomNote = -1;
  for (int i = 127; i >= 0; i--)
    if (notesUpper[i]) bottomNote = i;

  if (bottomNote >= 0)
    for (int v = 4; v < 8; v++) assignVoice(bottomNote, noteVel, v);
  else
    for (int v = 4; v < 8; v++) releaseVoice(noteMsg, v);
}

void commandLastNoteUniUpper() {
  for (int i = 0; i < 40; i++) {
    int8_t idx = noteOrderUpper[mod(orderIndxUpper - i, 40)];
    if (notesUpper[idx]) {
      for (int v = 4; v < 8; v++) assignVoice(idx, noteVel, v);
      return;
    }
  }
  for (int v = 4; v < 8; v++) releaseVoice(noteMsg, v);
}

void myNoteOn(byte channel, byte note, byte velocity) {

  prevNote = note;

  int voiceNum = -1;

  switch (playMode) {

    // WHOLE MODE (No changes needed if currently working)
    case 0:
      switch (lowerData[P_keyboardModeSW]) {
        case 0:
          voiceNum = getVoiceNo(-1) - 1;
          assignVoice(note, velocity, voiceNum);
          break;  // Poly1
        case 1:
          voiceNum = getVoiceNoPoly2(-1) - 1;
          assignVoice(note, velocity, voiceNum);
          break;                                             // Poly2
        case 2: commandMonoNoteOn(note, velocity); break;    // Mono
        case 3: commandUnisonNoteOn(note, velocity); break;  // Unison
      }
      voiceAssignment[note] = voiceNum;
      break;

    // DUAL MODE (Explicitly corrected, place this clearly here):
    case 1:
      {
        // Lower Split
        if (lowerData[P_keyboardModeSW] == 1) {  // Poly2 Lower
          int lowerVoice = getLowerSplitVoicePoly2(note);
          int oldNote = voiceToNoteLower[lowerVoice];
          if (oldNote >= 0) {
            releaseVoice(oldNote, lowerVoice);
            voiceAssignmentLower[oldNote] = -1;
          }
          assignVoice(note, velocity, lowerVoice);
          voiceAssignmentLower[note] = lowerVoice;
          voiceToNoteLower[lowerVoice] = note;
        } else if (lowerData[P_keyboardModeSW] == 0) {  // Poly1 Lower
          int lowerVoice = getLowerSplitVoice(note);
          assignVoice(note, velocity, lowerVoice);
          voiceAssignmentLower[note] = lowerVoice;
          voiceToNoteLower[lowerVoice] = note;
        } else if (lowerData[P_keyboardModeSW] == 2) {
          commandMonoNoteOnLower(note, velocity, 2);
        } else if (lowerData[P_keyboardModeSW] == 3) {
          commandUnisonNoteOnLower(note, velocity, 2);
        }

        // Upper Split
        if (upperData[P_keyboardModeSW] == 1) {  // Poly2 Upper
          int upperVoice = getUpperSplitVoicePoly2(note);
          int oldNote = voiceToNoteUpper[upperVoice - 4];
          if (oldNote >= 0) {
            releaseVoice(oldNote, upperVoice);
            voiceAssignmentUpper[oldNote] = -1;
          }
          assignVoice(note, velocity, upperVoice);
          voiceAssignmentUpper[note] = upperVoice;
          voiceToNoteUpper[upperVoice - 4] = note;
        } else if (upperData[P_keyboardModeSW] == 0) {  // Poly1 Upper
          int upperVoice = getUpperSplitVoice(note);
          assignVoice(note, velocity, upperVoice);
          voiceAssignmentUpper[note] = upperVoice;
          voiceToNoteUpper[upperVoice - 4] = note;
        } else if (upperData[P_keyboardModeSW] == 2) {
          commandMonoNoteOnUpper(note, velocity, 2);
        } else if (upperData[P_keyboardModeSW] == 3) {
          commandUnisonNoteOnUpper(note, velocity, 2);
        }
      }
      break;

      // SPLIT MODE (Also explicitly corrected, place here clearly):
    case 2:  // SPLIT MODE explicitly confirmed (note-on):
      if (note < splitPoint) {
        switch (lowerData[P_keyboardModeSW]) {
          case 0:
            voiceNum = getLowerSplitVoice(note);
            assignVoice(note, velocity, voiceNum);
            voiceAssignmentLower[note] = voiceNum;
            voiceToNoteLower[voiceNum] = note;
            break;
          case 1:
            voiceNum = getLowerSplitVoicePoly2(note);
            assignVoice(note, velocity, voiceNum);
            voiceAssignmentLower[note] = voiceNum;
            voiceToNoteLower[voiceNum] = note;
            break;
          case 2:
            commandMonoNoteOnLower(note, velocity, 2);
            break;
          case 3:
            commandUnisonNoteOnLower(note, velocity, 2);
            break;
        }
      } else {
        switch (upperData[P_keyboardModeSW]) {
          case 0:
            voiceNum = getUpperSplitVoice(note);
            assignVoice(note, velocity, voiceNum);
            voiceAssignmentUpper[note] = voiceNum;
            voiceToNoteUpper[voiceNum - 4] = note;
            break;
          case 1:
            voiceNum = getUpperSplitVoicePoly2(note);
            assignVoice(note, velocity, voiceNum);
            voiceAssignmentUpper[note] = voiceNum;
            voiceToNoteUpper[voiceNum - 4] = note;
            break;
          case 2:
            commandMonoNoteOnUpper(note, velocity, 2);
            break;
          case 3:
            commandUnisonNoteOnUpper(note, velocity, 2);
            break;
        }
      }
      break;
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {

  int assignedVoice = voiceAssignment[note];

  switch (playMode) {

    // WHOLE MODE corrected explicitly
    case 0:
      switch (lowerData[P_keyboardModeSW]) {
        case 0:
          assignedVoice = getVoiceNo(note) - 1;
          releaseVoice(note, assignedVoice);
          break;
        case 1:
          assignedVoice = getVoiceNoPoly2(note) - 1;
          releaseVoice(note, assignedVoice);
          break;
        case 2: commandMonoNoteOff(note); break;
        case 3: commandUnisonNoteOff(note); break;
      }
      break;

      // DUAL MODE corrected explicitly
    case 1:  // DUAL MODE Poly2 fix explicitly (note-off):
      {
        // Lower Split
        if (lowerData[P_keyboardModeSW] == 2) commandMonoNoteOffLower(note);
        else if (lowerData[P_keyboardModeSW] == 3) commandUnisonNoteOffLower(note);
        else {
          int lowerVoice = voiceAssignmentLower[note];
          if (lowerVoice >= 0 && lowerVoice <= 3 && voiceToNoteLower[lowerVoice] == note) {
            releaseVoice(note, lowerVoice);
            voiceAssignmentLower[note] = -1;
            voiceToNoteLower[lowerVoice] = -1;
          }
        }

        // Upper Split
        if (upperData[P_keyboardModeSW] == 2) commandMonoNoteOffUpper(note);
        else if (upperData[P_keyboardModeSW] == 3) commandUnisonNoteOffUpper(note);
        else {
          int upperVoice = voiceAssignmentUpper[note];
          if (upperVoice >= 4 && upperVoice <= 7 && voiceToNoteUpper[upperVoice - 4] == note) {
            releaseVoice(note, upperVoice);
            voiceAssignmentUpper[note] = -1;
            voiceToNoteUpper[upperVoice - 4] = -1;
          }
        }
      }
      break;

      // SPLIT MODE corrected explicitly
    case 2:  // SPLIT MODE explicitly corrected (note-off):
      {
        if (note < splitPoint) {
          if (lowerData[P_keyboardModeSW] == 2) {
            commandMonoNoteOffLower(note);
          } else if (lowerData[P_keyboardModeSW] == 3) {
            commandUnisonNoteOffLower(note);
          } else {
            int lowerVoice = voiceAssignmentLower[note];
            if (lowerVoice >= 0 && lowerVoice <= 3 && voiceToNoteLower[lowerVoice] == note) {
              releaseVoice(note, lowerVoice);
              voiceAssignmentLower[note] = -1;
              voiceToNoteLower[lowerVoice] = -1;
            }
          }
        } else {
          if (upperData[P_keyboardModeSW] == 2) {
            commandMonoNoteOffUpper(note);
          } else if (upperData[P_keyboardModeSW] == 3) {
            commandUnisonNoteOffUpper(note);
          } else {
            int upperVoice = voiceAssignmentUpper[note];
            if (upperVoice >= 4 && upperVoice <= 7 && voiceToNoteUpper[upperVoice - 4] == note) {
              releaseVoice(note, upperVoice);
              voiceAssignmentUpper[note] = -1;
              voiceToNoteUpper[upperVoice - 4] = -1;
            }
          }
        }
      }
      break;
  }
}

void commandMonoNoteOn(byte note, byte velocity) {
  notesWhole[note] = true;
  noteMsg = note;
  noteVel = velocity;
  orderIndxWhole = (orderIndxWhole + 1) % 40;
  noteOrderWhole[orderIndxWhole] = note;

  if (2 == 0) commandTopNoteWhole();
  else if (2 == 1) commandBottomNoteWhole();
  else commandLastNoteWhole();
}

void commandMonoNoteOff(byte note) {
  notesWhole[note] = false;
  noteMsg = note;
  commandLastNoteWhole();
}

void commandTopNoteWhole() {
  int topNote = -1;
  for (int i = 0; i < 128; i++)
    if (notesWhole[i]) topNote = i;

  if (topNote >= 0) assignVoice(topNote, noteVel, 0);
  else releaseVoice(noteMsg, 0);
}

void commandBottomNoteWhole() {
  int bottomNote = -1;
  for (int i = 127; i >= 0; i--)
    if (notesWhole[i]) bottomNote = i;

  if (bottomNote >= 0) assignVoice(bottomNote, noteVel, 0);
  else releaseVoice(noteMsg, 0);
}

void commandLastNoteWhole() {
  for (int i = 0; i < 40; i++) {
    int8_t idx = noteOrderWhole[mod(orderIndxWhole - i, 40)];
    if (notesWhole[idx]) {
      assignVoice(idx, noteVel, 0);
      return;
    }
  }
  releaseVoice(noteMsg, 0);
}

void commandUnisonNoteOn(byte note, byte velocity) {
  notesWhole[note] = true;
  noteMsg = note;
  noteVel = velocity;
  orderIndxWhole = (orderIndxWhole + 1) % 40;
  noteOrderWhole[orderIndxWhole] = note;

  if (2 == 0) commandTopNoteUniWhole();
  else if (2 == 1) commandBottomNoteUniWhole();
  else commandLastNoteUniWhole();
}

void commandUnisonNoteOff(byte note) {
  notesWhole[note] = false;
  noteMsg = note;
  commandLastNoteUniWhole();
}

void commandTopNoteUniWhole() {
  int topNote = -1;
  for (int i = 0; i < 128; i++)
    if (notesWhole[i]) topNote = i;
  if (topNote >= 0)
    for (int v = 0; v < 8; v++) assignVoice(topNote, noteVel, v);
  else
    for (int v = 0; v < 8; v++) releaseVoice(noteMsg, v);
}

void commandBottomNoteUniWhole() {
  int bottomNote = -1;
  for (int i = 127; i >= 0; i--)
    if (notesWhole[i]) bottomNote = i;
  if (bottomNote >= 0)
    for (int v = 0; v < 8; v++) assignVoice(bottomNote, noteVel, v);
  else
    for (int v = 0; v < 8; v++) releaseVoice(noteMsg, v);
}

void commandLastNoteUniWhole() {
  for (int i = 0; i < 40; i++) {
    int8_t idx = noteOrderWhole[mod(orderIndxWhole - i, 40)];
    if (notesWhole[idx]) {
      for (int v = 0; v < 8; v++) assignVoice(idx, noteVel, v);
      return;
    }
  }
  for (int v = 0; v < 8; v++) releaseVoice(noteMsg, v);
}


void commandMonoNoteOnUpper(byte note, byte velocity, byte priority) {
  notesUpper[note] = true;
  noteMsg = note;
  noteVel = velocity;
  orderIndxUpper = (orderIndxUpper + 1) % 40;
  noteOrderUpper[orderIndxUpper] = note;
  if (priority == 0) commandTopNoteUpper();
  else if (priority == 1) commandBottomNoteUpper();
  else commandLastNoteUpper();
}

void commandMonoNoteOffUpper(byte note) {
  notesUpper[note] = false;
  noteMsg = note;
  commandLastNoteUpper();
}

void commandMonoNoteOnLower(byte note, byte velocity, byte priority) {
  notesLower[note] = true;
  noteMsg = note;
  noteVel = velocity;
  orderIndxLower = (orderIndxLower + 1) % 40;
  noteOrderLower[orderIndxLower] = note;

  if (priority == 0) commandTopNoteLower();
  else if (priority == 1) commandBottomNoteLower();
  else commandLastNoteLower();
}

void commandMonoNoteOffLower(byte note) {
  notesLower[note] = false;
  noteMsg = note;
  commandLastNoteLower();
}

void commandUnisonNoteOnUpper(byte note, byte velocity, byte priority) {
  notesUpper[note] = true;
  noteMsg = note;                                       // explicitly set here
  noteVel = velocity;                                   // explicitly set here
  if (priority == 0) commandTopNoteUniUpper();          // Highest priority
  else if (priority == 1) commandBottomNoteUniUpper();  // Lowest priority
  else commandLastNoteUniUpper();                       // Last note priority
}

void commandUnisonNoteOffUpper(byte note) {
  notesUpper[note] = false;
  noteMsg = note;  // explicitly set here
  commandLastNoteUniUpper();
}

void commandUnisonNoteOnLower(byte note, byte velocity, byte priority) {
  notesLower[note] = true;
  noteMsg = note;                                       // explicitly set here
  noteVel = velocity;                                   // explicitly set here
  if (priority == 0) commandTopNoteUniLower();          // Highest priority
  else if (priority == 1) commandBottomNoteUniLower();  // Lowest priority
  else commandLastNoteUniLower();                       // Last note priority
}

void commandUnisonNoteOffLower(byte note) {
  notesLower[note] = false;
  noteMsg = note;  // explicitly set here
  commandLastNoteUniLower();
}

int getUpperSplitVoice(byte note) {
  for (int i = 0; i < 4; i++) {
    int idx = 4 + (upperSplitVoicePointer + i) % 4;
    if (!voiceOn[idx]) {
      upperSplitVoicePointer = (idx + 1) % 4;
      return idx;
    }
  }
  // fallback oldest (poly2 style if no voice free)
  int oldest = 4;
  unsigned long oldestTime = voices[4].timeOn;
  for (int i = 5; i < 8; i++)
    if (voices[i].timeOn < oldestTime) {
      oldest = i;
      oldestTime = voices[i].timeOn;
    }
  upperSplitVoicePointer = ((oldest - 4) + 1) % 4;
  return oldest;
}

int getLowerSplitVoice(byte note) {
  for (int i = 0; i < 4; i++) {
    int idx = (lowerSplitVoicePointer + i) % 4;
    if (!voiceOn[idx]) {
      lowerSplitVoicePointer = (idx + 1) % 4;
      return idx;
    }
  }
  int oldest = 0;
  unsigned long oldestTime = voices[0].timeOn;
  for (int i = 1; i < 4; i++)
    if (voices[i].timeOn < oldestTime) {
      oldest = i;
      oldestTime = voices[i].timeOn;
    }
  lowerSplitVoicePointer = (oldest + 1) % 4;
  return oldest;
}

int getLowerSplitVoicePoly2(byte note) {
  for (int i = 0; i < 4; i++)
    if (!voiceOn[i]) return i;

  int oldest = 0;
  unsigned long oldestTime = voices[0].timeOn;

  for (int i = 1; i < 4; i++) {
    if (voices[i].timeOn < oldestTime) {
      oldest = i;
      oldestTime = voices[i].timeOn;
    }
  }
  return oldest;
}

int getUpperSplitVoicePoly2(byte note) {
  for (int i = 4; i < 8; i++)
    if (!voiceOn[i]) return i;

  int oldest = 4;
  unsigned long oldestTime = voices[4].timeOn;

  for (int i = 5; i < 8; i++) {
    if (voices[i].timeOn < oldestTime) {
      oldest = i;
      oldestTime = voices[i].timeOn;
    }
  }
  return oldest;
}

inline void sendVoiceNoteOn(int voiceIdx, byte note, byte vel) {
  if (voiceIdx < 4) MIDI6.sendNoteOn(note, vel, OUT_CH);
  else MIDI7.sendNoteOn(note, vel, OUT_CH);
}

inline void sendVoiceNoteOff(int voiceIdx, byte note) {
  if (voiceIdx < 4) MIDI6.sendNoteOn(note, 0, OUT_CH);
  else MIDI7.sendNoteOn(note, 0, OUT_CH);
}

void assignVoice(byte note, byte velocity, int voiceIdx) {
  if (voiceIdx < 0 || voiceIdx >= 8) return;

  // If this voice is currently sounding a different note, turn it off first
  // (critical for mono/unison priority changes, and for voice stealing)
  if (voices[voiceIdx].noteOn && voices[voiceIdx].note >= 0 && voices[voiceIdx].note != note) {
    sendVoiceNoteOff(voiceIdx, (byte)voices[voiceIdx].note);
  }

  voices[voiceIdx].note = note;
  voices[voiceIdx].velocity = velocity;
  voices[voiceIdx].timeOn = millis();
  voices[voiceIdx].noteOn = true;
  voiceOn[voiceIdx] = true;

  sendVoiceNoteOn(voiceIdx, note, velocity);
}

void releaseVoice(byte note, int voiceIdx) {
  if (voiceIdx < 0 || voiceIdx >= 8) return;

  // Only release if this voice is actually holding that note
  if (voices[voiceIdx].noteOn && voices[voiceIdx].note == note) {
    sendVoiceNoteOff(voiceIdx, note);

    voices[voiceIdx].note = -1;
    voices[voiceIdx].noteOn = false;
    voiceOn[voiceIdx] = false;

    if (voiceIdx < 4) {
      voiceAssignmentLower[note] = -1;
      voiceToNoteLower[voiceIdx] = -1;
    } else {
      voiceAssignmentUpper[note] = -1;
      voiceToNoteUpper[voiceIdx - 4] = -1;
    }
  }
}


int getVoiceNoPoly2(int note) {
  voiceToReturn = -1;       // Initialize to 'null'
  earliestTime = millis();  // Initialize to now

  if (note == -1) {
    // NoteOn() - Get the oldest free voice (recent voices may still be on the release stage)
    if (voices[lastUsedVoice].note == -1) {
      return lastUsedVoice + 1;
    }

    // If the last used voice is not free or doesn't exist, check if the first voice is free
    if (voices[0].note == -1) {
      return 1;
    }

    // Find the lowest available voice for the new note
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == -1) {
        return i + 1;
      }
    }

    // If no voice is available, release the oldest note
    int oldestVoice = 0;
    for (int i = 1; i < NO_OF_VOICES; i++) {
      if (voices[i].timeOn < voices[oldestVoice].timeOn) {
        oldestVoice = i;
      }
    }
    return oldestVoice + 1;
  } else {
    // NoteOff() - Get the voice number from the note
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == note) {
        return i + 1;
      }
    }
  }

  // Shouldn't get here, return voice 1
  return 1;
}


int getVoiceNo(int note) {
  voiceToReturn = -1;       //Initialise to 'null'
  earliestTime = millis();  //Initialise to now
  if (note == -1) {
    //NoteOn() - Get the oldest free voice (recent voices may be still on release stage)
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == -1) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    if (voiceToReturn == -1) {
      //No free voices, need to steal oldest sounding voice
      earliestTime = millis();  //Reinitialise
      for (int i = 0; i < NO_OF_VOICES; i++) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    return voiceToReturn + 1;
  } else {
    //NoteOff() - Get voice number from note
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == note) {
        return i + 1;
      }
    }
  }
  //Shouldn't get here, return voice 1
  return 1;
}

void DinHandlePitchBend(byte channel, int pitch) {
  if (wholemode) {
    MIDI6.sendPitchBend(pitch, 1);
    MIDI7.sendPitchBend(pitch, 1);
  }
  if (dualmode) {
    MIDI6.sendPitchBend(pitch, 1);
    MIDI7.sendPitchBend(pitch, 1);
  }
  if (splitmode) {
    MIDI6.sendPitchBend(pitch, 1);
    MIDI7.sendPitchBend(pitch, 1);
  }
}

void allNotesOff() {
  midiCCOutUpper(WSallNotesOff, 127);
  midiCCOutLower(WSallNotesOff, 127);
}

void updatePWMMod(boolean announce) {
  if (announce) {
    showCurrentParameterPage("VCO PWM", int(PWMModstr));
  }
  if (upperSW) {
    midiCCOut(CCPWMMod, upperData[P_PWMMod]);
    midiCCOutUpper(CCPWMMod, upperData[P_PWMMod]);
  } else {
    midiCCOut(CCPWMMod, lowerData[P_PWMMod]);
    midiCCOutLower(CCPWMMod, lowerData[P_PWMMod]);
    if (wholemode) {
      midiCCOutUpper(CCPWMMod, upperData[P_PWMMod]);
    }
  }
}

void updatecrossMod(boolean announce) {
  if (announce) {
    showCurrentParameterPage("VCO Cross Mod", int(crossModstr));
  }
  if (upperSW) {
    midiCCOut(CCcrossMod, upperData[P_crossMod]);
    midiCCOutUpper(CCcrossMod, upperData[P_crossMod]);
  } else {
    midiCCOut(CCcrossMod, lowerData[P_crossMod]);
    midiCCOutLower(CCcrossMod, lowerData[P_crossMod]);
    if (wholemode) {
      midiCCOutUpper(CCcrossMod, upperData[P_crossMod]);
    }
  }
}

void updateglideTime(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Glide Time", String(glideTimestr * 10) + " Seconds");
  }
  if (upperSW) {
    midiCCOut(CCglideTime, upperData[P_glideTime]);
    midiCCOutUpper(CCglideTime, upperData[P_glideTime]);
  } else {
    midiCCOut(CCglideTime, lowerData[P_glideTime]);
    midiCCOutLower(CCglideTime, lowerData[P_glideTime]);
    if (wholemode) {
      midiCCOutUpper(WSglideTime, upperData[P_glideTime]);
    }
  }
}

void updateFilterCutoff(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Cutoff", String(filterCutoffstr) + " Hz");
  }
  if (upperSW) {
    midiCCOut(CCfilterCutoff, upperData[P_filterCutoff]);
    midiCCOutUpper(CCfilterCutoff, upperData[P_filterCutoff]);
  } else {
    midiCCOut(CCfilterCutoff, lowerData[P_filterCutoff]);
    midiCCOutLower(CCfilterCutoff, lowerData[P_filterCutoff]);
    if (wholemode) {
      midiCCOutUpper(CCfilterCutoff, upperData[P_filterCutoff]);
    }
  }
}

void updatevcfLfoDepth(boolean announce) {
  if (announce) {
    showCurrentParameterPage("TM depth", int(vcfLfoDepthstr));
  }
  if (upperSW) {
    midiCCOut(CCvcfLfoDepth, upperData[P_vcfLfoDepth]);
    midiCCOutUpper(CCvcfLfoDepth, upperData[P_vcfLfoDepth]);
  } else {
    midiCCOut(CCvcfLfoDepth, lowerData[P_vcfLfoDepth]);
    midiCCOutLower(CCvcfLfoDepth, lowerData[P_vcfLfoDepth]);
    if (wholemode) {
      midiCCOutUpper(CCvcfLfoDepth, upperData[P_vcfLfoDepth]);
    }
  }
}

void updateresonance(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Resonance", int(resonancestr));
  }
  if (upperSW) {
    midiCCOut(CCresonance, upperData[P_resonance]);
    midiCCOutUpper(CCresonance, upperData[P_resonance]);
  } else {
    midiCCOut(CCresonance, lowerData[P_resonance]);
    midiCCOutLower(CCresonance, lowerData[P_resonance]);
    if (wholemode) {
      midiCCOutUpper(CCresonance, upperData[P_resonance]);
    }
  }
}

void updatevcfEnvDepth(boolean announce) {
  if (announce) {
    showCurrentParameterPage("EG Depth", int(vcfEnvDepthstr));
  }
  if (upperSW) {
    midiCCOut(CCvcfEnvDepth, upperData[P_vcfEnvDepth]);
    midiCCOutUpper(CCvcfEnvDepth, upperData[P_vcfEnvDepth]);
  } else {
    midiCCOut(CCvcfEnvDepth, lowerData[P_vcfEnvDepth]);
    midiCCOutLower(CCvcfEnvDepth, lowerData[P_vcfEnvDepth]);
    if (wholemode) {
      midiCCOutUpper(CCvcfEnvDepth, upperData[P_vcfEnvDepth]);
    }
  }
}

void updatevcfKeyFollow(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Key Follow", String(vcfKeyFollowstr) + " %");
  }
  if (upperSW) {
    midiCCOut(CCvcfKeyFollow, upperData[P_vcfKeyFollow]);
    midiCCOutUpper(CCvcfKeyFollow, upperData[P_vcfKeyFollow]);
  } else {
    midiCCOut(CCvcfKeyFollow, lowerData[P_vcfKeyFollow]);
    midiCCOutLower(CCvcfKeyFollow, lowerData[P_vcfKeyFollow]);
    if (wholemode) {
      midiCCOutUpper(CCvcfKeyFollow, upperData[P_vcfKeyFollow]);
    }
  }
}

void updatevcaLevel(boolean announce) {
  if (announce) {
    showCurrentParameterPage("VCA Level", String(vcaLevelstr));
  }
  if (upperSW) {
    midiCCOut(CCvcaLevel, upperData[P_vcaLevel]);
    midiCCOutUpper(CCvcaLevel, upperData[P_vcaLevel]);
  } else {
    midiCCOut(CCvcaLevel, lowerData[P_vcaLevel]);
    midiCCOutLower(CCvcaLevel, lowerData[P_vcaLevel]);
    if (wholemode) {
      midiCCOutUpper(CCvcaLevel, upperData[P_vcaLevel]);
    }
  }
}

void updatebendRange(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Bend Range", String(bendRangestr) + " Semitones");
  }
  if (upperSW) {
    midiCCOut(CCbendRange, upperData[P_vcoBendRange]);
    if (upperData[P_vcoBendSW] == 1) {
      midiCCOutUpper(CCbendRange, upperData[P_vcoBendRange]);
    }
  } else {
    midiCCOut(CCbendRange, lowerData[P_vcoBendRange]);
    if (lowerData[P_vcoBendSW] == 1) {
      midiCCOutLower(CCbendRange, lowerData[P_vcoBendRange]);
      if (wholemode) {
        midiCCOutUpper(CCbendRange, upperData[P_vcoBendRange]);
      }
    }
  }
}

void updatedelayLevel(boolean announce) {
  if (announce) {
    if (delayLevelstr == 0) {
      showCurrentParameterPage("Delay Level", "Off");
    } else {
      showCurrentParameterPage("Delay Level", String(delayLevelstr));
    }
  }
  if (upperSW) {
    midiCCOut(CCdelayLevel, upperData[P_delayLevel]);
    midiCCOutUpper(CCdelayLevel, upperData[P_delayLevel]);
  } else {
    midiCCOut(CCdelayLevel, lowerData[P_delayLevel]);
    midiCCOutLower(CCdelayLevel, lowerData[P_delayLevel]);
    if (wholemode) {
      midiCCOutUpper(CCdelayLevel, upperData[P_delayLevel]);
    }
  }
}

void updatedelayTime(boolean announce) {
  if (announce) {
    if (delayTimestr == 0) {
      showCurrentParameterPage("Delay Time", "Off");
    } else {
      showCurrentParameterPage("Delay Time", String(delayTimestr));
    }
  }
  if (upperSW) {
    midiCCOut(CCdelayTime, upperData[P_delayTime]);
    midiCCOutUpper(CCdelayTime, upperData[P_delayTime]);
  } else {
    midiCCOut(CCdelayTime, lowerData[P_delayTime]);
    midiCCOutLower(CCdelayTime, lowerData[P_delayTime]);
    if (wholemode) {
      midiCCOutUpper(CCdelayTime, upperData[P_delayTime]);
    }
  }
}

void updatedelayFeedback(boolean announce) {
  if (announce) {
    if (delayFeedbackstr == 0) {
      showCurrentParameterPage("Delay Feedback", "Off");
    } else {
      showCurrentParameterPage("Delay Feedback", String(delayFeedbackstr));
    }
  }
  if (upperSW) {
    midiCCOut(CCdelayFeedback, upperData[P_delayFeedback]);
    midiCCOutUpper(CCdelayFeedback, upperData[P_delayFeedback]);
  } else {
    midiCCOut(CCdelayFeedback, lowerData[P_delayFeedback]);
    midiCCOutLower(CCdelayFeedback, lowerData[P_delayFeedback]);
    if (wholemode) {
      midiCCOutUpper(CCdelayFeedback, upperData[P_delayFeedback]);
    }
  }
}

void updateLFORate(boolean announce) {

  if (announce) {
    showCurrentParameterPage("LFO Rate", String(LFORatestr) + " Hz");
  }
  if (upperSW) {
    midiCCOut(CClfoRate, upperData[P_lfoRate]);
    midiCCOutUpper(CClfoRate, upperData[P_lfoRate]);
  } else {
    midiCCOut(CClfoRate, lowerData[P_lfoRate]);
    midiCCOutLower(CClfoRate, lowerData[P_lfoRate]);
    if (wholemode) {
      midiCCOutUpper(CClfoRate, upperData[P_lfoRate]);
    }
  }
}

void updatearpRate(boolean announce) {

  if (announce) {
    showCurrentParameterPage("Arp Rate", String(arpRatestr) + " Hz");
  }
  if (upperSW) {
    midiCCOut(CCarpRate, upperData[P_arpRate]);

  } else {
    midiCCOut(CCarpRate, lowerData[P_arpRate]);

    if (wholemode) {
    }
  }
}

void updatelfoDelay(boolean announce) {
  if (announce) {
    showCurrentParameterPage("LFO Delay", String(lfoDelaystr));
  }
  if (upperSW) {
    midiCCOut(CClfoDelay, upperData[P_lfoDelay]);
    midiCCOutUpper(CClfoDelay, upperData[P_lfoDelay]);
  } else {
    midiCCOut(CClfoDelay, lowerData[P_lfoDelay]);
    midiCCOutLower(CClfoDelay, lowerData[P_lfoDelay]);
    if (wholemode) {
      midiCCOutUpper(CClfoDelay, upperData[P_lfoDelay]);
    }
  }
}

void updatevcoLfoMod(boolean announce) {
  if (announce) {
    showCurrentParameterPage("LFO VCO Mod", String(vcoLfoModstr));
  }
  midiCCOut(CCvcoLfoMod, upperData[P_vcoLfoMod]);
  if (upperSW) {
    midiCCOutUpper(CCvcoLfoMod, upperData[P_vcoLfoMod]);
  } else {
    midiCCOutLower(CCvcoLfoMod, lowerData[P_vcoLfoMod]);
    if (wholemode) {
      midiCCOutUpper(CCvcoLfoMod, upperData[P_vcoLfoMod]);
    }
  }
}

void updatevcoEnvMod(boolean announce) {
  if (announce) {
    showCurrentParameterPage("ENV VCO Mod", String(vcoEnvModstr));
  }
  midiCCOut(CCvcoEnvMod, upperData[P_vcoEnvMod]);
  if (upperSW) {
    midiCCOutUpper(CCvcoEnvMod, upperData[P_vcoEnvMod]);
  } else {
    midiCCOutLower(CCvcoEnvMod, lowerData[P_vcoEnvMod]);
    if (wholemode) {
      midiCCOutUpper(CCvcoEnvMod, upperData[P_vcoEnvMod]);
    }
  }
}

void updatelfoWaveform(boolean announce) {

  if (announce) {
    switch (lfoWaveformDisplay) {
      case 0:
        StratuslfoWaveform = "Sine";
        break;

      case 1:
        StratuslfoWaveform = "Triangle";
        break;

      case 2:
        StratuslfoWaveform = "Sawtooth";
        break;

      case 3:
        StratuslfoWaveform = "Square";
        break;

      case 4:
        StratuslfoWaveform = "Random";
        break;

      case 5:
        StratuslfoWaveform = "Noise";
        break;
    }
    showCurrentParameterPage("LFO Wave", StratuslfoWaveform);
  }
  if (upperSW) {
    midiCCOutUpper(CClfoWaveform, upperData[P_lfoWaveform]);
  } else {
    midiCCOutLower(CClfoWaveform, lowerData[P_lfoWaveform]);
    if (wholemode) {
      midiCCOutUpper(CClfoWaveform, upperData[P_lfoWaveform]);
    }
  }
}

void updatevco1Range(boolean announce) {

  if (announce) {
    switch (vco1RangeDisplay) {
      case 0:
        StratuslfoWaveform = "64 Foot";
        break;

      case 1:
        StratuslfoWaveform = "32 Foot";
        break;

      case 2:
        StratuslfoWaveform = "16 Foot";
        break;

      case 3:
        StratuslfoWaveform = "8 Foot";
        break;

      case 4:
        StratuslfoWaveform = "4 Foot";
        break;

      case 5:
        StratuslfoWaveform = "2 Foot";
        break;
    }
    showCurrentParameterPage("VCO1 Range", StratuslfoWaveform);
  }
  if (upperSW) {
    midiCCOut(CCvco1Range, upperData[P_vco1Range]);
    midiCCOutUpper(CCvco1Range, upperData[P_vco1Range]);
  } else {
    midiCCOut(CCvco1Range, lowerData[P_vco1Range]);
    midiCCOutLower(CCvco1Range, lowerData[P_vco1Range]);
    if (wholemode) {
      midiCCOutUpper(CCvco1Range, upperData[P_vco1Range]);
    }
  }
}

void updatevco1Waveform(boolean announce) {

  if (announce) {
    switch (vco1WaveformDisplay) {
      case 0:
        StratuslfoWaveform = "Sine";
        break;

      case 1:
        StratuslfoWaveform = "Triangle";
        break;

      case 2:
        StratuslfoWaveform = "Sawtooth";
        break;

      case 3:
        StratuslfoWaveform = "Pulse";
        break;

      case 4:
        StratuslfoWaveform = "Square";
        break;

      case 5:
        StratuslfoWaveform = "Noise";
        break;
    }
    showCurrentParameterPage("VCO1 Waveform", StratuslfoWaveform);
  }
  if (upperSW) {
    midiCCOut(CCvco1Waveform, upperData[P_vco1Waveform]);
    midiCCOutUpper(CCvco1Waveform, upperData[P_vco1Waveform]);
  } else {
    midiCCOut(CCvco1Waveform, lowerData[P_vco1Waveform]);
    midiCCOutLower(CCvco1Waveform, lowerData[P_vco1Waveform]);
    if (wholemode) {
      midiCCOutUpper(CCvco1Waveform, upperData[P_vco1Waveform]);
    }
  }
}

void updatevco2Range(boolean announce) {

  if (announce) {
    if (vco2WaveformDisplay < 3) {
      switch (vco2RangeDisplay) {
        case 0x00:
          StratuslfoWaveform = "64 Foot";
          break;

        case 0x10:
          StratuslfoWaveform = "32 Foot";
          break;

        case 0x30:
          StratuslfoWaveform = "16 Foot";
          break;

        case 0x50:
          StratuslfoWaveform = "8 Foot";
          break;

        case 0x70:
          StratuslfoWaveform = "4 Foot";
          break;

        case 0x7F:
          StratuslfoWaveform = "2 Foot";
          break;
      }
      showCurrentParameterPage("VCO2 Range", StratuslfoWaveform);
    } else {
      showCurrentParameterPage("VCO2 Low Range", vco2RangeDisplay);
    }
  }
  if (upperSW) {
    midiCCOut(CCvco2Range, upperData[P_vco2Range]);
    midiCCOutUpper(CCvco2Range, upperData[P_vco2Range]);
  } else {
    midiCCOut(CCvco2Range, lowerData[P_vco2Range]);
    midiCCOutLower(CCvco2Range, lowerData[P_vco2Range]);
    if (wholemode) {
      midiCCOutUpper(CCvco2Range, upperData[P_vco2Range]);
    }
  }
}

void updatevco2Waveform(boolean announce) {

  if (announce) {
    switch (vco2WaveformDisplay) {
      case 0:
        StratuslfoWaveform = "Sine";
        break;

      case 1:
        StratuslfoWaveform = "Sawtooth";
        break;

      case 2:
        StratuslfoWaveform = "Pulse";
        break;

      case 3:
        StratuslfoWaveform = "Sine - Low";
        break;

      case 4:
        StratuslfoWaveform = "Sawtooth - Low";
        break;

      case 5:
        StratuslfoWaveform = "Pulse - Low";
        break;
    }
    showCurrentParameterPage("VCO2 Waveform", StratuslfoWaveform);
  }
  if (upperSW) {
    midiCCOut(CCvco2Waveform, upperData[P_vco2Waveform]);
    midiCCOutUpper(CCvco2Waveform, upperData[P_vco2Waveform]);
  } else {
    midiCCOut(CCvco2Waveform, lowerData[P_vco2Waveform]);
    midiCCOutLower(CCvco2Waveform, lowerData[P_vco2Waveform]);
    if (wholemode) {
      midiCCOutUpper(CCvco2Waveform, upperData[P_vco2Waveform]);
    }
  }
}

void updatevco2Fine(boolean announce) {
  if (announce) {
    if (vco2Finestr > 0) {
      showCurrentParameterPage("VCO2 Fine Tune", "+" + String(vco2Finestr));
    } else {
      showCurrentParameterPage("VCO2 Fine Tune", String(vco2Finestr));
    }
  }
  if (upperSW) {
    midiCCOut(CCvco2Fine, upperData[P_vco2Fine]);
    midiCCOutUpper(CCvco2Fine, upperData[P_vco2Fine]);
  } else {
    midiCCOut(CCvco2Fine, lowerData[P_vco2Fine]);
    midiCCOutLower(CCvco2Fine, lowerData[P_vco2Fine]);
    if (wholemode) {
      midiCCOutUpper(CCvco2Fine, upperData[P_vco2Fine]);
    }
  }
}

void updatevcoBalance(boolean announce) {
  if (announce) {
    if (vcoBalancestr > 0) {
      showCurrentParameterPage("VCO Balance", "+" + String(vcoBalancestr));
    } else {
      showCurrentParameterPage("VCO Balance", String(vcoBalancestr));
    }
  }
  if (upperSW) {
    midiCCOut(CCvcoBalance, upperData[P_vcoBalance]);
    midiCCOutUpper(CCvcoBalance, upperData[P_vcoBalance]);
  } else {
    midiCCOut(CCvcoBalance, lowerData[P_vcoBalance]);
    midiCCOutLower(CCvcoBalance, lowerData[P_vcoBalance]);
    if (wholemode) {
      midiCCOutUpper(CCvcoBalance, upperData[P_vcoBalance]);
    }
  }
}

void updateHPF(boolean announce) {
  if (announce) {
    showCurrentParameterPage("HPF Cutoff", String(HPFstr));
  }
  if (upperSW) {
    midiCCOut(CCHPF, upperData[P_HPF]);
    midiCCOutUpper(CCHPF, upperData[P_HPF]);
  } else {
    midiCCOut(CCHPF, lowerData[P_HPF]);
    midiCCOutLower(CCHPF, lowerData[P_HPF]);
    if (wholemode) {
      midiCCOutUpper(CCHPF, upperData[P_HPF]);
    }
  }
}

void updateenv1Attack(boolean announce) {
  if (announce) {
    if (env1Attackstr < 1000) {
      showCurrentParameterPage("ENV1 Attack", String(int(env1Attackstr)) + " ms", FILTER_ENV);
    } else {
      showCurrentParameterPage("ENV1 Attack", String(env1Attackstr * 0.001) + " s", FILTER_ENV);
    }
  }
  if (upperSW) {
    midiCCOut(CCenv1Attack, upperData[P_env1Attack]);
    midiCCOutUpper(CCenv1Attack, upperData[P_env1Attack]);
  } else {
    midiCCOut(CCenv1Attack, lowerData[P_env1Attack]);
    midiCCOutLower(CCenv1Attack, lowerData[P_env1Attack]);
    if (wholemode) {
      midiCCOutUpper(CCenv1Attack, upperData[P_env1Attack]);
    }
  }
}

void updateenv1Decay(boolean announce) {
  if (announce) {
    if (env1Decaystr < 1000) {
      showCurrentParameterPage("ENV1 Decay", String(int(env1Decaystr)) + " ms", FILTER_ENV);
    } else {
      showCurrentParameterPage("ENV1 Decay", String(env1Decaystr * 0.001) + " s", FILTER_ENV);
    }
  }
  if (upperSW) {
    midiCCOut(CCenv1Decay, upperData[P_env1Decay]);
    midiCCOutUpper(CCenv1Decay, upperData[P_env1Decay]);
  } else {
    midiCCOut(CCenv1Decay, lowerData[P_env1Decay]);
    midiCCOutLower(CCenv1Decay, lowerData[P_env1Decay]);
    if (wholemode) {
      midiCCOutUpper(CCenv1Decay, upperData[P_env1Decay]);
    }
  }
}

void updateenv1Sustain(boolean announce) {
  if (announce) {
    showCurrentParameterPage("ENV1 Sustain", String(env1Sustainstr), FILTER_ENV);
  }
  if (upperSW) {
    midiCCOut(CCenv1Sustain, upperData[P_env1Sustain]);
    midiCCOutUpper(CCenv1Sustain, upperData[P_env1Sustain]);
  } else {
    midiCCOut(CCenv1Sustain, lowerData[P_env1Sustain]);
    midiCCOutLower(CCenv1Sustain, lowerData[P_env1Sustain]);
    if (wholemode) {
      midiCCOutUpper(CCenv1Sustain, upperData[P_env1Sustain]);
    }
  }
}

void updateenv1Release(boolean announce) {
  if (announce) {
    if (env1Releasestr < 1000) {
      showCurrentParameterPage("ENV1 Release", String(int(env1Releasestr)) + " ms", FILTER_ENV);
    } else {
      showCurrentParameterPage("ENV1 Release", String(env1Releasestr * 0.001) + " s", FILTER_ENV);
    }
  }
  if (upperSW) {
    midiCCOut(CCenv1Release, upperData[P_env1Release]);
    midiCCOutUpper(CCenv1Release, upperData[P_env1Release]);
  } else {
    midiCCOut(CCenv1Release, lowerData[P_env1Release]);
    midiCCOutLower(CCenv1Release, lowerData[P_env1Release]);
    if (wholemode) {
      midiCCOutUpper(CCenv1Release, upperData[P_env1Release]);
    }
  }
}

void updateenv2Attack(boolean announce) {
  if (announce) {
    if (env2Attackstr < 1000) {
      showCurrentParameterPage("ENV2 Attack", String(int(env2Attackstr)) + " ms", AMP_ENV);
    } else {
      showCurrentParameterPage("ENV2 Attack", String(env2Attackstr * 0.001) + " s", AMP_ENV);
    }
  }
  if (upperSW) {
    midiCCOut(CCenv2Attack, upperData[P_env2Attack]);
    midiCCOutUpper(CCenv2Attack, upperData[P_env2Attack]);
  } else {
    midiCCOut(CCenv2Attack, lowerData[P_env2Attack]);
    midiCCOutLower(CCenv2Attack, lowerData[P_env2Attack]);
    if (wholemode) {
      midiCCOutUpper(CCenv2Attack, upperData[P_env2Attack]);
    }
  }
}

void updateenv2Decay(boolean announce) {
  if (announce) {
    if (env2Decaystr < 1000) {
      showCurrentParameterPage("ENV2 Decay", String(int(env2Decaystr)) + " ms", AMP_ENV);
    } else {
      showCurrentParameterPage("ENV2 Decay", String(env2Decaystr * 0.001) + " s", AMP_ENV);
    }
  }
  if (upperSW) {
    midiCCOut(CCenv2Decay, upperData[P_env2Decay]);
    midiCCOutUpper(CCenv2Decay, upperData[P_env2Decay]);
  } else {
    midiCCOut(CCenv2Decay, lowerData[P_env2Decay]);
    midiCCOutLower(CCenv2Decay, lowerData[P_env2Decay]);
    if (wholemode) {
      midiCCOutUpper(CCenv2Decay, upperData[P_env2Decay]);
    }
  }
}

void updateenv2Sustain(boolean announce) {
  if (announce) {
    showCurrentParameterPage("ENV2 Sustain", String(env2Sustainstr), AMP_ENV);
  }
  if (upperSW) {
    midiCCOut(CCenv2Sustain, upperData[P_env2Sustain]);
    midiCCOutUpper(CCenv2Sustain, upperData[P_env2Sustain]);
  } else {
    midiCCOut(CCenv2Sustain, lowerData[P_env2Sustain]);
    midiCCOutLower(CCenv2Sustain, lowerData[P_env2Sustain]);
    if (wholemode) {
      midiCCOutUpper(CCenv2Sustain, upperData[P_env2Sustain]);
    }
  }
}

void updateenv2Release(boolean announce) {
  if (announce) {
    if (env2Releasestr < 1000) {
      showCurrentParameterPage("ENV2 Release", String(int(env2Releasestr)) + " ms", AMP_ENV);
    } else {
      showCurrentParameterPage("ENV2 Release", String(env2Releasestr * 0.001) + " s", AMP_ENV);
    }
  }
  if (upperSW) {
    midiCCOut(CCenv2Release, upperData[P_env2Release]);
    midiCCOutUpper(CCenv2Release, upperData[P_env2Release]);
  } else {
    midiCCOut(CCenv2Release, lowerData[P_env2Release]);
    midiCCOutLower(CCenv2Release, lowerData[P_env2Release]);
    if (wholemode) {
      midiCCOutUpper(CCenv2Release, upperData[P_env2Release]);
    }
  }
}

void updatevolume(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Volume", int(volumestr));
  }
  if (upperSW) {
    midiCCOut(CCvolume, upperData[P_volume]);

  } else {
    midiCCOut(CCvolume, lowerData[P_volume]);
  }
}

void updatebalance(boolean announce) {
  if (announce) {
    showCurrentParameterPage("Balance", int(balancestr));
  }
  if (upperSW) {
    midiCCOut(CCbalance, upperData[P_balance]);

  } else {
    midiCCOut(CCbalance, lowerData[P_balance]);
  }
}

// // ////////////////////////////////////////////////////////////////

void updatedual_button(boolean announce) {
  if (dualmode) {
    playMode = 1;
    if (announce) {
      showCurrentParameterPage("Key Mode", "Dual");
    }
    mcp4.digitalWrite(SPLIT_LED, LOW);
    mcp4.digitalWrite(WHOLE_LED, LOW);
    mcp4.digitalWrite(DUAL_LED, HIGH);
    wholemode = false;
    dualmode = true;
    splitmode = false;
  }
}

void updatewhole_button(boolean announce) {
  if (wholemode) {
    playMode = 0;
    if (announce) {
      showCurrentParameterPage("Key Mode", "Whole");
    }
    mcp4.digitalWrite(DUAL_LED, LOW);
    mcp4.digitalWrite(SPLIT_LED, LOW);
    mcp4.digitalWrite(WHOLE_LED, HIGH);
    wholemode = true;
    dualmode = false;
    splitmode = false;
    upperSW = false;
    lowerSW = true;
    updatelowerSW(0);
  }
}

void updatesplit_button(boolean announce) {
  if (splitmode) {
    playMode = 2;
    if (announce) {
      showCurrentParameterPage("Key Mode", "Split");
    }
    mcp4.digitalWrite(WHOLE_LED, LOW);
    mcp4.digitalWrite(DUAL_LED, LOW);
    mcp4.digitalWrite(SPLIT_LED, HIGH);
    wholemode = false;
    dualmode = false;
    splitmode = true;
  }
}

void updateplayMode(boolean announce) {
  updatewhole_button(0);
  updatedual_button(0);
  updatesplit_button(0);
}

void updateupperSW(boolean announce) {
  if (!wholemode) {
    if (upperSW) {
      lowerSW = false;
      setAllButtons();
      mcp4.digitalWrite(PM_LOWER_LED, LOW);
      mcp4.digitalWrite(PM_UPPER_LED, HIGH);
    }
  }
}

void updatelowerSW(boolean announce) {
  if (lowerSW) {
    upperSW = false;
    setAllButtons();
    mcp4.digitalWrite(PM_UPPER_LED, LOW);
    mcp4.digitalWrite(PM_LOWER_LED, HIGH);
  }
}

// void updatekeyboardMode(boolean announce) {
//   if (upperSW) {
//     if (dualmode) {
//       lowerData[P_keyboardMode] = upperData[P_keyboardMode];
//     }
//     if (upperData[P_keyboardMode] == 0) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Poly 1");
//       }
//       midiCCOut72(CCkeyboardMode, 0);
//       midiCCOut(CCkeyboardMode, 0);
//     } else if (upperData[P_keyboardMode] == 1) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Poly 2");
//       }
//       midiCCOut72(CCkeyboardMode, 1);
//       midiCCOut(CCkeyboardMode, 1);
//     } else if (upperData[P_keyboardMode] == 2) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Mono");
//       }
//       midiCCOut72(CCkeyboardMode, 2);
//       midiCCOut(CCkeyboardMode, 2);
//     } else if (upperData[P_keyboardMode] == 3) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Unison");
//       }
//       midiCCOut72(CCkeyboardMode, 3);
//       midiCCOut(CCkeyboardMode, 3);
//     }
//   } else {
//     if (dualmode) {
//       upperData[P_keyboardMode] = lowerData[P_keyboardMode];
//     }
//     if (lowerData[P_keyboardMode] == 0) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Poly 1");
//       }
//       midiCCOut72(CCkeyboardMode, 0);
//       midiCCOut(CCkeyboardMode, 0);
//     } else if (lowerData[P_keyboardMode] == 1) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Poly 2");
//       }
//       midiCCOut72(CCkeyboardMode, 1);
//       midiCCOut(CCkeyboardMode, 1);
//     } else if (lowerData[P_keyboardMode] == 2) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Mono");
//       }
//       midiCCOut72(CCkeyboardMode, 2);
//       midiCCOut(CCkeyboardMode, 2);
//     } else if (lowerData[P_keyboardMode] == 3) {
//       if (announce) {
//         showCurrentParameterPage("Keyboard Mode", "Unison");
//       }
//       midiCCOut72(CCkeyboardMode, 3);
//       midiCCOut(CCkeyboardMode, 3);
//     }
//   }
// }

void updateglideSW(boolean announce) {

  if (announce) {
    switch (glideSW) {
      case 0:
        showCurrentParameterPage("Glide", "Off");
        break;

      case 1:
        if (!wholemode) {
          showCurrentParameterPage("Glide", "Upper Only");
        }
        if (wholemode) {
          showCurrentParameterPage("Glide", "Unavailable");
        }
        break;

      case 2:
        showCurrentParameterPage("Glide", "On");
        break;
    }
  }
  switch (glideSW) {
    case 0:
      midiCCOutUpper(CCglideSW, 0);
      midiCCOutLower(CCglideSW, 0);
      mcp1.digitalWrite(GLIDE_LED_GRN, LOW);
      mcp1.digitalWrite(GLIDE_LED_RED, LOW);
      break;

    case 1:
      if (!wholemode) {
        midiCCOutUpper(CCglideSW, 127);
        midiCCOutLower(CCglideSW, 0);
        midiCCOutUpper(CCglideTime, upperData[P_glideTime]);
        mcp1.digitalWrite(GLIDE_LED_GRN, HIGH);
        mcp1.digitalWrite(GLIDE_LED_RED, LOW);
      }
      break;

    case 2:
      midiCCOutUpper(CCglideSW, 127);
      midiCCOutLower(CCglideSW, 127);
      midiCCOutUpper(CCglideTime, upperData[P_glideTime]);
      midiCCOutLower(CCglideTime, lowerData[P_glideTime]);
      mcp1.digitalWrite(GLIDE_LED_GRN, HIGH);
      mcp1.digitalWrite(GLIDE_LED_RED, HIGH);
      break;
  }
}

void updatevcoBendSW(boolean announce) {

  if (upperSW) {
    switch (upperData[P_vcoBendSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("VCO Bend", "Off");
        }
        midiCCOutUpper(CCbendRange, 0);
        mcp1.digitalWrite(VCO_BEND_LED_GRN, LOW);
        mcp1.digitalWrite(VCO_BEND_LED_RED, LOW);
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("VCO Bend", "On");
        }
        midiCCOutUpper(CCbendRange, upperData[P_vcoBendRange]);
        mcp1.digitalWrite(VCO_BEND_LED_GRN, LOW);
        mcp1.digitalWrite(VCO_BEND_LED_RED, HIGH);
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("VCO Bend", "2 Octaves");
        }
        midiCCOutUpper(CCbendRange, 0x18);
        mcp1.digitalWrite(VCO_BEND_LED_GRN, HIGH);
        mcp1.digitalWrite(VCO_BEND_LED_RED, HIGH);
        break;
    }
  } else {
    switch (lowerData[P_vcoBendSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("VCO Bend", "Off");
        }
        midiCCOutLower(CCbendRange, 0);
        mcp1.digitalWrite(VCO_BEND_LED_GRN, LOW);
        mcp1.digitalWrite(VCO_BEND_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCbendRange, 0);
        }
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("VCO Bend", "On");
        }
        midiCCOutLower(CCbendRange, lowerData[P_vcoBendRange]);
        mcp1.digitalWrite(VCO_BEND_LED_GRN, LOW);
        mcp1.digitalWrite(VCO_BEND_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCbendRange, upperData[P_vcoBendRange]);
        }
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("VCO Bend", "2 Octaves");
        }
        midiCCOutLower(CCbendRange, 0x18);
        mcp1.digitalWrite(VCO_BEND_LED_GRN, HIGH);
        mcp1.digitalWrite(VCO_BEND_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCbendRange, 0x18);
        }
        break;
    }
  }
}

void updatevcoModSelSW(boolean announce) {

  if (upperSW) {
    switch (upperData[P_vcoModSelSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("VCO Mod Dest", "VCO1");
        }
        midiCCOutUpper(CCvcoModSelSW, upperData[P_vcoModSelSW]);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_GRN, HIGH);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_RED, LOW);
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("VCO Mod Dest", "VCO1 & VCO2");
        }
        midiCCOutUpper(CCvcoModSelSW, upperData[P_vcoModSelSW]);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_GRN, HIGH);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_RED, HIGH);
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("VCO Mod Dest", "VCO2");
        }
        midiCCOutUpper(CCvcoModSelSW, upperData[P_vcoModSelSW]);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_GRN, LOW);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_RED, HIGH);
        break;
    }
  } else {
    switch (lowerData[P_vcoModSelSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("VCO Mod Dest", "VCO1");
        }
        midiCCOutUpper(CCvcoModSelSW, lowerData[P_vcoModSelSW]);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_GRN, HIGH);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCvcoModSelSW, upperData[P_vcoModSelSW]);
        }
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("VCO Mod Dest", "VCO1 & VCO2");
        }
        midiCCOutUpper(CCvcoModSelSW, lowerData[P_vcoModSelSW]);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_GRN, HIGH);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCvcoModSelSW, upperData[P_vcoModSelSW]);
        }
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("VCO Mod Dest", "VCO2");
        }
        midiCCOutUpper(CCvcoModSelSW, upperData[P_vcoModSelSW]);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_GRN, LOW);
        mcp3.digitalWrite(VCO_MOD_DEST_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCvcoModSelSW, upperData[P_vcoModSelSW]);
        }
        break;
    }
  }
}

void updatePWMModSW(boolean announce) {

  if (upperSW) {
    switch (upperData[P_PWMModSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("PWM Mod Src", "ENV1");
        }
        midiCCOutUpper(CCPWMModSW, upperData[P_PWMModSW]);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_GRN, LOW);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_RED, HIGH);
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("PWM Mod Dest", "Manual");
        }
        midiCCOutUpper(CCPWMModSW, upperData[P_PWMModSW]);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_GRN, HIGH);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_RED, HIGH);
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("VCO Mod Dest", "LFO");
        }
        midiCCOutUpper(CCPWMModSW, upperData[P_PWMModSW]);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_GRN, HIGH);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_RED, LOW);
        break;
    }
  } else {
    switch (lowerData[P_PWMModSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("PWM Mod Src", "ENV1");
        }
        midiCCOutUpper(CCPWMModSW, lowerData[P_PWMModSW]);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_GRN, LOW);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCPWMModSW, upperData[P_PWMModSW]);
        }
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("PWM Mod Src", "Manual");
        }
        midiCCOutUpper(CCPWMModSW, lowerData[P_PWMModSW]);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_GRN, HIGH);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCPWMModSW, upperData[P_PWMModSW]);
        }
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("PWM Mod Src", "LFO");
        }
        midiCCOutUpper(CCPWMModSW, upperData[P_PWMModSW]);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_GRN, HIGH);
        mcp4.digitalWrite(VCO_PWM_SRC_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCPWMModSW, upperData[P_PWMModSW]);
        }
        break;
    }
  }
}

void updatevcaModSW (boolean announce) {

  if (upperSW) {
    switch (upperData[P_vcaModSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "Off");
        }
        midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, LOW);
        mcp7.digitalWrite(VCA_MOD_LED_RED, LOW);
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "1");
        }
        midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, LOW);
        mcp7.digitalWrite(VCA_MOD_LED_RED, HIGH);
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "2");
        }
        midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, HIGH);
        mcp7.digitalWrite(VCA_MOD_LED_RED, LOW);
        break;

      case 3:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "3");
        }
        midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, HIGH);
        mcp7.digitalWrite(VCA_MOD_LED_RED, HIGH);
        break;
    }
  } else {
    switch (lowerData[P_vcaModSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "Off");
        }
        midiCCOutUpper(CCvcaModSW, lowerData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, LOW);
        mcp7.digitalWrite(VCA_MOD_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        }
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "1");
        }
        midiCCOutUpper(CCvcaModSW, lowerData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, LOW);
        mcp7.digitalWrite(VCA_MOD_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        }
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "2");
        }
        midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, HIGH);
        mcp7.digitalWrite(VCA_MOD_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        }
        break;

      case 3:
        if (announce) {
          showCurrentParameterPage("VCA Mod Depth", "3");
        }
        midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        mcp7.digitalWrite(VCA_MOD_LED_GRN, HIGH);
        mcp7.digitalWrite(VCA_MOD_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCvcaModSW, upperData[P_vcaModSW]);
        }
        break;
    }
  }
}

void updateenv1InvertSW(boolean announce) {
  if (upperSW) {
    if (!upperData[P_env1InvertSW]) {
      if (announce) {
        showCurrentParameterPage("ENV1 Polarity", "Invert");
      }
      midiCCOut(CCenv1InvertSW, 0);
      midiCCOutUpper(CCenv1InvertSW, 0);
      mcp7.digitalWrite(ENV1_INVERT_LED_GRN, LOW);
      mcp7.digitalWrite(ENV1_INVERT_LED_RED, HIGH);
    } else {
      if (announce) {
        showCurrentParameterPage("ENV1 Polarity", "Normal");
      }
      midiCCOut(CCenv1InvertSW, 1);
      midiCCOutUpper(CCenv1InvertSW, 1);
      mcp7.digitalWrite(ENV1_INVERT_LED_GRN, HIGH);
      mcp7.digitalWrite(ENV1_INVERT_LED_RED, LOW);
    }
  } else {
    if (!lowerData[P_env1InvertSW]) {
      if (announce) {
        showCurrentParameterPage("ENV1 Polarity", "Invert");
      }
      midiCCOut(CCenv1InvertSW, 0);
      midiCCOutLower(CCenv1InvertSW, 0);
      mcp7.digitalWrite(ENV1_INVERT_LED_GRN, LOW);
      mcp7.digitalWrite(ENV1_INVERT_LED_RED, HIGH);
      if (wholemode) {
        midiCCOutUpper(CCenv1InvertSW, 0);
      }
    } else {
      if (announce) {
        showCurrentParameterPage("ENV1 Polarity", "Normal");
      }
      midiCCOut(CCenv1InvertSW, 1);
      midiCCOutLower(CCenv1InvertSW, 1);
      mcp7.digitalWrite(ENV1_INVERT_LED_GRN, HIGH);
      mcp7.digitalWrite(ENV1_INVERT_LED_RED, LOW);
      if (wholemode) {
        midiCCOutUpper(CCenv1InvertSW, 1);
      }
    }
  }
}

void updateenv2KeyFollowSW (boolean announce) {

  if (upperSW) {
    switch (upperData[P_env2KeyFollowSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("Key Follow", "Off");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, LOW);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, LOW);
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("Key Follow", "ENV1");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, LOW);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, HIGH);
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("Key Follow", "ENV2");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, HIGH);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, LOW);
        break;

      case 3:
        if (announce) {
          showCurrentParameterPage("Key Follow", "ENV1 & 2");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, HIGH);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, HIGH);
        break;
    }
  } else {
    switch (lowerData[P_env2KeyFollowSW]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("Key Follow", "Off");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, lowerData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, LOW);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        }
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("Key Follow", "ENV1");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, lowerData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, LOW);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        }
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("Key Follow", "ENV2");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, HIGH);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        }
        break;

      case 3:
        if (announce) {
          showCurrentParameterPage("Key Follow", "ENV1 & 2");
        }
        midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_GRN, HIGH);
        mcp8.digitalWrite(ENV_KEYFOLLOW_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCenv2KeyFollowSW, upperData[P_env2KeyFollowSW]);
        }
        break;
    }
  }
}

void updatechorus (boolean announce) {

  if (upperSW) {
    switch (upperData[P_chorus]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("Chorus", "Off");
        }
        midiCCOutUpper(CCchorus, upperData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, LOW);
        mcp8.digitalWrite(CHORUS_LED_RED, LOW);
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("Chorus", "1");
        }
        midiCCOutUpper(CCchorus, upperData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, LOW);
        mcp8.digitalWrite(CHORUS_LED_RED, HIGH);
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("Chorus", "2");
        }
        midiCCOutUpper(CCchorus, upperData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, HIGH);
        mcp8.digitalWrite(CHORUS_LED_RED, LOW);
        break;

      case 3:
        if (announce) {
          showCurrentParameterPage("Chorus", "1 & 2");
        }
        midiCCOutUpper(CCchorus, upperData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, HIGH);
        mcp8.digitalWrite(CHORUS_LED_RED, HIGH);
        break;
    }
  } else {
    switch (lowerData[P_chorus]) {
      case 0:
        if (announce) {
          showCurrentParameterPage("Chorus", "Off");
        }
        midiCCOutUpper(CCchorus, lowerData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, LOW);
        mcp8.digitalWrite(CHORUS_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCchorus, upperData[P_chorus]);
        }
        break;

      case 1:
        if (announce) {
          showCurrentParameterPage("Chorus", "1");
        }
        midiCCOutUpper(CCchorus, lowerData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, LOW);
        mcp8.digitalWrite(CHORUS_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCchorus, upperData[P_chorus]);
        }
        break;

      case 2:
        if (announce) {
          showCurrentParameterPage("Chorus", "2");
        }
        midiCCOutUpper(CCchorus, upperData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, HIGH);
        mcp8.digitalWrite(CHORUS_LED_RED, LOW);
        if (wholemode) {
          midiCCOutUpper(CCchorus, upperData[P_chorus]);
        }
        break;

      case 3:
        if (announce) {
          showCurrentParameterPage("Chorus", "1 & 2");
        }
        midiCCOutUpper(CCchorus, upperData[P_chorus]);
        mcp8.digitalWrite(CHORUS_LED_GRN, HIGH);
        mcp8.digitalWrite(CHORUS_LED_RED, HIGH);
        if (wholemode) {
          midiCCOutUpper(CCchorus, upperData[P_chorus]);
        }
        break;
    }
  }
}

void updatesyncSW(boolean announce) {
  if (upperSW) {
    if (!upperData[P_syncSW]) {
      if (announce) {
        showCurrentParameterPage("Sync", "Off");
      }
      midiCCOut(CCvco2Sync, 0);
      midiCCOutUpper(CCvco2Sync, 0);
      mcp3.digitalWrite(VCO2_SYNC_LED, LOW);
    } else {
      if (announce) {
        showCurrentParameterPage("Sync", "On");
      }
      midiCCOut(CCvco2Sync, 1);
      midiCCOutUpper(CCvco2Sync, 1);
      mcp3.digitalWrite(VCO2_SYNC_LED, HIGH);
    }
  } else {
    if (!lowerData[P_syncSW]) {
      if (announce) {
        showCurrentParameterPage("Sync", "Off");
      }
      midiCCOut(CCvco2Sync, 0);
      midiCCOutLower(CCvco2Sync, 0);
      mcp3.digitalWrite(VCO2_SYNC_LED, LOW);
      if (wholemode) {
        midiCCOutUpper(CCvco2Sync, 0);
      }
    } else {
      if (announce) {
        showCurrentParameterPage("Sync", "On");
      }
      midiCCOut(CCvco2Sync, 1);
      midiCCOutLower(CCvco2Sync, 1);
      mcp3.digitalWrite(VCO2_SYNC_LED, HIGH);
      if (wholemode) {
        midiCCOutUpper(CCvco2Sync, 1);
      }
    }
  }
}

void updatevcoModSW(boolean announce) {
  if (upperSW) {
    if (!upperData[P_vcoModSW]) {
      if (announce) {
        showCurrentParameterPage("VCO ModWheel", "Off");
      }
      midiCCOut(CCvcoModSW, upperData[P_vcoModSW]);

      mcp1.digitalWrite(VCO_LFO_LED, LOW);
    } else {
      if (announce) {
        showCurrentParameterPage("VCO ModWheel", "On");
      }
      midiCCOut(CCvcoModSW, upperData[P_vcoModSW]);

      mcp1.digitalWrite(VCO_LFO_LED, HIGH);
    }
  } else {
    if (!lowerData[P_vcoModSW]) {
      if (announce) {
        showCurrentParameterPage("VCO ModWheel", "Off");
      }
      midiCCOut(CCvcoModSW, lowerData[P_vcoModSW]);

      mcp1.digitalWrite(VCO_LFO_LED, LOW);
      if (wholemode) {

      }
    } else {
      if (announce) {
        showCurrentParameterPage("VCO ModWheel", "On");
      }
      midiCCOut(CCvcoModSW, lowerData[P_vcoModSW]);

      mcp1.digitalWrite(VCO_LFO_LED, HIGH);
      if (wholemode) {

      }
    }
  }
}

void updatevcfModSW(boolean announce) {
  if (upperSW) {
    if (!upperData[P_vcfModSW]) {
      if (announce) {
        showCurrentParameterPage("VCF ModWheel", "Off");
      }
      midiCCOut(CCvcfModSW, upperData[P_vcfModSW]);

      mcp1.digitalWrite(VCF_LFO_LED, LOW);
    } else {
      if (announce) {
        showCurrentParameterPage("VCF ModWheel", "On");
      }
      midiCCOut(CCvcfModSW, upperData[P_vcfModSW]);

      mcp1.digitalWrite(VCF_LFO_LED, HIGH);
    }
  } else {
    if (!lowerData[P_vcfModSW]) {
      if (announce) {
        showCurrentParameterPage("VCF ModWheel", "Off");
      }
      midiCCOut(CCvcfModSW, lowerData[P_vcfModSW]);

      mcp1.digitalWrite(VCF_LFO_LED, LOW);
      if (wholemode) {

      }
    } else {
      if (announce) {
        showCurrentParameterPage("VCF ModWheel", "On");
      }
      midiCCOut(CCvcfModSW, lowerData[P_vcfModSW]);

      mcp1.digitalWrite(VCF_LFO_LED, HIGH);
      if (wholemode) {

      }
    }
  }
}

void updatevco2RangeSW(boolean announce) {
  if (upperSW) {
    if (!upperData[P_vco2RangeSW]) {
      if (announce) {
        showCurrentParameterPage("VCO2 Range", "Normal");
      }
      midiCCOut(CCvco2RangeSW, 0);
      if (upperData[P_vco2Waveform] > 2) {
        upperData[P_vco2Waveform] = (upperData[P_vco2Waveform] - 3 );
        updatevco2Waveform(0);
      }
      mcp3.digitalWrite(VCO2_RANGE_LED, LOW);
    } else {
      if (announce) {
        showCurrentParameterPage("VCO2 Range", "Low");
      }
      midiCCOut(CCvco2RangeSW, 1);
      if (upperData[P_vco2Waveform] < 3) {
        upperData[P_vco2Waveform] = (upperData[P_vco2Waveform] + 3 );
        updatevco2Waveform(0);
      }
      mcp3.digitalWrite(VCO2_RANGE_LED, HIGH);
    }
  } else {
    if (!lowerData[P_vco2RangeSW]) {
      if (announce) {
        showCurrentParameterPage("VCO2 Range", "Normal");
      }
      midiCCOut(CCvco2RangeSW, 0);
      if (lowerData[P_vco2Waveform] > 2) {
        lowerData[P_vco2Waveform] = (lowerData[P_vco2Waveform] - 3 );
        updatevco2Waveform(0);
      }
      mcp3.digitalWrite(VCO2_RANGE_LED, LOW);
    } else {
      if (announce) {
        showCurrentParameterPage("VCO2 Range", "Low");
      }
      midiCCOut(CCvco2RangeSW, 1);
      if (lowerData[P_vco2Waveform] < 3) {
        lowerData[P_vco2Waveform] = (lowerData[P_vco2Waveform] + 3 );
        updatevco2Waveform(0);
      }
      mcp3.digitalWrite(VCO2_RANGE_LED, HIGH);
    }
  }
}

void updatevcfSlopeSW(boolean announce) {
  if (upperSW) {
    if (!upperData[P_vcfSlopeSW]) {
      if (announce) {
        showCurrentParameterPage("VCF Slope", "24 dB");
      }
      midiCCOut(CCvcfSlopeSW, 0);
      midiCCOutUpper(CCvcfSlopeSW, upperData[P_vcfSlopeSW]);
      mcp7.digitalWrite(VCF_SLOPE_LED_RED, LOW);
      mcp7.digitalWrite(VCF_SLOPE_LED_GRN, HIGH);
    } else {
      if (announce) {
        showCurrentParameterPage("VCF Slope", "12 dB");
      }
      midiCCOut(CCvcfSlopeSW, 1);
      midiCCOutUpper(CCvcfSlopeSW, upperData[P_vcfSlopeSW]);
      mcp7.digitalWrite(VCF_SLOPE_LED_RED, HIGH);
      mcp7.digitalWrite(VCF_SLOPE_LED_GRN, LOW);
    }
  } else {
    if (!lowerData[P_vcfSlopeSW]) {
      if (announce) {
        showCurrentParameterPage("VCF Slope", "24 dB");
      }
      midiCCOut(CCvcfSlopeSW, 0);
      midiCCOutLower(CCvcfSlopeSW, lowerData[P_vcfSlopeSW]);
      mcp7.digitalWrite(VCF_SLOPE_LED_RED, LOW);
      mcp7.digitalWrite(VCF_SLOPE_LED_GRN, HIGH);
      if (wholemode) {
        midiCCOutUpper(CCvcfSlopeSW, upperData[P_vcfSlopeSW]);
      }
    } else {
      if (announce) {
        showCurrentParameterPage("VCF Slope", "12 dB");
      }
      midiCCOut(CCvcfSlopeSW, 1);
      midiCCOutLower(CCvcfSlopeSW, lowerData[P_vcfSlopeSW]);
      mcp7.digitalWrite(VCF_SLOPE_LED_RED, HIGH);
      mcp7.digitalWrite(VCF_SLOPE_LED_GRN, LOW);
      if (wholemode) {
        midiCCOutUpper(CCvcfSlopeSW, upperData[P_vcfSlopeSW]);
      }
    }
  }
}

void updatevcfEgSelectSW(boolean announce) {
  if (upperSW) {
    if (!upperData[P_vcfEgSelectSW]) {
      if (announce) {
        showCurrentParameterPage("VCF Env Src", "ENV2");
      }
      midiCCOut(CCvcfEgSelectSW, 0);
      midiCCOutUpper(CCvcfEgSelectSW, upperData[P_vcfEgSelectSW]);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_RED, LOW);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_GRN, HIGH);
    } else {
      if (announce) {
        showCurrentParameterPage("VCF Env Src", "ENV1");
      }
      midiCCOut(CCvcfEgSelectSW, 1);
      midiCCOutUpper(CCvcfEgSelectSW, upperData[P_vcfEgSelectSW]);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_RED, HIGH);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_GRN, LOW);
    }
  } else {
    if (!lowerData[P_vcfEgSelectSW]) {
      if (announce) {
        showCurrentParameterPage("VCF Env Src", "ENV2");
      }
      midiCCOut(CCvcfEgSelectSW, 0);
      midiCCOutLower(CCvcfEgSelectSW, lowerData[P_vcfEgSelectSW]);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_RED, LOW);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_GRN, HIGH);
      if (wholemode) {
        midiCCOutUpper(CCvcfEgSelectSW, upperData[P_vcfEgSelectSW]);
      }
    } else {
      if (announce) {
        showCurrentParameterPage("VCF Env Src", "ENV1");
      }
      midiCCOut(CCvcfEgSelectSW, 1);
      midiCCOutLower(CCvcfEgSelectSW, lowerData[P_vcfEgSelectSW]);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_RED, HIGH);
      mcp7.digitalWrite(VCF_ENV_SRC_LED_GRN, LOW);
      if (wholemode) {
        midiCCOutUpper(CCvcfEgSelectSW, upperData[P_vcfEgSelectSW]);
      }
    }
  }
}

void updatePatchname() {
  refreshPatchDisplayFromState();
}

void myControlChange(byte channel, byte control, int value) {

  switch (control) {

    case CCdual_button:
      updatedual_button(1);
      break;

    case CCsplit_button:
      updatesplit_button(1);
      break;

    case CCwhole_button:
      updatewhole_button(1);
      break;

    case CCglideTime:
      if (upperSW) {
        upperData[P_glideTime] = value;
      } else {
        lowerData[P_glideTime] = value;
        if (wholemode) {
          upperData[P_glideTime] = value;
        }
      }
      glideTimestr = LINEAR[value];
      updateglideTime(1);
      break;

    case CCvco2Fine:
      if (upperSW) {
        upperData[P_vco2Fine] = value;
      } else {
        lowerData[P_vco2Fine] = value;
        if (wholemode) {
          upperData[P_vco2Fine] = value;
        }
      }
      // Display mapping: 0–127 → -63…+63 (center = 0)
      fineDisp = (int16_t)value - 64;
      fineDisp = constrain(fineDisp, -63, 63);

      vco2Finestr = fineDisp;
      updatevco2Fine(1);
      break;

    case CCvcoBalance:
      if (upperSW) {
        upperData[P_vcoBalance] = value;
      } else {
        lowerData[P_vcoBalance] = value;
        if (wholemode) {
          upperData[P_vcoBalance] = value;
        }
      }
      // Display mapping: 0–127 → -63…+63 (center = 0)
      fineDisp = (int16_t)value - 64;
      fineDisp = constrain(fineDisp, -63, 63);

      vcoBalancestr = fineDisp;
      updatevcoBalance(1);
      break;

    case CCHPF:
      if (upperSW) {
        upperData[P_HPF] = value;
      } else {
        lowerData[P_HPF] = value;
        if (wholemode) {
          upperData[P_HPF] = value;
        }
      }
      HPFstr = value;
      updateHPF(1);
      break;

    case CCcrossMod:
      if (upperSW) {
        upperData[P_crossMod] = value;
      } else {
        lowerData[P_crossMod] = value;
        if (wholemode) {
          upperData[P_crossMod] = value;
        }
      }
      crossModstr = value;
      updatecrossMod(1);
      break;

    case CCPWMMod:
      if (upperSW) {
        upperData[P_PWMMod] = value;
      } else {
        lowerData[P_PWMMod] = value;
        if (wholemode) {
          upperData[P_PWMMod] = value;
        }
      }
      PWMModstr = value;
      updatePWMMod(1);
      break;

    case CClfoDelay:
      if (upperSW) {
        upperData[P_lfoDelay] = value;
      } else {
        lowerData[P_lfoDelay] = value;
        if (wholemode) {
          upperData[P_lfoDelay] = value;
        }
      }
      lfoDelaystr = value;  // for display
      updatelfoDelay(1);
      break;

    case CCvcoLfoMod:
      if (upperSW) {
        upperData[P_vcoLfoMod] = value;
      } else {
        lowerData[P_vcoLfoMod] = value;
        if (wholemode) {
          upperData[P_vcoLfoMod] = value;
        }
      }
      vcoLfoModstr = value;  // for display
      updatevcoLfoMod(1);
      break;

    case CCvcoEnvMod:
      if (upperSW) {
        upperData[P_vcoEnvMod] = value;
      } else {
        lowerData[P_vcoEnvMod] = value;
        if (wholemode) {
          upperData[P_vcoEnvMod] = value;
        }
      }
      vcoEnvModstr = value;  // for display
      updatevcoEnvMod(1);
      break;

    case CCfilterCutoff:
      if (upperSW) {
        upperData[P_filterCutoff] = value;
        oldfilterCutoffU = value;
      } else {
        lowerData[P_filterCutoff] = value;
        oldfilterCutoffL = value;
        if (wholemode) {
          upperData[P_filterCutoff] = value;
          oldfilterCutoffU = value;
        }
      }
      filterCutoffstr = FILTERCUTOFF[value];
      updateFilterCutoff(1);
      break;

    case CCvcfLfoDepth:
      if (upperSW) {
        upperData[P_vcfLfoDepth] = value;
      } else {
        lowerData[P_vcfLfoDepth] = value;
        if (wholemode) {
          upperData[P_vcfLfoDepth] = value;
        }
      }
      vcfLfoDepthstr = value;
      updatevcfLfoDepth(1);
      break;

    case CCresonance:
      if (upperSW) {
        upperData[P_resonance] = value;
      } else {
        lowerData[P_resonance] = value;
        if (wholemode) {
          upperData[P_resonance] = value;
        }
      }
      resonancestr = value;
      updateresonance(1);
      break;

    case CCvcfEnvDepth:
      if (upperSW) {
        upperData[P_vcfEnvDepth] = value;
      } else {
        lowerData[P_vcfEnvDepth] = value;
        if (wholemode) {
          upperData[P_vcfEnvDepth] = value;
        }
      }
      vcfEnvDepthstr = int(value);
      updatevcfEnvDepth(1);
      break;

    case CCvcfKeyFollow:
      if (upperSW) {
        upperData[P_vcfKeyFollow] = value;
      } else {
        lowerData[P_vcfKeyFollow] = value;
        if (wholemode) {
          upperData[P_vcfKeyFollow] = value;
        }
      }
      vcfKeyFollowstr = map(value, 0, 127, 0, 120);  // for display
      updatevcfKeyFollow(1);
      break;

    case CCvcaLevel:
      if (upperSW) {
        upperData[P_vcaLevel] = value;
      } else {
        lowerData[P_vcaLevel] = value;
        if (wholemode) {
          upperData[P_vcaLevel] = value;
        }
      }
      vcaLevelstr = value;  // for display
      updatevcaLevel(1);
      break;

    case CCdelayLevel:
      value = map(value, 0, 127, 0, 15);
      if (upperSW) {
        upperData[P_delayLevel] = value;
      } else {
        lowerData[P_delayLevel] = value;
        if (wholemode) {
          upperData[P_delayLevel] = value;
        }
      }
      delayLevelstr = value;  // for display
      updatedelayLevel(1);
      break;

    case CCbendRange:
      value = map(value, 0, 127, 0, 12);
      if (upperSW) {
        upperData[P_vcoBendRange] = value;
      } else {
        lowerData[P_vcoBendRange] = value;
        if (wholemode) {
          upperData[P_vcoBendRange] = value;
        }
      }
      bendRangestr = value;  // for display
      updatebendRange(1);
      break;

    case CCdelayTime:
      value = map(value, 0, 127, 0, 15);
      if (upperSW) {
        upperData[P_delayTime] = value;
      } else {
        lowerData[P_delayTime] = value;
        if (wholemode) {
          upperData[P_delayTime] = value;
        }
      }
      delayTimestr = value;  // for display
      updatedelayTime(1);
      break;

    case CCdelayFeedback:
      value = map(value, 0, 127, 0, 15);
      if (upperSW) {
        upperData[P_delayFeedback] = value;
      } else {
        lowerData[P_delayFeedback] = value;
        if (wholemode) {
          upperData[P_delayFeedback] = value;
        }
      }
      delayFeedbackstr = value;  // for display
      updatedelayFeedback(1);
      break;

    case CClfoRate:
      if (upperSW) {
        upperData[P_lfoRate] = value;
      } else {
        lowerData[P_lfoRate] = value;
        if (wholemode) {
          upperData[P_lfoRate] = value;
        }
      }
      LFORatestr = LFOTEMPO[value];  // for display
      updateLFORate(1);
      break;

    case CCarpRate:
      if (upperSW) {
        upperData[P_arpRate] = value;
      } else {
        lowerData[P_arpRate] = value;
        if (wholemode) {
          upperData[P_arpRate] = value;
        }
      }
      arpRatestr = LFOTEMPO[value];  // for display
      updatearpRate(1);
      break;

    case CCenv1Attack:
      if (upperSW) {
        upperData[P_env1Attack] = value;
      } else {
        lowerData[P_env1Attack] = value;
        if (wholemode) {
          upperData[P_env1Attack] = value;
        }
      }
      env1Attackstr = ENVTIMES[value];
      updateenv1Attack(1);
      break;

    case CCenv1Decay:
      if (upperSW) {
        upperData[P_env1Decay] = value;
      } else {
        lowerData[P_env1Decay] = value;
        if (wholemode) {
          upperData[P_env1Decay] = value;
        }
      }
      env1Decaystr = ENVTIMES[value];
      updateenv1Decay(1);
      break;

    case CCenv1Sustain:
      if (upperSW) {
        upperData[P_env1Sustain] = value;
      } else {
        lowerData[P_env1Sustain] = value;
        if (wholemode) {
          upperData[P_env1Sustain] = value;
        }
      }
      env1Sustainstr = LINEAR_FILTERMIXERSTR[value];
      updateenv1Sustain(1);
      break;

    case CCenv1Release:
      if (upperSW) {
        upperData[P_env1Release] = value;
      } else {
        lowerData[P_env1Release] = value;
        if (wholemode) {
          upperData[P_env1Release] = value;
        }
      }
      env1Releasestr = ENVTIMES[value];
      updateenv1Release(1);
      break;

    case CCenv2Attack:
      if (upperSW) {
        upperData[P_env2Attack] = value;
      } else {
        lowerData[P_env2Attack] = value;
        if (wholemode) {
          upperData[P_env2Attack] = value;
        }
      }
      env2Attackstr = ENVTIMES[value];
      updateenv2Attack(1);
      break;

    case CCenv2Decay:
      if (upperSW) {
        upperData[P_env2Decay] = value;
      } else {
        lowerData[P_env2Decay] = value;
        if (wholemode) {
          upperData[P_env2Decay] = value;
        }
      }
      env2Decaystr = ENVTIMES[value];
      updateenv2Decay(1);
      break;

    case CCenv2Sustain:
      if (upperSW) {
        upperData[P_env2Sustain] = value;
      } else {
        lowerData[P_env2Sustain] = value;
        if (wholemode) {
          upperData[P_env2Sustain] = value;
        }
      }
      env2Sustainstr = LINEAR_FILTERMIXERSTR[value];
      updateenv2Sustain(1);
      break;

    case CCenv2Release:
      if (upperSW) {
        upperData[P_env2Release] = value;
      } else {
        lowerData[P_env2Release] = value;
        if (wholemode) {
          upperData[P_env2Release] = value;
        }
      }
      env2Releasestr = ENVTIMES[value];
      updateenv2Release(1);
      break;

    case CCvolume:
      upperData[P_volume] = value;
      lowerData[P_volume] = value;
      // logic to update DAC here
      volumestr = value;
      updatevolume(1);
      break;

    case CCbalance:
      upperData[P_balance] = value;
      lowerData[P_balance] = value;
      // logic to update DAC here
      balancestr = value;
      updatebalance(1);
      break;

      //   //   ////////////////////////////////////////////////

      // case CCplayMode:
      //   updateplayMode(1);
      //   break;

      // case CCNotePriority:
      //   if (upperData[P_keyboardMode] >= 2) {
      //     if (upperSW) {
      //       upperData[P_NotePriority] = value;
      //     }
      //     updateNotePriority(1);
      //   }
      //   if (lowerData[P_keyboardMode] >= 2) {
      //     if (lowerSW) {
      //       lowerData[P_NotePriority] = value;
      //     }
      //     updateNotePriority(1);
      //   }
      //   break;

      // case CCkeyboardMode:
      //   if (upperSW) {
      //     upperData[P_keyboardMode] = panelData[P_keyboardMode];
      //   } else {
      //     lowerData[P_keyboardMode] = panelData[P_keyboardMode];
      //   }
      //   updatekeyboardMode(1);
      //   break;

    case CCglideSW:
      updateglideSW(1);
      break;

    case CCvcoBendSW:
      if (upperSW) {
        upperData[P_vcoBendSW] = upperData[P_vcoBendSW] + 1;
        if (upperData[P_vcoBendSW] > 2) {
          upperData[P_vcoBendSW] = 0;
        }
      } else {
        lowerData[P_vcoBendSW] = lowerData[P_vcoBendSW] + 1;
        if (lowerData[P_vcoBendSW] > 2) {
          lowerData[P_vcoBendSW] = 0;
        }
        if (wholemode) {
          upperData[P_vcoBendSW] = lowerData[P_vcoBendSW];
        }
      }
      updatevcoBendSW(1);
      break;

    case CCvcoModSW:
      if (upperSW) {
        upperData[P_vcoModSW] = !upperData[P_vcoModSW];
      } else {
        lowerData[P_vcoModSW] = !lowerData[P_vcoModSW];
        if (wholemode) {
          upperData[P_vcoModSW] = lowerData[P_vcoModSW];
        }
      }
      updatevcoModSW(1);
      break;

    case CCvcfModSW:
      if (upperSW) {
        upperData[P_vcfModSW] = !upperData[P_vcfModSW];
      } else {
        lowerData[P_vcfModSW] = !lowerData[P_vcfModSW];
        if (wholemode) {
          upperData[P_vcfModSW] = lowerData[P_vcfModSW];
        }
      }
      updatevcfModSW(1);
      break;

    case CCvco2Sync:
      if (upperSW) {
        upperData[P_syncSW] = !upperData[P_syncSW];
      } else {
        lowerData[P_syncSW] = !lowerData[P_syncSW];
        if (wholemode) {
          upperData[P_syncSW] = lowerData[P_syncSW];
        }
      }
      updatesyncSW(1);
      break;

    case CCenv1InvertSW:
      if (upperSW) {
        upperData[P_env1InvertSW] = !upperData[P_env1InvertSW];
      } else {
        lowerData[P_env1InvertSW] = !lowerData[P_env1InvertSW];
        if (wholemode) {
          upperData[P_env1InvertSW] = lowerData[P_env1InvertSW];
        } 
      }
      updateenv1InvertSW(1);
      break;

    case CCenv2KeyFollowSW:
      if (upperSW) {
        upperData[P_env2KeyFollowSW] = upperData[P_env2KeyFollowSW] + 1;
        if (upperData[P_env2KeyFollowSW] > 3) {
          upperData[P_env2KeyFollowSW] = 0;
        }
      } else {
        lowerData[P_env2KeyFollowSW] = lowerData[P_env2KeyFollowSW] + 1;
        if (lowerData[P_env2KeyFollowSW] > 3) {
          lowerData[P_env2KeyFollowSW] = 0;
        }
        if (wholemode) {
          upperData[P_env2KeyFollowSW] = lowerData[P_env2KeyFollowSW];
        } 
      }
      updateenv2KeyFollowSW(1);
      break;

    case CCchorus:
      if (upperSW) {
        upperData[P_chorus] = upperData[P_chorus] + 1;
        if (upperData[P_chorus] > 3) {
          upperData[P_chorus] = 0;
        }
      } else {
        lowerData[P_chorus] = lowerData[P_chorus] + 1;
        if (lowerData[P_chorus] > 3) {
          lowerData[P_chorus] = 0;
        }
        if (wholemode) {
          upperData[P_chorus] = lowerData[P_chorus];
        } 
      }
      updatechorus(1);
      break;

    case CCvcfSlopeSW:
      if (upperSW) {
        upperData[P_vcfSlopeSW] = !upperData[P_vcfSlopeSW];
      } else {
        lowerData[P_vcfSlopeSW] = !lowerData[P_vcfSlopeSW];
        if (wholemode) {
          upperData[P_vcfSlopeSW] = lowerData[P_vcfSlopeSW];
        } 
      }
      updatevcfSlopeSW(1);
      break;

    case CCvcfEgSelectSW:
      if (upperSW) {
        upperData[P_vcfEgSelectSW] = !upperData[P_vcfEgSelectSW];
      } else {
        lowerData[P_vcfEgSelectSW] = !lowerData[P_vcfEgSelectSW];
        if (wholemode) {
          upperData[P_vcfEgSelectSW] = lowerData[P_vcfEgSelectSW];
        } 
      }
      updatevcfEgSelectSW(1);
      break;

    case CCvco2RangeSW:
      if (upperSW) {
        upperData[P_vco2RangeSW] = !upperData[P_vco2RangeSW];
      } else {
        lowerData[P_vco2RangeSW] = !lowerData[P_vco2RangeSW];
        if (wholemode) {
          upperData[P_vco2RangeSW] = lowerData[P_vco2RangeSW];
        } 
      }
      updatevco2RangeSW(1);
      break;

    case CCvcoModSelSW:
      if (upperSW) {
        upperData[P_vcoModSelSW] = upperData[P_vcoModSelSW] + 1;
        if (upperData[P_vcoModSelSW] > 2) {
          upperData[P_vcoModSelSW] = 0;
        }
      } else {
        lowerData[P_vcoModSelSW] = lowerData[P_vcoModSelSW] + 1;
        if (lowerData[P_vcoModSelSW] > 2) {
          lowerData[P_vcoModSelSW] = 0;
        }
        if (wholemode) {
          upperData[P_vcoModSelSW] = lowerData[P_vcoModSelSW];
        }       
      }
      updatevcoModSelSW(1);
      break;

    case CCPWMModSW:
      if (upperSW) {
        upperData[P_PWMModSW] = upperData[P_PWMModSW] + 1;
        if (upperData[P_PWMModSW] > 2) {
          upperData[P_PWMModSW] = 0;
        }
      } else {
        lowerData[P_PWMModSW] = lowerData[P_PWMModSW] + 1;
        if (lowerData[P_PWMModSW] > 2) {
          lowerData[P_PWMModSW] = 0;
        }
        if (wholemode) {
          upperData[P_PWMModSW] = lowerData[P_PWMModSW];
        }
      }
      updatePWMModSW(1);
      break;

    case CCvcaModSW:
      if (upperSW) {
        upperData[P_vcaModSW] = upperData[P_vcaModSW] + 1;
        if (upperData[P_vcaModSW] > 3) {
          upperData[P_vcaModSW] = 0;
        }
      } else {
        lowerData[P_vcaModSW] = lowerData[P_vcaModSW] + 1;
        if (lowerData[P_vcaModSW] > 3) {
          lowerData[P_vcaModSW] = 0;
        }
        if (wholemode) {
          upperData[P_vcaModSW] = lowerData[P_vcaModSW];
        }
      }
      updatevcaModSW(1);
      break;

    case CCvco1Range:
      value = map(value, 0, 127, 0, 5);
      if (upperSW) {
        upperData[P_vco1Range] = value;
      } else {
        lowerData[P_vco1Range] = value;
        if (wholemode) {
          upperData[P_vco1Range] = value;
        }
      }
      vco1RangeDisplay = value;
      updatevco1Range(1);
      break;

    case CCvco1Waveform:
      value = map(value, 0, 127, 0, 5);
      if (upperSW) {
        upperData[P_vco1Waveform] = value;
      } else {
        lowerData[P_vco1Waveform] = value;
        if (wholemode) {
          upperData[P_vco1Waveform] = value;
        }
      }
      vco1WaveformDisplay = value;
      updatevco1Waveform(1);
      break;

    case CCvco2Range:
      if (upperSW) {
        upperData[P_vco2Range] = value;
      } else {
        lowerData[P_vco2Range] = value;
        if (wholemode) {
          upperData[P_vco2Range] = value;
        }
      }
      vco2RangeDisplay = value;
      updatevco2Range(1);
      break;

    case CCvco2Waveform:
      value = map(value, 0, 127, 0, 5);
      if (upperSW) {
        upperData[P_vco2Waveform] = value;
      } else {
        lowerData[P_vco2Waveform] = value;
        if (wholemode) {
          upperData[P_vco2Waveform] = value;
        }
      }
      vco2WaveformDisplay = value;
      updatevco2Waveform(1);
      break;

    case CClfoWaveform:
      value = map(value, 0, 127, 0, 5);
      if (upperSW) {
        upperData[P_lfoWaveform] = value;
      } else {
        lowerData[P_lfoWaveform] = value;
        if (wholemode) {
          upperData[P_lfoWaveform] = value;
        }
      }
      lfoWaveformDisplay = value;
      updatelfoWaveform(1);
      break;

    case CCupperSW:
      updateupperSW(1);
      break;

    case CClowerSW:
      updatelowerSW(1);
      break;

    case CCallnotesoff:
      allNotesOff();
      break;
  }
}

void myProgramChange(byte channel, byte program) {
  if (inPerformanceMode) {
    if (program < performances.size()) {
      performanceIndex = program;
      currentPerformance = performances[performanceIndex];

      // Update playmode and patch indices
      playMode = currentPerformance.mode;
      wholemode = (playMode == WHOLE);
      updateplayMode(0);

      // Set patch indices
      for (int i = 0; i < patches.size(); i++) {
        if (patches[i].patchNo == currentPerformance.upperPatchNo) upperPatchIndex = i;
        if (patches[i].patchNo == currentPerformance.lowerPatchNo) lowerPatchIndex = i;
      }

      // Recall both patches
      upperSW = true;
      recallPatch(currentPerformance.upperPatchNo);
      upperSW = false;
      recallPatch(currentPerformance.lowerPatchNo);

      refreshPatchDisplayFromState();
    }
  } else {
    // Normal patch recall
    state = PATCH;
    patchNo = program + 1;
    recallPatch(patchNo);
    state = PARAMETER;
  }
}

void myAfterTouch(byte channel, byte value) {

  afterTouch = value;
  afterTouchU = (afterTouch * upperData[P_ATDepth]) / 127;
  afterTouchL = (afterTouch * lowerData[P_ATDepth]) / 127;

  switch (upperData[P_AfterTouchDest]) {
    case 1:
      MIDI6.sendAfterTouch(value, 2);
      break;
    case 2:
      upperData[P_filterCutoff] = (oldfilterCutoffU + afterTouchU);
      if (afterTouchU < 10) {
        upperData[P_filterCutoff] = oldfilterCutoffU;
      }
      if (upperData[P_filterCutoff] > 127) {
        upperData[P_filterCutoff] = 127;
      }
      break;
    case 3:
      upperData[P_vcfLfoDepth] = afterTouchU;
      break;
  }
  switch (lowerData[P_AfterTouchDest]) {
    case 1:
      MIDI6.sendAfterTouch(value, 1);
      if (wholemode) {
        MIDI6.sendAfterTouch(value, 2);
      }
      break;
    case 2:
      lowerData[P_filterCutoff] = (oldfilterCutoffL + afterTouchL);
      if (afterTouchL < 10) {
        lowerData[P_filterCutoff] = oldfilterCutoffL;
      }
      if (lowerData[P_filterCutoff] > 127) {
        lowerData[P_filterCutoff] = 127;
      }
      break;
    case 3:
      lowerData[P_vcfLfoDepth] = afterTouchL;
      break;
  }
}

void recallPatch(int patchNo) {
  allNotesOff();

  File patchFile = SD.open(String(patchNo).c_str());
  if (!patchFile) {
    Serial.println("File not found");
  } else {
    String data[NO_OF_PARAMS];
    recallPatchData(patchFile, data);
    patchFile.close();

    // Find matching patch in the circular buffer to set name and number
    for (int i = 0; i < patches.size(); i++) {

      if (patches[i].patchNo == patchNo) {
        if (upperSW) {
          upperPatchIndex = i;
          currentPgmNumU = String(patches[i].patchNo);
          currentPatchNameU = patches[i].patchName;
          //storeLastPatchU(currentPgmNumU)
        } else {
          lowerPatchIndex = i;
          currentPgmNumL = String(patches[i].patchNo);
          currentPatchNameL = patches[i].patchName;
          //storeLastPatchL(currentPgmNumL)
        }

        break;
      }
    }

    setCurrentPatchData(data);
  }
}

void setCurrentPatchData(String data[]) {
  int tempData[75];  // Temporary array for converted integers

  // Convert data from String to int once
  for (int i = 1; i <= 74; i++) {
    tempData[i] = data[i].toInt();
  }

  if (upperSW) {
    patchNameU = data[0];
    tempData[0] = 1;
    memcpy(upperData, tempData, sizeof(tempData));

    oldfilterCutoffU = upperData[P_filterCutoff];
    upperParamsToDisplay();
    setAllButtons();
  } else {
    patchNameL = data[0];
    tempData[0] = 1;
    memcpy(lowerData, tempData, sizeof(tempData));

    oldfilterCutoffL = lowerData[P_filterCutoff];
    lowerParamsToDisplay();
    setAllButtons();

    if (wholemode) {

      // Update previous values and pick-up flags
      for (int i = 1; i <= 74; i++) {
        upperData[i] = lowerData[i];  // Store previous value
      }

      oldfilterCutoffU = upperData[P_filterCutoff];
      upperParamsToDisplay();
      setAllButtons();
    }
  }

  updatePatchname();
}

void upperParamsToDisplay() {

  updateglideTime(0);
  updatePWMMod(0);
  updatevco2Fine(0);
  updateFilterCutoff(0);
  updateresonance(0);
  updatevcfEnvDepth(0);
  updatevcfKeyFollow(0);
  updatevcfLfoDepth(0);
  updateenv1Attack(0);
  updateenv1Decay(0);
  updateenv1Sustain(0);
  updateenv1Release(0);
  updateenv2Attack(0);
  updateenv2Decay(0);
  updateenv2Sustain(0);
  updateenv2Release(0);
  updateLFORate(0);
  updatelfoDelay(0);
  updatevcoLfoMod(0);
  updatevcoEnvMod(0);
  updatebendRange(0);
  updatevolume(0);
  updatebalance(0);
  updateHPF(0);
  updatecrossMod(0);
  updatevco1Range(0);
  updatevco2Range(0);
  updatevco1Waveform(0);
  updatevco2Waveform(0);
  updatelfoWaveform(0);
}

void lowerParamsToDisplay() {

  updateglideTime(0);
  updatePWMMod(0);
  updatevco2Fine(0);
  updateFilterCutoff(0);
  updateresonance(0);
  updatevcfEnvDepth(0);
  updatevcfKeyFollow(0);
  updateenv1Attack(0);
  updateenv1Decay(0);
  updateenv1Sustain(0);
  updateenv1Release(0);
  updateenv2Attack(0);
  updateenv2Decay(0);
  updateenv2Sustain(0);
  updateenv2Release(0);
  updateLFORate(0);
  updatelfoDelay(0);
  updatevcoLfoMod(0);
  updatevcoEnvMod(0);
  updatebendRange(0);
  updatevolume(0);
  updatebalance(0);
  updateHPF(0);
  updatecrossMod(0);
  updatevco1Range(0);
  updatevco2Range(0);
  updatevco1Waveform(0);
  updatevco2Waveform(0);
  updatelfoWaveform(0);
}

void setAllButtons() {
  // updatekeyboardMode(0);
  updatevcaModSW(0);
  updateglideSW(0);
  updatevcoBendSW(0);
  updatesyncSW(0);
  updatevcoModSW(0);
  updatevcfModSW(0);
  updatevcoModSelSW(0);
  updatePWMModSW(0);
  updatevco2RangeSW(0);
  updatevcfSlopeSW(0);
  updatevcfEgSelectSW(0);
  updateenv1InvertSW(0);
  updateenv2KeyFollowSW(0);
  updatechorus(0);
}

String getCurrentPatchData() {
  if (upperSW) {
    return patchNameU + "," + String(upperData[P_vcoBendRange]) + "," + String(upperData[P_vcfBendRange]) + "," + String(upperData[P_vcoLfoModDepth]) + "," + String(upperData[P_vcfLfoModDepth])
           + "," + String(upperData[P_glideTime]) + "," + String(upperData[P_balance]) + "," + String(upperData[P_volume]) + "," + String(upperData[P_arpRate]) + "," + String(upperData[P_lfoRate])
           + "," + String(upperData[P_lfoDelay]) + "," + String(upperData[P_lfoWaveform]) + "," + String(upperData[P_vcoLfoMod]) + "," + String(upperData[P_vcoEnvMod])
           + "," + String(upperData[P_PWMMod]) + "," + String(upperData[P_crossMod]) + "," + String(upperData[P_vco1Range]) + "," + String(upperData[P_vco1Waveform])
           + "," + String(upperData[P_vco2Range]) + "," + String(upperData[P_vco2Fine]) + "," + String(upperData[P_vco2Waveform]) + "," + String(upperData[P_vcoBalance])
           + "," + String(upperData[P_HPF]) + "," + String(upperData[P_filterCutoff]) + "," + String(upperData[P_resonance]) + "," + String(upperData[P_vcfEnvDepth])
           + "," + String(upperData[P_vcfLfoDepth]) + "," + String(upperData[P_vcfKeyFollow]) + "," + String(upperData[P_vcaLevel]) + "," + String(upperData[P_env1Attack])
           + "," + String(upperData[P_env1Decay]) + "," + String(upperData[P_env1Sustain]) + "," + String(upperData[P_env1Release]) + "," + String(upperData[P_env2Attack])
           + "," + String(upperData[P_env2Decay]) + "," + String(upperData[P_env2Sustain]) + "," + String(upperData[P_env2Release]) + "," + String(upperData[P_delayLevel])
           + "," + String(upperData[P_delayTime]) + "," + String(upperData[P_delayFeedback]) + "," + String(upperData[P_vcoBendSW]) + "," + String(upperData[P_vcoModSW])
           + "," + String(upperData[P_glideSW]) + "," + String(upperData[P_arpSW]) + "," + String(upperData[P_vcoModSelSW]) + "," + String(upperData[P_PWMModSW]) + "," + String(upperData[P_syncSW])
           + "," + String(upperData[P_vco2RangeSW]) + "," + String(upperData[P_vcfSlopeSW]) + "," + String(upperData[P_vcfEgSelectSW]) + "," + String(upperData[P_vcaModSW])
           + "," + String(upperData[P_env1InvertSW]) + "," + String(upperData[P_env2KeyFollowSW]) + "," + String(upperData[P_keyboardModeSW]) + "," + String(upperData[P_assignModeSW]) + "," + String(upperData[P_arpRangeSW])
           + "," + String(upperData[P_arpModeSW]) + "," + String(upperData[P_AfterTouchDest]) + "," + String(upperData[P_ATDepth]) + "," + String(upperData[P_chorus]) + "," + String(upperData[P_vcfModSW]);
  } else {
    return patchNameL + "," + String(lowerData[P_vcoBendRange]) + "," + String(lowerData[P_vcfBendRange]) + "," + String(lowerData[P_vcoLfoModDepth]) + "," + String(lowerData[P_vcfLfoModDepth])
           + "," + String(lowerData[P_glideTime]) + "," + String(lowerData[P_balance]) + "," + String(lowerData[P_volume]) + "," + String(lowerData[P_arpRate]) + "," + String(lowerData[P_lfoRate])
           + "," + String(lowerData[P_lfoDelay]) + "," + String(lowerData[P_lfoWaveform]) + "," + String(lowerData[P_vcoLfoMod]) + "," + String(lowerData[P_vcoEnvMod])
           + "," + String(lowerData[P_PWMMod]) + "," + String(lowerData[P_crossMod]) + "," + String(lowerData[P_vco1Range]) + "," + String(lowerData[P_vco1Waveform])
           + "," + String(lowerData[P_vco2Range]) + "," + String(lowerData[P_vco2Fine]) + "," + String(lowerData[P_vco2Waveform]) + "," + String(lowerData[P_vcoBalance])
           + "," + String(lowerData[P_HPF]) + "," + String(lowerData[P_filterCutoff]) + "," + String(lowerData[P_resonance]) + "," + String(lowerData[P_vcfEnvDepth])
           + "," + String(lowerData[P_vcfLfoDepth]) + "," + String(lowerData[P_vcfKeyFollow]) + "," + String(lowerData[P_vcaLevel]) + "," + String(lowerData[P_env1Attack])
           + "," + String(lowerData[P_env1Decay]) + "," + String(lowerData[P_env1Sustain]) + "," + String(lowerData[P_env1Release]) + "," + String(lowerData[P_env2Attack])
           + "," + String(lowerData[P_env2Decay]) + "," + String(lowerData[P_env2Sustain]) + "," + String(lowerData[P_env2Release]) + "," + String(lowerData[P_delayLevel])
           + "," + String(lowerData[P_delayTime]) + "," + String(lowerData[P_delayFeedback]) + "," + String(lowerData[P_vcoBendSW]) + "," + String(lowerData[P_vcoModSW])
           + "," + String(lowerData[P_glideSW]) + "," + String(lowerData[P_arpSW]) + "," + String(lowerData[P_vcoModSelSW]) + "," + String(lowerData[P_PWMModSW]) + "," + String(lowerData[P_syncSW])
           + "," + String(lowerData[P_vco2RangeSW]) + "," + String(lowerData[P_vcfSlopeSW]) + "," + String(lowerData[P_vcfEgSelectSW]) + "," + String(lowerData[P_vcaModSW])
           + "," + String(lowerData[P_env1InvertSW]) + "," + String(lowerData[P_env2KeyFollowSW]) + "," + String(lowerData[P_keyboardModeSW]) + "," + String(lowerData[P_assignModeSW]) + "," + String(lowerData[P_arpRangeSW])
           + "," + String(lowerData[P_arpModeSW]) + "," + String(lowerData[P_AfterTouchDest]) + "," + String(lowerData[P_ATDepth]) + "," + String(lowerData[P_chorus]) + "," + String(lowerData[P_vcfModSW]);
  }
}

void midiCCOut(byte cc, byte value) {
  MIDI.sendControlChange(cc, value, midiChannel);  //MIDI DIN main out
}

void midiCCOutUpper(byte cc, byte value) {
  MIDI7.sendControlChange(cc, value, 1);  //MIDI DIN to synth board upper
}

void midiCCOutLower(byte cc, byte value) {
  MIDI6.sendControlChange(cc, value, 1);  //MIDI DIN to synth board lower
}


void outputDAC(int CHIP_SELECT, uint32_t sample_data1, uint32_t sample_data2, uint32_t sample_data3, uint32_t sample_data4) {
  SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE1));
  digitalWriteFast(CHIP_SELECT, LOW);
  SPI.transfer32(sample_data1);
  digitalWriteFast(CHIP_SELECT, HIGH);
  digitalWriteFast(CHIP_SELECT, LOW);
  SPI.transfer32(sample_data2);
  digitalWriteFast(CHIP_SELECT, HIGH);
  digitalWriteFast(CHIP_SELECT, LOW);
  SPI.transfer32(sample_data3);
  digitalWriteFast(CHIP_SELECT, HIGH);
  digitalWriteFast(CHIP_SELECT, LOW);
  SPI.transfer32(sample_data4);
  digitalWriteFast(CHIP_SELECT, HIGH);
  SPI.endTransaction();
  delayMicroseconds(2);
}

void showSettingsPage() {
  showSettingsPage(settings::current_setting(), settings::current_setting_value(), state);
}

void showPerformancePage(String perfNum, String name, int upperNo, String upperName, int lowerNo, String lowerName) {
  currentPerfNum = perfNum;
  currentPerfName = name;
  currentUpperPatchNo = upperNo;
  currentUpperPatchName = upperName;
  currentLowerPatchNo = lowerNo;
  currentLowerPatchName = lowerName;
}

void reinitialiseToPanel() {
  if (upperSW) {
    for (int i = 1; i < 77; i++) {
      upperData[i] = 0;
    }
    // upperData[P_osc1SawLevel] = 127;
    // upperData[P_osc2SawLevel] = 127;
    // upperData[P_osc2Detune] = 8;
    // upperData[P_filterCutoff] = 127;
    // upperData[P_env2Sustain] = 127;
    // upperData[P_volumeControl] = 127;
    // upperData[P_noiseLevel] = 63;
    // upperData[P_osc1PW] = 63;
    // upperData[P_osc2PW] = 63;
    upperParamsToDisplay();
    setAllButtons();
  } else {
    for (int i = 1; i < 77; i++) {
      lowerData[i] = 0;
    }
    // lowerData[P_osc1SawLevel] = 127;
    // lowerData[P_osc2SawLevel] = 127;
    // lowerData[P_osc2Detune] = 8;
    // lowerData[P_filterCutoff] = 127;
    // lowerData[P_env2Sustain] = 127;
    // lowerData[P_volumeControl] = 127;
    // lowerData[P_noiseLevel] = 63;
    // lowerData[P_osc1PW] = 63;
    // lowerData[P_osc2PW] = 63;
    lowerParamsToDisplay();
    setAllButtons();
    if (wholemode) {
      for (int i = 1; i < 77; i++) {
        upperData[i] = 0;
      }
      // upperData[P_osc1SawLevel] = 127;
      // upperData[P_osc2SawLevel] = 127;
      // upperData[P_osc2Detune] = 8;
      // upperData[P_filterCutoff] = 127;
      // upperData[P_env2Sustain] = 127;
      // upperData[P_volumeControl] = 127;
      // upperData[P_noiseLevel] = 63;
      // upperData[P_osc1PW] = 63;
      // upperData[P_osc2PW] = 63;
      upperParamsToDisplay();
      setAllButtons();
    }
  }
  patchName = INITPATCHNAME;
  showPatchPage("Initial", "Patch Settings", "", "");
}

void deletePerformance(int perfNo) {
  char filename[32];
  snprintf(filename, sizeof(filename), "/performances/perf%03d", perfNo);
  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.print("[DELETE] Removed performance: ");
    Serial.println(filename);
  }
}

void renumberPerformancesOnSD() {
  char filename[32];
  for (int i = 0; i < performances.size(); i++) {
    Performance p = performances[i];
    p.performanceNo = i + 1;
    performances[i] = p;

    snprintf(filename, sizeof(filename), "/performances/perf%03d", p.performanceNo);
    savePerformance(filename, p);
  }
}

void checkSwitches() {

  saveButton.update();
  if (saveButton.held()) {
    if (inPerformanceMode && (state == PARAMETER || state == PATCH)) {
      state = PERFORMANCE_DELETE;
    } else if (state == PARAMETER || state == PATCH) {
      state = DELETE;
    }
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case SAVE:
        {
          if (renamedPatch.length() == 0) {
            renamedPatch = INITPATCHNAME;  // fallback if no rename occurred
          }

          // Update patch name depending on upper or lower
          if (upperSW) {
            patchNameU = renamedPatch;
            currentPatchNameU = renamedPatch;
            currentPgmNumU = String(patches.last().patchNo);
          } else {
            patchNameL = renamedPatch;
            currentPatchNameL = renamedPatch;
            currentPgmNumL = String(patches.last().patchNo);
          }

          // ✅ Update last patch in the buffer before saving
          patches.last().patchName = renamedPatch;

          // ✅ Save updated patch data
          String patchData = getCurrentPatchData();
          savePatch(String(patches.last().patchNo).c_str(), patchData);

          // ✅ Reload and reorder patches explicitly
          loadPatches();
          setPatchesOrdering(patches.last().patchNo);

          // ✅ Correctly update patch index for immediate display
          for (int i = 0; i < patches.size(); i++) {
            if (patches[i].patchNo == patches.last().patchNo) {
              if (upperSW) upperPatchIndex = i;
              else lowerPatchIndex = i;
              break;
            }
          }

          // ✅ Immediately refresh display with updated data
          refreshPatchDisplayFromState();

          renamedPatch = "";
          state = PARAMETER;
        }
        break;


      case PATCHNAMING:
        {
          //Serial.println("renamedPatch BEFORE SAVING: " + renamedPatch);

          if (renamedPatch.length() == 0) {
            renamedPatch = patches.last().patchName;  // fallback to existing name
          }

          // Update correct upper/lower patch name based on current layer
          if (upperSW) {
            patchNameU = renamedPatch;
            currentPatchNameU = renamedPatch;  // Update immediately
            currentPgmNumU = String(patches.last().patchNo);
          } else {
            patchNameL = renamedPatch;
            currentPatchNameL = renamedPatch;  // Update immediately
            currentPgmNumL = String(patches.last().patchNo);
          }

          // Update last patch in the patches buffer
          patches.last().patchName = renamedPatch;

          // Save patch data (with the correct name included)
          String patchData = getCurrentPatchData();
          savePatch(String(patches.last().patchNo).c_str(), patchData);

          loadPatches();                   // Refresh patches list from SD card
          refreshPatchDisplayFromState();  // immediately update the display
          setPatchesOrdering(patches.last().patchNo);

          renamedPatch = "";
          state = PARAMETER;
        }
        break;


      case PARAMETER:
        if (inPerformanceMode) {
          if (performances.size() < PERFORMANCES_LIMIT) {
            int newPerfNo = performances.size() + 1;
            Performance newPerf = {
              newPerfNo,
              patches[upperPatchIndex].patchNo,
              patches[lowerPatchIndex].patchNo,
              INITPATCHNAME,
              (PlayMode)playMode
            };
            currentPerformance = newPerf;
            performances.push(newPerf);
            performanceIndex = performances.size() - 1;

            showPerformancePage(
              String(newPerf.performanceNo),
              newPerf.name,
              newPerf.upperPatchNo,
              getPatchName(newPerf.upperPatchNo),
              newPerf.lowerPatchNo,
              getPatchName(newPerf.lowerPatchNo));

            state = PERFORMANCE_SAVE;
          }
        } else {
          // 🛠 PATCH SAVE FLOW
          if (patches.size() < PATCHES_LIMIT) {
            resetPatchesOrdering();  // start from patch 1
            patches.push({ patches.size() + 1, INITPATCHNAME });
            state = SAVE;
          }
        }
        break;

      case PERFORMANCE_SAVE:
        currentPerformance = performances[performanceIndex];
        state = PERFORMANCE_NAMING;
        renamedPatch = currentPerformance.name;
        charIndex = 0;
        currentCharacter = CHARACTERS[charIndex];
        startedRenaming = false;
        showRenamingPage(renamedPatch);
        break;

      case PERFORMANCE_NAMING:
        if (saveButton.numClicks() == 1) {
          if (renamedPatch.length() > 0) {
            currentPerformance.name = renamedPatch;
          }

          upperSW = true;
          savePatch(String(currentPerformance.upperPatchNo).c_str(), getCurrentPatchData());

          upperSW = false;
          savePatch(String(currentPerformance.lowerPatchNo).c_str(), getCurrentPatchData());

          upperSW = true;

          // Update full performance data
          currentPerformance.upperPatchNo = patches[upperPatchIndex].patchNo;
          currentPerformance.lowerPatchNo = patches[lowerPatchIndex].patchNo;
          currentPerformance.mode = (PlayMode)playMode;

          for (int i = 0; i < performances.size(); i++) {
            if (performances[i].performanceNo == currentPerformance.performanceNo) {
              performances[i] = currentPerformance;
              break;
            }
          }

          char filename[16];
          snprintf(filename, sizeof(filename), "perf%03d", currentPerformance.performanceNo);

          savePerformance(filename, currentPerformance);
          loadPerformances();

          renamedPatch = "";
          charIndex = 0;
          currentCharacter = CHARACTERS[0];
          startedRenaming = false;
          state = PARAMETER;
        } else if (recallButton.numClicks() == 1) {
          if (renamedPatch.length() < 12) {
            renamedPatch.concat(String(currentCharacter));
            charIndex = 0;
            currentCharacter = CHARACTERS[charIndex];
            showRenamingPage(renamedPatch);
          }
        } else if (backButton.numClicks() == 1) {
          renamedPatch = "";
          charIndex = 0;
          startedRenaming = false;
          state = PARAMETER;
          if (performances.size() > 0 && performances.last().name == INITPATCHNAME) {
            performances.pop();
          }
        }
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = SETTINGS;
        showSettingsPage();
        break;
      case SETTINGS:
        showSettingsPage();
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SETTINGS:
        state = PARAMETER;
        break;
      case SETTINGSVALUE:
        state = SETTINGS;
        showSettingsPage();
        break;
      case PERFORMANCE_NAMING:
        renamedPatch = "";
        charIndex = 0;
        state = PARAMETER;
        // Optionally remove the unsaved performance from the buffer:
        if (performances.size() > 0 && performances.last().name == INITPATCHNAME) {
          performances.pop();
        }
        break;
      case PERFORMANCE_DELETE:
        setPerformancesOrdering(currentPerformance.performanceNo);
        state = PARAMETER;
        break;
    }
  }

  // Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    if (!recallHeldToggleLatch) {
      inPerformanceMode = !inPerformanceMode;
      recallHeldToggleLatch = true;

      //Serial.print("[MODE] Switched to ");
      //Serial.println(inPerformanceMode ? "Performance Mode" : "Patch Mode");

      showCurrentParameterPage("Mode", inPerformanceMode ? "Performance" : "Patch");

      if (inPerformanceMode && performances.size() > 0) {
        // Entering Performance Mode
        performanceIndex = 0;
        currentPerformance = performances[performanceIndex];

        showPerformancePage(
          String(currentPerformance.performanceNo),
          currentPerformance.name,
          currentPerformance.upperPatchNo,
          getPatchName(currentPerformance.upperPatchNo),
          currentPerformance.lowerPatchNo,
          getPatchName(currentPerformance.lowerPatchNo));

      } else {
        // Returning to Patch Mode
        refreshPatchDisplayFromState();
      }
    }
  } else {
    recallHeldToggleLatch = false;
  }
  if (recallButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        //Serial.println("[INFO] Ignored default RECALL to avoid overwriting performance recall.");
        state = PARAMETER;
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 12)  //actually 12 chars
        {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        break;
      case SETTINGS:
        state = SETTINGSVALUE;
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;

      case PARAMETER:
        // Enter performance recall
        if (performances.size() > 0) {
          currentPerformance = performances.first();
          showPerformancePage(
            String(currentPerformance.performanceNo),
            currentPerformance.name,
            currentPerformance.upperPatchNo,
            getPatchName(currentPerformance.upperPatchNo),
            currentPerformance.lowerPatchNo,
            getPatchName(currentPerformance.lowerPatchNo));
          state = PERFORMANCE_RECALL;
        }
        break;

      case PERFORMANCE_RECALL:
        for (int i = 0; i < patches.size(); i++) {
          if (patches[i].patchNo == currentPerformance.upperPatchNo) {
            upperPatchIndex = i;
          }
          if (patches[i].patchNo == currentPerformance.lowerPatchNo) {
            lowerPatchIndex = i;
          }
        }

        playMode = currentPerformance.mode;
        wholemode = (playMode == WHOLE);
        updateplayMode(0);

        upperSW = true;
        recallPatch(currentPerformance.upperPatchNo);

        upperSW = false;
        recallPatch(currentPerformance.lowerPatchNo);

        refreshPatchDisplayFromState();

        state = PARAMETER;
        patchNo = 0;  // ✅ Clear global patchNo to avoid accidental reuse
        return;

      case PERFORMANCE_NAMING:
        if (renamedPatch.length() < 12) {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        break;

      case PERFORMANCE_DELETE:
        if (performances.size() > 0) {
          state = PERFORMANCE_DELETEMSG;

          int deletedNo = performances.first().performanceNo;
          performances.shift();          // Remove from buffer
          deletePerformance(deletedNo);  // Delete file
          loadPerformances();            // Refresh buffer
          renumberPerformancesOnSD();    // Reorder files
          loadPerformances();            // Reload to apply new order

          currentPerformance = performances.first();
          recallPerformance(currentPerformance);
        }
        state = PARAMETER;
        return;


      case PERFORMANCE_DELETEMSG:
        // Show deletion complete screen briefly
        tft.fillScreen(ST7735_BLACK);
        tft.setFont(&FreeSans12pt7b);
        tft.setTextColor(ST7735_YELLOW);
        tft.setCursor(10, 60);
        tft.println("Renumbering");
        tft.setCursor(10, 100);
        tft.println("Performances...");
        tft.updateScreen();
        delay(1000);
        state = PARAMETER;
        break;
    }
  }
}

// Updated checkEncoder() with upperPatchIndex and lowerPatchIndex
void checkEncoder() {
  long encRead = encoder.read();
  bool moved = false;

  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    moved = true;

    switch (state) {

      case PERFORMANCE_DELETE:
        if (encCW) {
          performances.push(performances.shift());
        } else {
          performances.unshift(performances.pop());
        }
        break;

      case PERFORMANCE_SAVE:
        performanceIndex++;
        if (performanceIndex >= performances.size()) performanceIndex = 0;
        currentPerformance = performances[performanceIndex];
        showPerformancePage(
          String(currentPerformance.performanceNo),
          currentPerformance.name,
          currentPerformance.upperPatchNo,
          getPatchName(currentPerformance.upperPatchNo),
          currentPerformance.lowerPatchNo,
          getPatchName(currentPerformance.lowerPatchNo));
        break;

      case PERFORMANCE_RECALL:
        performanceIndex++;
        if (performanceIndex >= performances.size()) performanceIndex = 0;
        currentPerformance = performances[performanceIndex];
        showPerformancePage(
          String(currentPerformance.performanceNo),
          currentPerformance.name,
          currentPerformance.upperPatchNo,
          getPatchName(currentPerformance.upperPatchNo),
          currentPerformance.lowerPatchNo,
          getPatchName(currentPerformance.lowerPatchNo));
        break;

      case PERFORMANCE_NAMING:
        if (!startedRenaming) {
          renamedPatch = "";
          startedRenaming = true;
        }

        charIndex++;
        if (charIndex >= TOTALCHARS) charIndex = 0;
        currentCharacter = CHARACTERS[charIndex];
        showRenamingPage(renamedPatch + currentCharacter);
        break;

      case PARAMETER:
        if (inPerformanceMode) {
          performanceIndex++;
          if (performanceIndex >= performances.size()) performanceIndex = 0;
          currentPerformance = performances[performanceIndex];

          for (int i = 0; i < patches.size(); i++) {
            if (patches[i].patchNo == currentPerformance.upperPatchNo) upperPatchIndex = i;
            if (patches[i].patchNo == currentPerformance.lowerPatchNo) lowerPatchIndex = i;
          }

          playMode = currentPerformance.mode;
          wholemode = (playMode == WHOLE);
          updateplayMode(0);

          upperSW = true;
          recallPatch(currentPerformance.upperPatchNo);
          upperSW = false;
          recallPatch(currentPerformance.lowerPatchNo);
        } else {
          if (upperSW) {
            upperPatchIndex++;
            if (upperPatchIndex >= patches.size()) upperPatchIndex = 0;
            patchNo = patches[upperPatchIndex].patchNo;
            recallPatch(patchNo);
          } else {
            lowerPatchIndex++;
            if (lowerPatchIndex >= patches.size()) lowerPatchIndex = 0;
            patchNo = patches[lowerPatchIndex].patchNo;
            recallPatch(patchNo);
          }
        }
        refreshPatchDisplayFromState();
        break;

      case RECALL:
      case SAVE:
      case DELETE:
        patches.push(patches.shift());
        break;

      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        break;

      case SETTINGS:
        settings::increment_setting();
        showSettingsPage();
        break;

      case SETTINGSVALUE:
        settings::increment_setting_value();
        showSettingsPage();
        break;
    }
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    moved = true;

    switch (state) {

      case PERFORMANCE_DELETE:
        if (encCW) {
          performances.push(performances.shift());
        } else {
          performances.unshift(performances.pop());
        }
        break;

      case PERFORMANCE_SAVE:
        performanceIndex--;
        if (performanceIndex < 0) performanceIndex = performances.size() - 1;
        currentPerformance = performances[performanceIndex];
        showPerformancePage(
          String(currentPerformance.performanceNo),
          currentPerformance.name,
          currentPerformance.upperPatchNo,
          getPatchName(currentPerformance.upperPatchNo),
          currentPerformance.lowerPatchNo,
          getPatchName(currentPerformance.lowerPatchNo));
        break;

      case PERFORMANCE_RECALL:
        performanceIndex--;
        if (performanceIndex < 0) performanceIndex = performances.size() - 1;
        currentPerformance = performances[performanceIndex];
        showPerformancePage(
          String(currentPerformance.performanceNo),
          currentPerformance.name,
          currentPerformance.upperPatchNo,
          getPatchName(currentPerformance.upperPatchNo),
          currentPerformance.lowerPatchNo,
          getPatchName(currentPerformance.lowerPatchNo));
        break;

      case PERFORMANCE_NAMING:
        if (!startedRenaming) {
          renamedPatch = "";
          startedRenaming = true;
        }

        charIndex--;
        if (charIndex < 0) charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex];
        showRenamingPage(renamedPatch + currentCharacter);
        break;

      case PARAMETER:
        if (inPerformanceMode) {
          performanceIndex--;
          if (performanceIndex < 0) performanceIndex = performances.size() - 1;
          currentPerformance = performances[performanceIndex];

          for (int i = 0; i < patches.size(); i++) {
            if (patches[i].patchNo == currentPerformance.upperPatchNo) upperPatchIndex = i;
            if (patches[i].patchNo == currentPerformance.lowerPatchNo) lowerPatchIndex = i;
          }

          playMode = currentPerformance.mode;
          wholemode = (playMode == WHOLE);
          updateplayMode(0);

          upperSW = true;
          recallPatch(currentPerformance.upperPatchNo);
          upperSW = false;
          recallPatch(currentPerformance.lowerPatchNo);
        } else {
          if (upperSW) {
            upperPatchIndex--;
            if (upperPatchIndex < 0) upperPatchIndex = patches.size() - 1;
            patchNo = patches[upperPatchIndex].patchNo;
            recallPatch(patchNo);
          } else {
            lowerPatchIndex--;
            if (lowerPatchIndex < 0) lowerPatchIndex = patches.size() - 1;
            patchNo = patches[lowerPatchIndex].patchNo;
            recallPatch(patchNo);
          }
        }
        refreshPatchDisplayFromState();
        break;


      case RECALL:
      case SAVE:
      case DELETE:
        patches.unshift(patches.pop());
        break;

      case PATCHNAMING:
        if (charIndex == -1) charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        break;

      case SETTINGS:
        settings::decrement_setting();
        showSettingsPage();
        break;

      case SETTINGSVALUE:
        settings::decrement_setting_value();
        showSettingsPage();
        break;
    }
  }

  if (moved) {
    encPrevious = encRead;
  }
}

String getPatchName(int patchNo) {
  for (int i = 0; i < patches.size(); i++) {
    if (patches[i].patchNo == patchNo) return patches[i].patchName;
  }
  return "-";
}

void setPerformancesOrdering(int no) {
  if (performances.size() < 2) return;
  while (performances.first().performanceNo != no) {
    performances.push(performances.shift());
  }
}

void checkMux() {

  digitalWriteFast(MUX_0, muxInput & B0001);
  digitalWriteFast(MUX_1, muxInput & B0010);
  digitalWriteFast(MUX_2, muxInput & B0100);
  digitalWriteFast(MUX_3, muxInput & B1000);
  delayMicroseconds(5);

  mux1Read = adc->adc0->analogRead(MUX1_S);
  mux2Read = adc->adc0->analogRead(MUX2_S);
  mux3Read = adc->adc1->analogRead(MUX3_S);

  if (mux1Read > (mux1ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux1Read < (mux1ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux1ValuesPrev[muxInput] = mux1Read;
    mux1Read = (mux1Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX1_VCO_BEND:
        myControlChange(midiChannel, CCbendRange, mux1Read);
        break;
      case MUX1_VCF_BEND:
        //myControlChange(midiChannel, CClfo1_rate, mux1Read);
        break;
      case MUX1_VCO_MOD:
        //myControlChange(midiChannel, CClfo1_delay, mux1Read);
        break;
      case MUX1_VCF_MOD:
        //myControlChange(midiChannel, CClfo1_lfo2, mux1Read);
        break;
      case MUX1_GLIDE_TIME:
        myControlChange(midiChannel, CCglideTime, mux1Read);
        break;
      case MUX1_VOLUME:
        myControlChange(midiChannel, CCvolume, mux1Read);
        break;
      case MUX1_BALANCE:
        myControlChange(midiChannel, CCbalance, mux1Read);
        break;
      case MUX1_ARP_RATE:
        myControlChange(midiChannel, CCarpRate, mux1Read);
        break;
      case MUX1_LFO_RATE:
        myControlChange(midiChannel, CClfoRate, mux1Read);
        break;
      case MUX1_LFO_DELAY:
        myControlChange(midiChannel, CClfoDelay, mux1Read);
        break;
      case MUX1_LFO_WAVE:
        myControlChange(midiChannel, CClfoWaveform, mux1Read);
        break;
      case MUX1_VCO_LFO_MOD:
        myControlChange(midiChannel, CCvcoLfoMod, mux1Read);
        break;
      case MUX1_VCO_ENV_MOD:
        myControlChange(midiChannel, CCvcoEnvMod, mux1Read);
        break;
      case MUX1_PWM_MOD:
        myControlChange(midiChannel, CCPWMMod, mux1Read);
        break;
      case MUX1_CROSS_MOD:
        myControlChange(midiChannel, CCcrossMod, mux1Read);
        break;
      case MUX1_VCO1_RANGE:
        myControlChange(midiChannel, CCvco1Range, mux1Read);
        break;
    }
  }

  if (mux2Read > (mux2ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux2Read < (mux2ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux2ValuesPrev[muxInput] = mux2Read;
    mux2Read = (mux2Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX2_VCO1_WAVE:
        myControlChange(midiChannel, CCvco1Waveform, mux2Read);
        break;
      case MUX2_VCO2_RANGE:
        myControlChange(midiChannel, CCvco2Range, mux2Read);
        break;
      case MUX2_VCO2_WAVE:
        myControlChange(midiChannel, CCvco2Waveform, mux2Read);
        break;
      case MUX2_VCO2_FINE:
        myControlChange(midiChannel, CCvco2Fine, mux2Read);
        break;
      case MUX2_VCO_BALANCE:
        myControlChange(midiChannel, CCvcoBalance, mux2Read);
        break;
      case MUX2_HPF:
        myControlChange(midiChannel, CCHPF, mux2Read);
        break;
      case MUX2_CUTOFF:
        myControlChange(midiChannel, CCfilterCutoff, mux2Read);
        break;
      case MUX2_RESONANCE:
        myControlChange(midiChannel, CCresonance, mux2Read);
        break;
      case MUX2_VCF_ENV_MOD:
        myControlChange(midiChannel, CCvcfEnvDepth, mux2Read);
        break;
      case MUX2_VCF_LFO_MOD:
        myControlChange(midiChannel, CCvcfLfoDepth, mux2Read);
        break;
      case MUX2_VCF_KEY_FOLLOW:
        myControlChange(midiChannel, CCvcfKeyFollow, mux2Read);
        break;
      case MUX2_VCA_LEVEL:
        myControlChange(midiChannel, CCvcaLevel, mux2Read);
        break;
    }
  }

  if (mux3Read > (mux3ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux3Read < (mux3ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux3ValuesPrev[muxInput] = mux3Read;
    mux3Read = (mux3Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX3_ENV1_ATTACK:
        myControlChange(midiChannel, CCenv1Attack, mux3Read);
        break;
      case MUX3_ENV1_DECAY:
        myControlChange(midiChannel, CCenv1Decay, mux3Read);
        break;
      case MUX3_ENV1_SUSTAIN:
        myControlChange(midiChannel, CCenv1Sustain, mux3Read);
        break;
      case MUX3_ENV1_RELEASE:
        myControlChange(midiChannel, CCenv1Release, mux3Read);
        break;
      case MUX3_ENV2_ATTACK:
        myControlChange(midiChannel, CCenv2Attack, mux3Read);
        break;
      case MUX3_ENV2_DECAY:
        myControlChange(midiChannel, CCenv2Decay, mux3Read);
        break;
      case MUX3_ENV2_SUSTAIN:
        myControlChange(midiChannel, CCenv2Sustain, mux3Read);
        break;
      case MUX3_ENV2_RELEASE:
        myControlChange(midiChannel, CCenv2Release, mux3Read);
        break;
      case MUX3_DELAY_LEVEL:
        myControlChange(midiChannel, CCdelayLevel, mux3Read);
        break;
      case MUX3_DELAY_TIME:
        myControlChange(midiChannel, CCdelayTime, mux3Read);
        break;
      case MUX3_DELAY_FEEDBACK:
        myControlChange(midiChannel, CCdelayFeedback, mux3Read);
        break;
    }
  }

  muxInput++;
  if (muxInput >= MUXCHANNELS) {
    muxInput = 0;
  }
}

void loop() {

  checkMux();
  checkSwitches();
  pollAllMCPs();
  checkEncoder();
  midi1.read(midiChannel);  //USB HOST MIDI Class Compliant
  MIDI.read(midiChannel);
  MIDI6.read(midiChannel);
  MIDI7.read();
  usbMIDI.read(midiChannel);
}
