// This optional setting causes Encoder to use more optimized code,
// It must be defined before Encoder.h is included.
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Bounce.h>
#include "TButton.h"
#include <ADC.h>
#include <ADC_util.h>

ADC *adc = new ADC();

#include "Rotary.h"
#include "RotaryEncOverMCP.h"

//Mux 1 Connections
#define MUX1_VCO_BEND 0
#define MUX1_VCF_BEND 1
#define MUX1_VCO_MOD 2
#define MUX1_VCF_MOD 3
#define MUX1_GLIDE_TIME 4
#define MUX1_BALANCE 5
#define MUX1_VOLUME 6
#define MUX1_ARP_RATE 7
#define MUX1_LFO_RATE 8
#define MUX1_LFO_DELAY 9
#define MUX1_LFO_WAVE 10
#define MUX1_VCO_LFO_MOD 11
#define MUX1_VCO_ENV_MOD 12
#define MUX1_PWM_MOD 13
#define MUX1_CROSS_MOD 14
#define MUX1_VCO1_RANGE 15

//Mux 2 Connections
#define MUX2_VCO1_WAVE 0
#define MUX2_VCO2_RANGE 1
#define MUX2_VCO2_FINE 2
#define MUX2_VCO2_WAVE 3
#define MUX2_VCO_BALANCE 4
#define MUX2_HPF 5
#define MUX2_CUTOFF 6
#define MUX2_RESONANCE 7
#define MUX2_VCF_ENV_MOD 8
#define MUX2_VCF_LFO_MOD 9
#define MUX2_VCF_KEY_FOLLOW 10
#define MUX2_VCA_LEVEL 11
#define MUX2_SPARE_12 12
#define MUX2_SPARE_13 13
#define MUX2_SPARE_14 14
#define MUX2_SPARE_15 15

//Mux 3 Connections
#define MUX3_ENV1_ATTACK 0
#define MUX3_ENV1_DECAY 1
#define MUX3_ENV1_SUSTAIN 2
#define MUX3_ENV1_RELEASE 3
#define MUX3_ENV2_ATTACK 4
#define MUX3_ENV2_DECAY 5
#define MUX3_ENV2_SUSTAIN 6
#define MUX3_ENV2_RELEASE 7
#define MUX3_DELAY_LEVEL 8
#define MUX3_DELAY_TIME 9
#define MUX3_DELAY_FEEDBACK 10
#define MUX3_SPARE_11 11
#define MUX3_SPARE_12 12
#define MUX3_SPARE_13 13
#define MUX3_SPARE_14 14
#define MUX3_SPARE_15 15

// Buttons

#define ARP_RANGE2_BUTTON 0
#define ARP_RANGE1_BUTTON 1
#define PORTAMENTO_BUTTON 2
#define VCF_MOD_BUTTON 3
#define VCO_MOD_BUTTON 4
#define VCF_BEND_BUTTON 5
#define VCO_BEND_BUTTON 6
#define ARP_MODE_UP_DOWN_BUTTON 7
#define ARP_MODE_RANDOM_BUTTON 8
#define ARP_CLK_BUTTON 9
#define ARP_RANGE3_BUTTON 10
#define ARP_RANGE4_BUTTON 11
#define ARP_MODE_UP_BUTTON 12
#define ARP_MODE_DOWN_BUTTON 13
#define VCO2_SYNC_BUTTON 14
#define VCO2_RANGE_BUTTON 15
#define VCO_PWM_SRC_BUTTON 16
#define VCO_MOD_DEST_BUTTON 17
#define VCO_BEND_BUTTON 18
#define SOLO_BUTTON 19
#define UNISON_BUTTON 20
#define POLY1_BUTTON 21
#define POLY2_BUTTON 22
#define LOWER_BUTTON 23
#define UPPER_BUTTON 24
#define DUAL_BUTTON 25
#define SPLIT_BUTTON 26
#define WHOLE_BUTTON 27
#define PANEL_LOWER_BUTTON 28
#define PANEL_UPPER_BUTTON 29
#define ENV1_INVERT_BUTTON 30
#define VCA_MOD_DEPTH_BUTTON 31
#define VCF_ENV_SRC_BUTTON 22
#define VCF_SLOPE_BUTTON 33
#define MANUAL_BUTTON 34
#define ENV2_KEYFOLLOW_BUTTON 35
#define CHORUS_BUTTON 36
#define PATCH1_BUTTON 37
#define PATCH2_BUTTON 38
#define PATCH3_BUTTON 39
#define PATCH4_BUTTON 40
#define PATCH5_BUTTON 41
#define PATCH6_BUTTON 42
#define PATCH7_BUTTON 41
#define PATCH8_BUTTON 42
#define PRESET1_BUTTON 43
#define PRESET2_BUTTON 44
#define PRESET3_BUTTON 45
#define PRESET4_BUTTON 46
#define PRESET5_BUTTON 47
#define PRESET6_BUTTON 48
#define PRESET7_BUTTON 49
#define PRESET8_BUTTON 50

// Pins for MCP23017
#define GPA0 0
#define GPA1 1
#define GPA2 2
#define GPA3 3
#define GPA4 4
#define GPA5 5
#define GPA6 6
#define GPA7 7
#define GPB0 8
#define GPB1 9
#define GPB2 10
#define GPB3 11
#define GPB4 12
#define GPB5 13
#define GPB6 14
#define GPB7 15

void mainButtonChanged(Button *btn, bool released);

Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;
Adafruit_MCP23017 mcp4;
Adafruit_MCP23017 mcp5;
Adafruit_MCP23017 mcp6;
Adafruit_MCP23017 mcp7;
Adafruit_MCP23017 mcp8;

//Array of pointers of all MCPs
Adafruit_MCP23017 *allMCPs[] = { &mcp1, &mcp2, &mcp3, &mcp4, &mcp5, &mcp6, &mcp7, &mcp8 };

/* Array of all rotary encoders and their pins */
RotaryEncOverMCP rotaryEncoders[] = {

};

// after your rotaryEncoders[] definition
constexpr size_t NUM_MCP = sizeof(allMCPs) / sizeof(allMCPs[0]);
constexpr int numMCPs = (int)(sizeof(allMCPs) / sizeof(*allMCPs));
constexpr int numEncoders = (int)(sizeof(rotaryEncoders) / sizeof(*rotaryEncoders));

// an array of vectors to hold pointers to the encoders on each MCP
std::vector<RotaryEncOverMCP *> encByMCP[NUM_MCP];

Button osc1_PW_Button = Button(&mcp1, 6, OSC1_PW_BUTTON, &mainButtonChanged);
Button osc2_PW_Button = Button(&mcp1, 14, OSC2_PW_BUTTON, &mainButtonChanged);
Button fm_depth_Button = Button(&mcp2, 6, FM_DEPTH_BUTTON, &mainButtonChanged);
Button osc2_detune_Button = Button(&mcp2, 14, OSC2_DETUNE_BUTTON, &mainButtonChanged);
Button osc1_saw_Button = Button(&mcp3, 6, OSC1_SAW_BUTTON, &mainButtonChanged);
Button osc1_pulse_Button = Button(&mcp3, 14, OSC1_PULSE_BUTTON, &mainButtonChanged);
Button osc1_sub_Button = Button(&mcp4, 6, OSC1_SUB_BUTTON, &mainButtonChanged);
Button osc2_saw_Button = Button(&mcp4, 14, OSC2_SAW_BUTTON, &mainButtonChanged);
Button osc2_pulse_Button = Button(&mcp5, 6, OSC2_PULSE_BUTTON, &mainButtonChanged);
Button osc2_tri_Button = Button(&mcp5, 14, OSC2_TRI_BUTTON, &mainButtonChanged);
Button effects_mix_Button = Button(&mcp7, 6, EFFECTS_MIX_BUTTON, &mainButtonChanged);
Button noise_Button = Button(&mcp7, 14, NOISE_BUTTON, &mainButtonChanged);

Button *mainButtons[] = {
  &osc1_PW_Button,
  &osc2_PW_Button,
  &fm_depth_Button,
  &osc2_detune_Button,
  &osc1_saw_Button,
  &osc1_pulse_Button,
  &osc1_sub_Button,
  &osc2_saw_Button,
  &osc2_pulse_Button,
  &osc2_tri_Button,
  &effects_mix_Button,
  &noise_Button,
};

Button *allButtons[] = {
  &osc1_PW_Button,
  &osc2_PW_Button,
  &fm_depth_Button,
  &osc2_detune_Button,
  &osc1_saw_Button,
  &osc1_pulse_Button,
  &osc1_sub_Button,
  &osc2_saw_Button,
  &osc2_pulse_Button,
  &osc2_tri_Button,
  &effects_mix_Button,
  &noise_Button,
};

// GP1
#define ARP_RANGE2_SW 0
#define ARP_RANGE1_SW 1
#define GLIDE_SW 2
#define VCF_MOD_SW 3
#define VCO_MOD_SW 4
#define VCF_BEND_SW 5
#define VCO_BEND_SW 6

// GP2
#define ARP_MODE_UP_DOWN_SW 0
#define ARP_MODE_RANDOM_SW 2
#define ARP_CLK_SW 6
#define ARP_RANGE3_SW 8
#define ARP_RANGE4_SW 10
#define ARP_MODE_UP_SW 12
#define ARP_MODE_DOWN_SW 14

// GP3
#define VCO2_SYNC_SW 0
#define VCO2_RANGE_SW 1
#define VCO_PWM_SRC_SW 2
#define VCO_MOD_DEST_SW 3
#define SOLO_SW 8
#define UNISON_SW 9
#define POLY1_SW 10
#define POLY2_SW 11

// GP4
#define LOWER_SW 8
#define UPPER_SW 9
#define DUAL_SW 10
#define SPLIT_SW 11
#define WHOLE_SW 12
#define PM_LOWER_SW 13
#define PM_UPPER_SW 14

// GP5


// GP6


// GP7
#define ENV1_INVERT_SW 0
#define VCA_MOD_DEPTH_SW 1
#define VCF_ENV_SRC_SW 2
#define VCF_SLOPE_SW 3
#define MANUAL_SW 12

// GP8
#define ENV2_KEYFOLLOW_SW 0
#define CHORUS_SW 8


//Note DAC
#define MULT1V 107
#define MULT1_2V 123
#define MULT2V 210
#define MULT5V 260
#define MULT33V 172
#define MULT3V 344
#define CLAMP2V 26500  // DAC value that corresponds to 2V

#define DAC_CS1 10

// 74HC165 Switches

#define POLY1_SW 0
#define POLY2_SW 1
#define UNISON_SW 2
#define MONO_SW 3
#define LOWER_SW 4
#define UPPER_SW 5
#define CHORD_HOLD_SW 6
#define KEYBOARD_SW 7

#define GLIDE_SW 8
#define PRIORITY_SW 9
#define DCO1_OCT_SW 10
#define DCO2_OCT_SW 11
#define KEYTRACK_SW 12
#define FILTER_TYPE_SW 13
#define FILTER_POLE_SW 14
#define EG_INVERT_SW 15

#define FILTER_ENV_VELOCITY_SW 16
#define FILTER_ENV_LIN_LOG_SW 17
#define FILTER_ENV_LOOP_SW 18
#define AMP_ENV_VELOCITY_SW 19
#define AMP_ENV_LIN_LOG_SW 20
#define AMP_ENV_LOOP_SW 21
#define LFO_WAVEFORM_SW 22
#define SYNC_SW 23

#define EFFECT_NUMBER_SW 24
#define PM_DCO1_DEST_SW 25
#define PM_FILT_ENV_DEST_SW 26
#define AMP_GATED_SW 27
#define EFFECT_BANK_SW 28
#define LFO_ALT_SW 29
#define LFO_MULTI_MONO_SW 30
#define LFO_MULT_SW 31

// New 595 outputs X8

#define SYNC_UPPER 0
#define SPARE1 1
#define SPARE2 2
#define SPARE3 3
#define FILTER_EG_INV_UPPER 4
#define FILTER_VELOCITY_UPPER 5
#define AMP_VELOCITY_UPPER 6
#define LFO_ALT_UPPER 7

#define POLYMOD_DEST_DCO1_UPPER 8
#define POLYMOD_DEST_FILTER_UPPER 9
#define EFFECT_BANK_1_UPPER 10
#define EFFECT_BANK_2_UPPER 11
#define EFFECT_BANK_3_UPPER 12
#define SPARE13 13
#define FILTER_LIN_LOG_UPPER 14
#define AMP_LIN_LOG_UPPER 15

#define EFFECT_2_UPPER 16
#define EFFECT_1_UPPER 17
#define EFFECT_0_UPPER 18
#define EFFECT_INTERNAL_UPPER 19
#define FILTER_POLE_UPPER 20
#define FILTERA_UPPER 21
#define FILTERB_UPPER 22
#define FILTERC_UPPER 23

#define SYNC_LOWER 24
#define SPARE25 25
#define SPARE26 26
#define SPARE27 27
#define FILTER_EG_INV_LOWER 28
#define FILTER_VELOCITY_LOWER 29
#define AMP_VELOCITY_LOWER 30
#define LFO_ALT_LOWER 31

#define POLYMOD_DEST_DCO1_LOWER 32
#define POLYMOD_DEST_FILTER_LOWER 33
#define EFFECT_BANK_1_LOWER 34
#define EFFECT_BANK_2_LOWER 35
#define EFFECT_BANK_3_LOWER 36
#define UPPER_RELAY_3 37
#define FILTER_LIN_LOG_LOWER 38
#define AMP_LIN_LOG_LOWER 39

#define EFFECT_2_LOWER 40
#define EFFECT_1_LOWER 41
#define EFFECT_0_LOWER 42
#define EFFECT_INTERNAL_LOWER 43
#define FILTER_POLE_LOWER 44
#define FILTERA_LOWER 45
#define FILTERB_LOWER 46
#define FILTERC_LOWER 47

#define FILTER_MODE_BIT0_UPPER 48
#define FILTER_MODE_BIT1_UPPER 49
#define FILTER_MODE_BIT0_LOWER 50
#define FILTER_MODE_BIT1_LOWER 51
#define AMP_MODE_BIT0_UPPER 52
#define AMP_MODE_BIT1_UPPER 53
#define AMP_MODE_BIT0_LOWER 54
#define AMP_MODE_BIT1_LOWER 55

#define UPPER_RELAY_1 56
#define UPPER_RELAY_2 57  // LEDs for LFO (lower Default)
#define LFO_MULTI_BIT0_UPPER 58
#define LFO_MULTI_BIT1_UPPER 59
#define LFO_MULTI_BIT2_UPPER 60
#define LFO_MULTI_BIT0_LOWER 61
#define LFO_MULTI_BIT1_LOWER 62
#define LFO_MULTI_BIT2_LOWER 63

// System Switches etc

#define TUNE_BUTTON 16
#define TUNE_LED 17

#define MUX1_S A0  // ADC0
#define MUX2_S A1  // ADC0
#define MUX3_S A2  // ADC1

#define MUX_0 30
#define MUX_1 31
#define MUX_2 32
#define MUX_3 33

#define RECALL_SW 20
#define SAVE_SW 21
#define SETTINGS_SW 22
#define BACK_SW 23

#define ENCODER_PINA 4
#define ENCODER_PINB 5

#define DEBOUNCE 30
#define MUXCHANNELS 16
#define QUANTISE_FACTOR 1

#define DEBOUNCE 30

static int mux1ValuesPrev[MUXCHANNELS] = {};
static int mux2ValuesPrev[MUXCHANNELS] = {};
static int mux3ValuesPrev[MUXCHANNELS] = {};

static int mux1Read = 0;
static int mux2Read = 0;
static int mux3Read = 0;

static byte muxInput = 0;

static long encPrevious = 0;

TButton saveButton{ SAVE_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton settingsButton{ SETTINGS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton backButton{ BACK_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton recallButton{ RECALL_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };  //On encoder

Encoder encoder(ENCODER_PINB, ENCODER_PINA);  //This often needs the pins swapping depending on the encoder

void setupHardware() {

  adc->adc0->setAveraging(32);                                          // set number of averages 0, 4, 8, 16 or 32.
  adc->adc0->setResolution(8);                                          // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);  // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  //MUXs on ADC1
  adc->adc1->setAveraging(32);                                          // set number of averages 0, 4, 8, 16 or 32.
  adc->adc1->setResolution(8);                                          // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);  // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  //Mux address pins

  pinMode(MUX_0, OUTPUT);
  pinMode(MUX_1, OUTPUT);
  pinMode(MUX_2, OUTPUT);
  pinMode(MUX_3, OUTPUT);

  digitalWrite(MUX_0, LOW);
  digitalWrite(MUX_1, LOW);
  digitalWrite(MUX_2, LOW);
  digitalWrite(MUX_3, LOW);

  pinMode(DAC_CS1, OUTPUT);
  digitalWrite(DAC_CS1, HIGH);

  pinMode(TUNE_LED, OUTPUT);
  digitalWrite(TUNE_LED, LOW);

  pinMode(TUNE_BUTTON, INPUT_PULLUP);

  pinMode(AUTOTUNE_INPUT, INPUT);

  //Switches

  pinMode(RECALL_SW, INPUT_PULLUP);  //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);
}
