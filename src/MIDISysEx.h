#pragma once
#include <Arduino.h>

struct Addr2 {
  uint8_t a;   // address MSB
  uint8_t b;   // address LSB
};

constexpr Addr2 LFO_RATE        {0x00, 0x00};
constexpr Addr2 LFO_DELAY_TIME  {0x00, 0x02};

constexpr Addr2 VCOMOD_LFO_MOD       {0x01, 0x00};
constexpr Addr2 VCOMOD_ENV_MOD       {0x01, 0x02};
constexpr Addr2 VCOMOD_PULSEWIDTHMOD {0x01, 0x06};
constexpr Addr2 VCOMOD_CROSSMOD      {0x01, 0x0A};

constexpr Addr2 VCO2_RANGE      {0x01, 0x12};
constexpr Addr2 VCO2_TUNE       {0x01, 0x14};
constexpr Addr2 VCO1_2_SOURCEMIX     {0x01, 0x18};

constexpr Addr2 HPF_CUTOFF      {0x02, 0x00};
constexpr Addr2 VCF_CUTOFF      {0x02, 0x02};
constexpr Addr2 VCF_REZ         {0x02, 0x04};
constexpr Addr2 VCF_ENV_MOD     {0x02, 0x08};
constexpr Addr2 VCF_LFO_MOD     {0x02, 0x0C};
constexpr Addr2 VCF_KEYFOLLOW   {0x02, 0x0E};

constexpr Addr2 VCA_LEVEL       {0x03, 0x00};

constexpr Addr2 ENV1_A          {0x04, 0x00};
constexpr Addr2 ENV1_D          {0x04, 0x02};
constexpr Addr2 ENV1_S          {0x04, 0x04};
constexpr Addr2 ENV1_R          {0x04, 0x06};

constexpr Addr2 ENV2_A          {0x05, 0x00};
constexpr Addr2 ENV2_D          {0x05, 0x02};
constexpr Addr2 ENV2_S          {0x05, 0x04};
constexpr Addr2 ENV2_R          {0x05, 0x06};

constexpr Addr2 GLIDE_TIME      {0x11, 0x02};
