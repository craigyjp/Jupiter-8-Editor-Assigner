#include <EEPROM.h>

#define EEPROM_MIDI_CH 0
#define EEPROM_SPLITTRANS 1
#define EEPROM_ENCODER_DIR 2
#define EEPROM_LAST_PATCHU 8
#define EEPROM_LAST_PATCHL 9
#define EEPROM_SPLITPOINT 12


int getMIDIChannel() {
  byte midiChannel = EEPROM.read(EEPROM_MIDI_CH);
  if (midiChannel < 0 || midiChannel > 16) midiChannel = MIDI_CHANNEL_OMNI;  //If EEPROM has no MIDI channel stored
  return midiChannel;
}

void storeMidiChannel(byte channel) {
  EEPROM.update(EEPROM_MIDI_CH, channel);
}

float getSplitPoint() {
  byte sp = EEPROM.read(EEPROM_SPLITPOINT);
  if (sp < 0 || sp > 24) sp = 12;
  return sp;
}

void storeSplitPoint(byte type) {
  EEPROM.update(EEPROM_SPLITPOINT, type);
}

float getSplitTrans() {
  int st = EEPROM.read(EEPROM_SPLITTRANS);
  if (st < 0 || st > 4) st = 2;
  return st;  //If EEPROM has no key tracking stored
}

void storeSplitTrans(byte type) {
  EEPROM.update(EEPROM_SPLITTRANS, type);
}

boolean getEncoderDir() {
  byte ed = EEPROM.read(EEPROM_ENCODER_DIR);
  if (ed < 0 || ed > 1) return true;  //If EEPROM has no encoder direction stored
  return ed == 1 ? true : false;
}

void storeEncoderDir(byte encoderDir) {
  EEPROM.update(EEPROM_ENCODER_DIR, encoderDir);
}

int getLastPatchU() {
  int lastPatchNumberU = EEPROM.read(EEPROM_LAST_PATCHU);
  if (lastPatchNumberU < 1 || lastPatchNumberU > 999) lastPatchNumberU = 1;
  return lastPatchNumberU;
}

int getLastPatchL() {
  int lastPatchNumberL = EEPROM.read(EEPROM_LAST_PATCHL);
  if (lastPatchNumberL < 1 || lastPatchNumberL > 999) lastPatchNumberL = 1;
  return lastPatchNumberL;
}

void storeLastPatchU(int lastPatchNumber) {
  EEPROM.update(EEPROM_LAST_PATCHU, lastPatchNumber);
}

void storeLastPatchL(int lastPatchNumber) {
  EEPROM.update(EEPROM_LAST_PATCHL, lastPatchNumber);
}
