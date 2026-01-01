//Values below are just for initialising and will be changed when synth is initialised to current panel controls & EEPROM settings
byte midiChannel = 1;  //(EEPROM)
int resolutionFrig = 1;

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

// Chord hold
bool chordHoldActive = false;
bool chordHoldWaitingForNotes = false;
uint8_t chordHoldCount = 0;
uint8_t chordHoldRoot = 0;
uint8_t chordHoldIntervals[MAX_CHORD_NOTES] = {0};
unsigned long chordHoldStartTime = 0;
bool chordHoldCaptureWindowActive = false;

// adding encoders
bool rotaryEncoderChanged(int id, bool clockwise, int speed);
#define NUM_ENCODERS 42
unsigned long lastTransition[NUM_ENCODERS + 1];
boolean accelerate = true;
int speed = 1;
int value = 0;
float lastSpeed[NUM_ENCODERS + 1] = { 0 }; // Or whatever your encoder count is

//adding button toggles
static int storedOsc1PWUpper = -1;
static int storedOsc1PWLower = -1;
static bool toggleOsc1PWUpper = false;
static bool toggleOsc1PWLower = false;

static int storedOsc2PWUpper = -1;
static int storedOsc2PWLower = -1;
static bool toggleOsc2PWUpper = false;
static bool toggleOsc2PWLower = false;

static int storedEffectsMixU = -1;
static int storedEffectsMixL = -1;
static bool toggleEffectsMixU = false;
static bool toggleEffectsMixL = false;

static int storedNoiseLevelU = -1;
static int storedNoiseLevelL = -1;
static bool toggleNoiseLevelU = false;
static bool toggleNoiseLevelL = false;

static int storedFM_DepthU = -1;
static int storedFM_DepthL = -1;
static bool toggleFM_DepthU = false;
static bool toggleFM_DepthL = false;

static int storedOsc2_detuneU = -1;
static int storedOsc2_detuneL = -1;
static bool toggleOsc2_detuneU = false;
static bool toggleOsc2_detuneL = false;

static int storedOsc1_SawU = -1;
static int storedOsc1_SawL = -1;
static bool toggleOsc1_SawU = false;
static bool toggleOsc1_SawL = false;

static int storedOsc2_SawU = -1;
static int storedOsc2_SawL = -1;
static bool toggleOsc2_SawU = false;
static bool toggleOsc2_SawL = false;

static int storedOsc1_PulseU = -1;
static int storedOsc1_PulseL = -1;
static bool toggleOsc1_PulseU = false;
static bool toggleOsc1_PulseL = false;

static int storedOsc2_PulseU = -1;
static int storedOsc2_PulseL = -1;
static bool toggleOsc2_PulseU = false;
static bool toggleOsc2_PulseL = false;

static int storedOsc1_SubU = -1;
static int storedOsc1_SubL = -1;
static bool toggleOsc1_SubU = false;
static bool toggleOsc1_SubL = false;

static int storedOsc2_TriU = -1;
static int storedOsc2_TriL = -1;
static bool toggleOsc2_TriU = false;
static bool toggleOsc2_TriL = false;

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
#define P_vcfEnvMod 13
#define P_PWMMod 14
#define P_crossMod 15
#define P_vco1Range 16
#define P_vco1Waveform 17
#define P_vco2Range 18
#define P_vco2Fine19
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

int playMode = 0;
int lowerSplitVoicePointer = 0;
int upperSplitVoicePointer = 0;
int performanceIndex = 0;
bool inPerformanceMode = false;
static bool recallHeldToggleLatch = false;
bool startedRenaming = false;
bool isAutotuning = false;
int scaled = 0;

// footswitch
bool upperfootPedal = false;
bool lowerfootPedal = false;
int upperfastpot3 = 125;
int upperslowpot3 = 3;
bool upperfast = false;
bool upperslow = true;
int lowerfastpot3 = 125;
int lowerslowpot3 = 3;
bool lowerfast = false;
bool lowerslow = true;
int upperLastSentPot3 = -1;
int lowerLastSentPot3 = -1;
unsigned long lastSpeedStepTime = 0;
const int SPEED_STEP_INTERVAL_MS = 20; // for ~2.5s traversal

//Delayed LFO
int numberOfNotes = 0;
int oldnumberOfNotes = 0;
int numberOfNotesU = 0;
int oldnumberOfNotesU = 0;
int numberOfNotesL = 0;
int oldnumberOfNotesL = 0;
unsigned long previousMillisL = 0;
unsigned long intervalL = 1;  //10 seconds
long delaytimeL = 0;
unsigned long previousMillisU = 0;
unsigned long intervalU = 1;  //10 seconds
long delaytimeU = 0;

boolean encCW = true;  //This is to set the encoder to increment when turned CW - Settings Option
boolean announce = true;
// polykit parameters in order of mux

String StratusLFOWaveform = "                ";

int oldfilterCutoff = 0;
int oldfilterCutoffU = 0;
int oldfilterCutoffL = 0;

boolean upperSW = false;
int oldupperSW = 0;
boolean lowerSW = true;
int oldlowerSW = 0;
int chordHoldSW = 0;
int chordHoldU = 0;
int chordHoldL = 0;

float afterTouch = 0;
float afterTouchU = 0;
float afterTouchL = 0;
int AfterTouchDest = 0;
int AfterTouchDestU = 0;
int AfterTouchDestL = 0;
float pwLFOstr = 0;
float fmDepthstr = 0;
float ATDepthstr = 0;
float osc2PWstr = 0;
float osc2PWMstr = 0;
float osc1PWstr = 0;
float osc1PWMstr = 0;
float glideTimestr = 0;
float osc2Detunestr = 0;
int osc2Intervalstr = 0;
float noiseLevelstr = 0;
float osc2SawLevelstr = 0;
float osc1SawLevelstr = 0;
float osc2PulseLevelstr = 0;
float osc1PulseLevelstr = 0;
float osc2TriangleLevelstr = 0;
float osc1SubLevelstr = 0;
float filterCutoffstr = 0;
float filterLFOstr = 0;
float filterResstr = 0;
float filterEGlevelstr = 0;
float LFORatestr = 0;
float LFODelaystr = 0;
int LFOWaveformstr = 0;
float filterAttackstr = 0;
float filterDecaystr = 0;
float filterSustainstr = 0;
float filterReleasestr = 0;
float amDepthstr = 0;
float volumeControlstr = 0;
float ampReleasestr = 0;
float ampSustainstr = 0;
float ampDecaystr = 0;
float ampAttackstr = 0;
float effectPot1str = 0;
float effectPot2str = 0;
float effectPot3str = 0;
float effectsMixstr = 0;
float pmDCO2str = 0;
float pmFilterEnvstr = 0;
float keytrackstr = 0;
float modWheelDepthstr = 0;
int modWheelLevelstr = 0;
int PitchBendLevelstr = 0;  // for display

boolean wholemode = true;
boolean dualmode = false;
boolean splitmode = false;

int LFOWaveCV = 0;
int LFOWaveCVupper = 0;
int LFOWaveCVlower = 0;

int returnvalue = 0;
