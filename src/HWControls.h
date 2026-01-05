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
#define MUX1_VOLUME 5
#define MUX1_BALANCE 6
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
#define MUX2_VCO2_WAVE 2
#define MUX2_VCO2_FINE 3
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
#define SOLO_BUTTON 18
#define UNISON_BUTTON 19
#define POLY1_BUTTON 20
#define POLY2_BUTTON 21
#define LOWER_BUTTON 22
#define UPPER_BUTTON 23
#define DUAL_BUTTON 24
#define SPLIT_BUTTON 25
#define WHOLE_BUTTON 26
#define PANEL_LOWER_BUTTON 27
#define PANEL_UPPER_BUTTON 28
#define ENV1_INVERT_BUTTON 29
#define VCA_MOD_DEPTH_BUTTON 30
#define VCF_ENV_SRC_BUTTON 31
#define VCF_SLOPE_BUTTON 32
#define MANUAL_BUTTON 33
#define ENV2_KEYFOLLOW_BUTTON 34
#define CHORUS_BUTTON 35
#define PATCH1_BUTTON 36
#define PATCH2_BUTTON 37
#define PATCH3_BUTTON 38
#define PATCH4_BUTTON 39
#define PATCH5_BUTTON 40
#define PATCH6_BUTTON 41
#define PATCH7_BUTTON 42
#define PATCH8_BUTTON 43
#define PRESET1_BUTTON 44
#define PRESET2_BUTTON 45
#define PRESET3_BUTTON 46
#define PRESET4_BUTTON 47
#define PRESET5_BUTTON 48
#define PRESET6_BUTTON 49
#define PRESET7_BUTTON 50
#define PRESET8_BUTTON 51

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

Button arp_range2_Button = Button(&mcp1, 0, ARP_RANGE2_BUTTON, &mainButtonChanged);
Button arp_range1_Button = Button(&mcp1, 1, ARP_RANGE1_BUTTON, &mainButtonChanged);
Button portamento_Button = Button(&mcp1, 2, PORTAMENTO_BUTTON, &mainButtonChanged);
Button vcf_mod_Button = Button(&mcp1, 3, VCF_MOD_BUTTON, &mainButtonChanged);
Button vco_mod_Button = Button(&mcp1, 4, VCO_MOD_BUTTON, &mainButtonChanged);
Button vcf_bend_Button = Button(&mcp1, 5, VCF_BEND_BUTTON, &mainButtonChanged);
Button vco_bend_Button = Button(&mcp1, 6, VCO_BEND_BUTTON, &mainButtonChanged);

Button arp_mode_up_down_Button = Button(&mcp2, 0, ARP_MODE_UP_DOWN_BUTTON, &mainButtonChanged);
Button arp_mode_random_Button = Button(&mcp2, 2, ARP_MODE_RANDOM_BUTTON, &mainButtonChanged);
Button arp_clk_Button = Button(&mcp2, 6, ARP_CLK_BUTTON, &mainButtonChanged);
Button arp_range3_Button = Button(&mcp2, 8, ARP_RANGE3_BUTTON, &mainButtonChanged);
Button arp_range4_Button = Button(&mcp2, 10, ARP_RANGE4_BUTTON, &mainButtonChanged);
Button arp_mode_up_Button = Button(&mcp2, 12, ARP_MODE_UP_BUTTON, &mainButtonChanged);
Button arp_mode_down_Button = Button(&mcp2, 14, ARP_MODE_DOWN_BUTTON, &mainButtonChanged);

Button vco2_sync_Button = Button(&mcp3, 0, VCO2_SYNC_BUTTON, &mainButtonChanged);
Button vco2_range_Button = Button(&mcp3, 1, VCO2_RANGE_BUTTON, &mainButtonChanged);
Button vco_pwm_src_Button = Button(&mcp3, 2, VCO_PWM_SRC_BUTTON, &mainButtonChanged);
Button vco_mod_dest_Button = Button(&mcp3, 3, VCO_MOD_DEST_BUTTON, &mainButtonChanged);
Button solo_Button = Button(&mcp3, 8, SOLO_BUTTON, &mainButtonChanged);
Button unison_Button = Button(&mcp3, 9, UNISON_BUTTON, &mainButtonChanged);
Button poly1_Button = Button(&mcp3, 10, POLY1_BUTTON, &mainButtonChanged);
Button poly2_Button = Button(&mcp3, 11, POLY2_BUTTON, &mainButtonChanged);

Button lower_Button = Button(&mcp4, 8, LOWER_BUTTON, &mainButtonChanged);
Button upper_Button = Button(&mcp4, 9, UPPER_BUTTON, &mainButtonChanged);
Button dual_Button = Button(&mcp4, 10, DUAL_BUTTON, &mainButtonChanged);
Button split_Button = Button(&mcp4, 11, SPLIT_BUTTON, &mainButtonChanged);
Button whole_Button = Button(&mcp4, 12, WHOLE_BUTTON, &mainButtonChanged);
Button panel_lower_Button = Button(&mcp4, 13, PANEL_LOWER_BUTTON, &mainButtonChanged);
Button panel_upper_Button = Button(&mcp4, 14, PANEL_UPPER_BUTTON, &mainButtonChanged);

Button env1_invert_Button = Button(&mcp7, 0, ENV1_INVERT_BUTTON, &mainButtonChanged);
Button vca_mode_depth_Button = Button(&mcp7, 1, VCA_MOD_DEPTH_BUTTON, &mainButtonChanged);
Button vcf_env_src_Button = Button(&mcp7, 2, VCF_ENV_SRC_BUTTON, &mainButtonChanged);
Button vcf_slope_Button = Button(&mcp7, 3, VCF_SLOPE_BUTTON, &mainButtonChanged);
Button manual_Button = Button(&mcp7, 12, MANUAL_BUTTON, &mainButtonChanged);

Button env2_keyfollow_Button = Button(&mcp8, 0, ENV2_KEYFOLLOW_BUTTON, &mainButtonChanged);
Button chorus_Button = Button(&mcp8, 8, CHORUS_BUTTON, &mainButtonChanged);

Button patch1_Button = Button(&mcp5, 8, PATCH1_BUTTON, &mainButtonChanged);
Button patch2_Button = Button(&mcp5, 9, PATCH2_BUTTON, &mainButtonChanged);
Button patch3_Button = Button(&mcp5, 10, PATCH3_BUTTON, &mainButtonChanged);
Button patch4_Button = Button(&mcp5, 11, PATCH4_BUTTON, &mainButtonChanged);
Button patch5_Button = Button(&mcp5, 12, PATCH5_BUTTON, &mainButtonChanged);
Button patch6_Button = Button(&mcp5, 13, PATCH6_BUTTON, &mainButtonChanged);
Button patch7_Button = Button(&mcp5, 14, PATCH7_BUTTON, &mainButtonChanged);
Button patch8_Button = Button(&mcp6, 8, PATCH8_BUTTON, &mainButtonChanged);

Button preset1_Button = Button(&mcp6, 9, PRESET1_BUTTON, &mainButtonChanged);
Button preset2_Button = Button(&mcp6, 10, PRESET2_BUTTON, &mainButtonChanged);
Button preset3_Button = Button(&mcp6, 11, PRESET3_BUTTON, &mainButtonChanged);
Button preset4_Button = Button(&mcp6, 12, PRESET4_BUTTON, &mainButtonChanged);
Button preset5_Button = Button(&mcp6, 13, PRESET5_BUTTON, &mainButtonChanged);
Button preset6_Button = Button(&mcp6, 14, PRESET6_BUTTON, &mainButtonChanged);
Button preset7_Button = Button(&mcp6, 6, PRESET7_BUTTON, &mainButtonChanged);
Button preset8_Button = Button(&mcp6, 5, PRESET8_BUTTON, &mainButtonChanged);

Button *mainButtons[] = {
  &arp_range2_Button,
  &arp_range1_Button,
  &portamento_Button,
  &vcf_mod_Button,
  &vco_mod_Button,
  &vcf_bend_Button,
  &vco_bend_Button,
  &arp_mode_up_down_Button,
  &arp_mode_random_Button,
  &arp_clk_Button,
  &arp_range3_Button,
  &arp_range4_Button,
  &arp_mode_up_Button,
  &arp_mode_down_Button,
  &vco2_sync_Button,
  &vco2_range_Button,
  &vco_pwm_src_Button,
  &vco_mod_dest_Button,
  &solo_Button,
  &unison_Button,
  &poly1_Button,
  &poly2_Button,
  &lower_Button,
  &upper_Button,
  &dual_Button,
  &split_Button,
  &whole_Button,
  &panel_lower_Button,
  &panel_upper_Button,
  &env1_invert_Button,
  &vca_mode_depth_Button,
  &vcf_env_src_Button,
  &vcf_slope_Button,
  &manual_Button,
  &env2_keyfollow_Button,
  &chorus_Button,
  &patch1_Button,
  &patch2_Button,
  &patch3_Button,
  &patch4_Button,
  &patch5_Button,
  &patch6_Button,
  &patch7_Button,
  &patch8_Button,
  &preset1_Button,
  &preset2_Button,
  &preset3_Button,
  &preset4_Button,
  &preset5_Button,
  &preset6_Button,
  &preset7_Button,
  &preset8_Button,
};

Button *allButtons[] = {
  &arp_range2_Button,
  &arp_range1_Button,
  &portamento_Button,
  &vcf_mod_Button,
  &vco_mod_Button,
  &vcf_bend_Button,
  &vco_bend_Button,
  &arp_mode_up_down_Button,
  &arp_mode_random_Button,
  &arp_clk_Button,
  &arp_range3_Button,
  &arp_range4_Button,
  &arp_mode_up_Button,
  &arp_mode_down_Button,
  &vco2_sync_Button,
  &vco2_range_Button,
  &vco_pwm_src_Button,
  &vco_mod_dest_Button,
  &solo_Button,
  &unison_Button,
  &poly1_Button,
  &poly2_Button,
  &lower_Button,
  &upper_Button,
  &dual_Button,
  &split_Button,
  &whole_Button,
  &panel_lower_Button,
  &panel_upper_Button,
  &env1_invert_Button,
  &vca_mode_depth_Button,
  &vcf_env_src_Button,
  &vcf_slope_Button,
  &manual_Button,
  &env2_keyfollow_Button,
  &chorus_Button,
  &patch1_Button,
  &patch2_Button,
  &patch3_Button,
  &patch4_Button,
  &patch5_Button,
  &patch6_Button,
  &patch7_Button,
  &patch8_Button,
  &preset1_Button,
  &preset2_Button,
  &preset3_Button,
  &preset4_Button,
  &preset5_Button,
  &preset6_Button,
  &preset7_Button,
  &preset8_Button,
};

// Buttons
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
#define PATCH1_SW 8
#define PATCH2_SW 9
#define PATCH3_SW 10
#define PATCH4_SW 11
#define PATCH5_SW 12
#define PATCH6_SW 13
#define PATCH7_SW 14

// GP6
#define PRESET8_SW 5
#define PRESET7_SW 6
#define PATCH1_SW  8
#define PRESET1_SW 9
#define PRESET2_SW 10
#define PRESET3_SW 11
#define PRESET4_SW 12
#define PRESET5_SW 13
#define PRESET6_SW 14

// GP7
#define ENV1_INVERT_SW 0
#define VCA_MOD_DEPTH_SW 1
#define VCF_ENV_SRC_SW 2
#define VCF_SLOPE_SW 3
#define MANUAL_SW 12

// GP8
#define ENV2_KEYFOLLOW_SW 0
#define CHORUS_SW 8

// LEDS

//GP1
#define ARP_RANGE2_LED 7
#define GLIDE_LED_GRN 8
#define GLIDE_LED_RED 9
#define VCF_LFO_LED 10
#define VCO_LFO_LED 11
#define VCF_BEND_LED 12
#define VCO_BEND_LED_GRN 13
#define VCO_BEND_LED_RED 14
#define ARP_RANGE1_LED 15

//GP3
#define VCO2_SYNC_LED 4
#define VCO2_RANGE_LED 5
#define VCO_MOD_DEST_LED_RED 6
#define VCO_MOD_DEST_LED_GRN 7
#define SOLO_LED 12
#define UNISON_LED 13
#define POLY1_LED 14
#define POLY2_LED 15

// GP4
#define PM_LOWER_LED 0
#define LOWER_LED 1
#define UPPER_LED 2
#define DUAL_LED 3
#define SPLIT_LED 4
#define WHOLE_LED 5
#define VCO_PWM_SRC_LED_GRN 6
#define VCO_PWM_SRC_LED_RED 7
#define PM_UPPER_LED 15

// GP5

#define PATCH1_BUTTON_LED 0
#define PATCH2_BUTTON_LED 1
#define PATCH3_BUTTON_LED 2
#define PATCH4_BUTTON_LED 3
#define PATCH5_BUTTON_LED 4
#define PATCH6_BUTTON_LED 5
#define PATCH7_BUTTON_LED 6
#define PATCH8_BUTTON_LED 7
#define PRESET1_BUTTON_LED 15

// GP6
#define PRESET7_BUTTON_LED 0
#define PRESET6_BUTTON_LED 1
#define PRESET5_BUTTON_LED 2
#define PRESET4_BUTTON_LED 3
#define PRESET3_BUTTON_LED 4
#define PRESET2_BUTTON_LED 7
#define PRESET8_BUTTON_LED 15

//GP7
#define ENV1_INVERT_LED_GRN 4
#define ENV1_INVERT_LED_RED 5
#define VCA_MOD_LED_GRN 6
#define VCA_MOD_LED_RED 7
#define VCF_ENV_SRC_LED_GRN 8
#define VCF_ENV_SRC_LED_RED 9
#define VCF_SLOPE_LED_GRN 10
#define VCF_SLOPE_LED_RED 11
#define MANUAL_LED 13

// GP8
#define ENV_KEYFOLLOW_LED_RED 1
#define ENV_KEYFOLLOW_LED_GRN 2
#define CHORUS_LED_GRN 9
#define CHORUS_LED_RED 10

const uint8_t VOICE_LED_PIN[8] = {
  PATCH1_BUTTON_LED,
  PATCH2_BUTTON_LED,
  PATCH3_BUTTON_LED,
  PATCH4_BUTTON_LED,
  PATCH5_BUTTON_LED,
  PATCH6_BUTTON_LED,
  PATCH7_BUTTON_LED,
  PATCH8_BUTTON_LED
};

//Note DAC
#define MULT1V 107
#define MULT1_2V 123
#define MULT2V 210
#define MULT5V 260
#define MULT33V 172
#define MULT3V 344
#define CLAMP2V 26500  // DAC value that corresponds to 2V

#define DAC_CS1 10

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

#define ENCODER_PINA 5
#define ENCODER_PINB 4

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

  //Mux ADC
  pinMode(MUX1_S, INPUT_DISABLE);
  pinMode(MUX2_S, INPUT_DISABLE);
  pinMode(MUX3_S, INPUT_DISABLE);

  //Switches

  pinMode(RECALL_SW, INPUT_PULLUP);  //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);
}

void setupMCPOutputs() {

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

  mcp5.pinMode(0, OUTPUT);   // pin 0 = GPA0 of MCP2301X
  mcp5.pinMode(1, OUTPUT);   // pin 1 = GPA1 of MCP2301X
  mcp5.pinMode(2, OUTPUT);   // pin 2 = GPA2 of MCP2301X
  mcp5.pinMode(3, OUTPUT);   // pin 3 = GPA3 of MCP2301X
  mcp5.pinMode(4, OUTPUT);   // pin 4 = GPA4 of MCP2301X
  mcp5.pinMode(5, OUTPUT);   // pin 5 = GPA5 of MCP2301X
  mcp5.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp5.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp5.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp6.pinMode(0, OUTPUT);   // pin 0 = GPA0 of MCP2301X
  mcp6.pinMode(1, OUTPUT);   // pin 1 = GPA1 of MCP2301X
  mcp6.pinMode(2, OUTPUT);   // pin 2 = GPA2 of MCP2301X
  mcp6.pinMode(3, OUTPUT);   // pin 3 = GPA3 of MCP2301X
  mcp6.pinMode(4, OUTPUT);   // pin 4 = GPA4 of MCP2301X
  mcp6.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp6.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

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
}
