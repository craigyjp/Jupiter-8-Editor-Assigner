byte midiChannel = 1;  //(EEPROM)
int resolutionFrig = 1;
static const byte OUT_CH = 1;   // choose your synth receive channel (1–16)
bool cardStatus = false;

unsigned long lastDisplayTriggerTime = 0;
bool waitingToUpdate = false;
const unsigned long displayTimeout = 2000;  // e.g. 5 seconds

// JP8 Hold

bool keyDownLower[128] = {0};
bool keyDownUpper[128] = {0};
bool keyDownWhole[128] = {0}; // optional (whole mode convenience)

bool holdManualLower = false;
bool holdManualUpper = false;
bool holdPedal = false;     // DP-2 pressed

bool holdLatchedLower[128] = {0}; // notes sustaining because Hold caught their note-off
bool holdLatchedUpper[128] = {0};

// JP8 Arpeggiator

// -------------------- ARP CONFIG --------------------
enum ArpMode : uint8_t { ARP_OFF=0, ARP_UP, ARP_DOWN, ARP_UPDOWN, ARP_RANDOM };
enum ArpClockSrc : uint8_t { ARPCLK_INTERNAL=0, ARPCLK_EXTERNAL, ARPCLK_MIDI };

volatile ArpClockSrc arpClockSrc = ARPCLK_INTERNAL;

enum ArpMidiDiv : uint8_t { ARP_DIV_8TH=0, ARP_DIV_8TH_TRIP, ARP_DIV_16TH };
volatile ArpMidiDiv arpMidiDiv = ARP_DIV_16TH;

// Your existing arpRate (0..127 assumed)
extern uint8_t arpRate;

// External / MIDI clock step accumulator
volatile uint16_t arpClkTickCount = 0;     // counts pulses/ticks until a step
volatile uint8_t arpTicksPerStep = 6;     // default: 16th @ MIDI clock (24ppqn -> 6 ticks)
bool midiClockRunning = false;          // true after Start/Continue, false after Stop
// 0 = 8th, 1 = 8th triplet, 2 = 16th
uint8_t arpMidiDivSW = 2; // pick your default

volatile ArpMode arpMode = ARP_OFF;
volatile uint8_t arpRange = 4;         // 1..4 octaves
volatile uint16_t arpStepMs = 125;      // step interval in ms (internal clock)
volatile float arpGate = 0.50f;         // not strictly needed; we do step-boundary note off

// In Split mode, JP-8 assigns arp to LOWER only. We'll honor that.
bool arpLowerOnlyWhenSplit = true;

// Prevent arp notes from affecting hold/keyDown tracking
bool arpInjecting = false;

// -------------------- ARP PATTERN (JP-8 style) --------------------
uint8_t arpPattern[8] = {0};
uint8_t arpLen = 0;

// Transport over unfolded pattern
int16_t arpPos = -1;    // index into unfolded sequence
int8_t  arpDir = +1;    // for UPDOWN ping-pong

// Current sounding arp note (so we can note-off at next step)
bool arpNoteActive = false;
uint8_t arpCurrentNote = 0;
uint8_t arpCurrentVel = 100;

// Internal clock
uint32_t arpLastStepMs = 0;
bool arpRunning = false;

float arpHzTarget  = 4.0f;   // desired Hz from pot
float arpHzSmooth  = 4.0f;   // smoothed Hz used by engine
uint32_t arpNextStepUs = 0;  // absolute time of next step (micros)
uint32_t arpLastSmoothUs = 0;

// Remember last arp range (JP-8: defaults to 4 after power-up, then remembers last used)
uint8_t lastArpRange = 4;
bool arpEverEnabledSinceBoot = false;

// Save/restore keyboard assign modes when arp forces Poly2
uint8_t savedLowerKBMode = 0;
uint8_t savedUpperKBMode = 0;
bool arpForcedPoly2 = false;

// JP8 style patch handling

bool jp8Mode = true;

#define JP8_SELECT_ROW 0
#define JP8_SELECT_COL 1
uint8_t jp8DigitState = JP8_SELECT_ROW;

uint8_t jp8Row = 1;         // 1..8
uint8_t jp8Col = 1;         // 1..8

uint8_t upperSlotRC = 11;   // 11..88
uint8_t lowerSlotRC = 11;   // 11..88

elapsedMillis jp8DigitTimer;
const uint16_t JP8_DIGIT_TIMEOUT_MS = 2000;

elapsedMillis jp8BlinkTimer;
const uint16_t JP8_BLINK_MS = 200;

constexpr uint8_t LED_ON  = HIGH;
constexpr uint8_t LED_OFF = LOW;

int lastPatchRC_U = 11; 
int lastPatchRC_L = 11;

bool manualSyncPending = false;
uint8_t manualSyncLayer = 0;
uint16_t manualSyncStep = 0;

elapsedMillis manualSyncTimer;
const uint16_t MANUAL_SYNC_PERIOD_MS = 2;

bool manualSyncInProgress = false;
bool suppressParamAnnounce = false;
bool bootInitInProgress = true;
// Other stuff

bool showMuxRead = true;
bool manualMode = false;

String patchNameU = INITPATCHNAME;
String patchNameL = INITPATCHNAME;
String patchName = INITPATCHNAME;
int upperpatchtag = 0;
int lowerpatchtag = 1;
byte splitPoint = 0;
byte oldsplitPoint = 0;
byte newsplitPoint = 0;
byte splitTrans = 0;
byte oldsplitTrans = 0;
int lowerTranspose = 0;

int noteMsg;
int noteVel;
int lastPlayedNote = -1;  // Track the last note played
int lastPlayedVoice = 0;  // Track the voice of the last note played
int lastUsedVoice = 0;    // Global variable to store the last used voice

int upperData[77];
int lowerData[77];
int panelData[77];

#define P_sysex 0
#define P_vcoBendRange 1
#define P_vcfBendRange 2
#define P_vcoLfoModDepth 3
#define P_vcfLfoModDepth 4
#define P_glideTime 5
#define P_balance 6
#define P_volume 7
#define P_arpRate 8
#define P_lfoRate 9
#define P_lfoDelay 10
#define P_lfoWaveform 11
#define P_vcoLfoMod 12
#define P_vcoEnvMod 13
#define P_PWMMod 14
#define P_crossMod 15
#define P_vco1Range 16
#define P_vco1Waveform 17
#define P_vco2Range 18
#define P_vco2Fine 19
#define P_vco2Waveform 20
#define P_vcoBalance 21
#define P_HPF 22
#define P_filterCutoff 23
#define P_resonance 24
#define P_vcfEnvDepth 25
#define P_vcfLfoDepth 26
#define P_vcfKeyFollow 27
#define P_vcaLevel 28
#define P_env1Attack 29
#define P_env1Decay 30
#define P_env1Sustain 31
#define P_env1Release 32
#define P_env2Attack 33
#define P_env2Decay 34
#define P_env2Sustain 35
#define P_env2Release 36
#define P_delayLevel 37
#define P_delayTime 38
#define P_delayFeedback 39
#define P_vcoBendSW 40
#define P_vcoModSW 41
#define P_glideSW 42
#define P_arpSW 43
#define P_vcoModSelSW 44
#define P_PWMModSW 45
#define P_syncSW 46
#define P_vco2RangeSW 47
#define P_vcfSlopeSW 48
#define P_vcfEgSelectSW 49
#define P_vcaModSW 50
#define P_env1InvertSW 51
#define P_env2KeyFollowSW 52
#define P_keyboardModeSW 53
#define P_assignModeSW 54
#define P_arpRangeSW 55
#define P_arpModeSW 56
#define P_AfterTouchDest 57
#define P_ATDepth 58
#define P_chorus 59
#define P_vcfModSW 60


int playMode = 0;
int keyboardMode = 0;
int glideSW = 0;
int vcoBendSW = 0;
int lowerSplitVoicePointer = 0;
int upperSplitVoicePointer = 0;
int performanceIndex = 0;
bool inPerformanceMode = false;
static bool recallHeldToggleLatch = false;
bool startedRenaming = false;
int scaled = 0;

bool encCW = true;  //This is to set the encoder to increment when turned CW - Settings Option
bool announce = true;
// polykit parameters in order of mux

String StratuslfoWaveform = "                ";

int oldfilterCutoff = 0;
int oldfilterCutoffU = 0;
int oldfilterCutoffL = 0;

bool upperSW = false;
int oldupperSW = 0;
bool lowerSW = true;
int oldlowerSW = 0;

int resonancestr = 0;
float filterCutoffstr = 0;
float glideTimestr = 0;
float env1Attackstr = 0;
float env1Decaystr = 0;
float env1Sustainstr = 0;
float env1Releasestr = 0;
float env2Releasestr = 0;
float env2Sustainstr = 0;
float env2Decaystr = 0;
float env2Attackstr = 0;
float LFORatestr = 0;
int lfoDelaystr = 0;
int lfoWaveformstr = 0;
int lfoWaveformDisplay = 0;
int vcoLfoModstr = 0;
int vcoEnvModstr = 0;
int PWMModstr = 0;
int crossModstr = 0;
int vco1Rangestr = 0;
int vco1RangeDisplay = 0;
int vco1Waveformstr = 0;
int vco1WaveformDisplay = 0;
int vco2Rangestr = 0;
int vco2RangeDisplay = 0;
int lowvco2RangeDisplay = 0;
int vco2Waveformstr = 0;
int vco2WaveformDisplay = 0;
float vco2Finestr = 0;
int16_t fineDisp = 0;
float vcoBalancestr = 0;
int HPFstr = 0;
int vcfEnvDepthstr = 0;
int vcfLfoDepthstr = 0;
int vcfKeyFollowstr = 0;
int vcaLevelstr = 0;
float arpRatestr = 0;
int volumestr = 0;
float balancestr = 0;
int delayLevelstr = 0;
int delayTimestr = 0;
int delayFeedbackstr = 0;
int bendRangestr = 0;
int vcoLfoModDepthstr = 0;
int vcfLfoModDepthstr = 0;
int ATDepthstr = 0;

bool wholemode = true;
bool whole_button = true;
bool dualmode = false;
bool dual_button = false;
bool splitmode = false;
bool split_button = false;

int returnvalue = 0;

static const bool LED_ACTIVE_LOW = false;  // set true if LOW turns LED on
