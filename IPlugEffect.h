#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  kRate = 0,
  kDepth,
  kDelay,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugEffect final : public Plugin
{
public:
  IPlugEffect(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

  private:
    double mSampleRate;
    double mLFOPhaseL = 0.0f;
    double mLFOPhaseR = float(PI); // 180° Versatz
    std::vector<float> mChorusBufferL;
    std::vector<float> mChorusBufferR;
    int mWritePos = 0;
    const int kMaxSampleRate = 44100;
    const int kMaxDepthMs = 20;
    const int kMaxSamples = kMaxDepthMs / 1000.0 * kMaxSampleRate * 2;
};
