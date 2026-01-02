//MIDI CC control numbers
//These broadly follow standard CC assignments
#define   CCmodwheel      1 //pitch LFO amount - less from mod wheel

#define   CClfoRate       3
#define   CCglideTime     5
#define   CClfoDelay      9
#define   CClfoWaveform   12

#define   CCvcoLfoMod     13
#define   CCvcoEnvMod     14
#define   CCvcoModSelSW   15

#define   CCPWMMod        16
#define   CCPWMModSW      17

#define   CCcrossMod      18
#define   CCvco1Range     19
#define   CCvco1Waveform  20

#define   CCvco2Sync      21
#define   CCvco2Range     22
#define   CCvco2Fine      23
#define   CCvco2Waveform  24

#define   CCvcoBalance    25

#define   CCHPF           26
#define   CCvcfSlopeSW    27
#define   CCvcfEnvDepth   28
#define   CCvcfEgSelectSW 29
#define   CCvcfLfoDepth   30
#define   CCvcfKeyFollow  31
#define   CCresonance     71
#define   CCfilterCutoff  74

#define   CCvcaLevel      35
#define   CCvcaModSW      46

#define   CCenv1Attack    47
#define   CCenv1Decay     52
#define   CCenv1Sustain   53
#define   CCenv1Release   54
#define   CCenv1InvertSW  55

#define   CCglideSW       65

#define   CCenv2Attack    73
#define   CCenv2Decay     75
#define   CCenv2Sustain   56
#define   CCenv2Release   72
#define   CCenv2KeyFollowSW 57

#define   CCbendRange     87

#define   CCdelayLevel    91
#define   CCdelayTime     82
#define   CCdelayFeedback 83
#define   CCchorus        93

#define   CCupperSW       97
#define   CClowerSW       98
#define   CCplayMode      100
#define   CCdual_button   101
#define   CCwhole_button  102
#define   CCsplit_button  103

// #define   CCLfoDepth  3 //pitch LFO amount - panel control

// #define   CCglideTime 5

// #define   CCvolumeControl 7
// #define   CCfilterA 8
// #define   CCfilterB 9
// #define   CCamDepth 11
// #define   CCosc2Interval 12

// #define   CCATDepth 14
// #define   CCfmDepth 15
// #define   CCosc1PW 16
// #define   CCosc2PW 17
// #define   CCosc1PWM 18
// #define   CCosc2PWM 19
// #define   CCmodWheelDepth 20
// #define   CCvcaGate 21
// #define   CCfilterLFO 22
// #define   CCnoiseLevel 23
// #define   CCfilterPoleSW 24
// #define   CCPM_FilterEnv 25
// #define   CCPM_DCO2 26
// #define   CCFilterLoop 27
// #define   CCAmpLoop 28

// #define   CCkeyTrack 30
// #define   CCkeyTrackSW 31
// #define   CCwholemode 32
// #define   CCdualmode 33
// #define   CCsplitmode 34
// #define   CCeffectNumSW 35
// #define   CCeffectBankSW 36
// #define   CCmonoMulti 37
// #define   CCpmDestDCO1SW 38
// #define   CCfilterType 39
// #define   CCpmDestFilterSW 40

// #define   CCvcaVel 42
// #define   CCfilterVel 43
// #define   CCfilterRelease 44
// #define   CCfilterAttack 45
// #define   CCfilterSustain 46
// #define   CCfilterDecay 47
// #define   CCfilterLevel 48

// #define   CCfilterEGinv 50

// #define   CCfilterEGlevel 53
// #define   CCkeyboardMode 54
// #define   CCNotePriority 55

// #define   CCampRelease 57
// #define   CCampAttack 58
// #define   CCampSustain 59
// #define   CCampDecay 60
// #define   CCosc2TriangleLevel 61
// #define   CCosc1SubLevel 62
// #define   CCLFODelay 63

// #define   CCglideSW 65
// #define   CCPitchBend 66
// #define   CCMWDepth 67
// #define   CCosc1PulseLevel 68
// #define   CCosc1SawLevel 69

// #define   CCosc2Detune 71

// #define   CCfilterCutoff 74

// #define   CClfoAlt 76
// #define   CCLFORate 77
// #define   CClfoMult 78

// #define   CCosc1Oct 80
// #define   CCosc2Oct 81

// #define   CCeffectPot1 83
// #define   CCeffectPot2  84
// #define   CCeffectPot3  85
// #define   CCeffectsMix  86
// #define   CCpwLFO 87
// #define   CCfilterenvLinLogSW 88
// #define   CCampenvLinLogSW 89
// #define   CCplayMode 90
// #define   CCLFOWaveform 91
// #define   CCdumpCompleteSW 92
// #define   CCdumpStartedSW 93
// #define   CCfilterRes 94
// #define   CCsyncSW 95
// #define   CCchordHoldSW 96

// #define   CCpwLFOwaveformSW 99

// #define   CCosc2PulseLevel 102
// #define   CCosc2SawLevel 103
#define   CCallnotesoff 123//Panic button

// CC values used in the WAVEshare to control params

#define   WSmodwheel  1 //pitch LFO amount - less from mod wheel
#define   WSglideTime 5 // 0-127
#define   WSglideSW 65  // > 63
#define   WSinterval 14 // 0-12
#define   WSdetune 15   // 0-127
#define   WSbendRange 16 // 0-127
#define   WSmodDepth 17 // 0-127
#define   WStmDepth 18 // 0-127
#define   WSATmodDepth 19 // 0-127
#define   WSATtmDepth 20 // 0-127
#define   WSosc1oct 21  // 0, 63, 127
#define   WSosc2oct 22  // 0, 63, 127
#define   WSosc1PW 23 // 0-127
#define   WSosc1PWM 24 // 0-127
#define   WSosc2PW 25 // 0-127
#define   WSosc2PWM 26 // 0-127

#define   WSkeytrack 27 // 0-127
#define   WSkeytrackSW 28 // > 63
#define   WSsyncW 29 // > 63
#define   WSFMDepth 30 // > 63

#define   WSautotune 121 // > 63
#define   WSresetAutotune 122 // > 63
#define   WSallNotesOff 123 // > 63
#define   WSkeyboardMode 127 // > 63


