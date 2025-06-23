#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include <iostream>

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kRate)->InitDouble("Rate", 0.6, 0.01, 10.0, 0.01, "Hz", IParam::EFlags::kFlagStepped);
  GetParam(kDepth)->InitDouble("Depth", 0.5, 0.01, kMaxDepthMs, 0.01, "ms", IParam::EFlags::kFlagStepped);
  GetParam(kDelay)->InitDouble("Delay", 10, 0, 30, 0.1, "ms", IParam::EFlags::kFlagStepped);

  mChorusBufferL.resize(kMaxSamples);
  mChorusBufferR.resize(kMaxSamples);

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b = pGraphics->GetBounds();

    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    int knobSize = 80;
    int padding = 10; // Abstand zwischen den Knobs und zum Rand
    float startX = (b.W() - (3 * knobSize + padding)) / 2.0;
    float startY = 10; // Position von oben
    IRECT knob1Bounds(startX, startY, startX + knobSize, startY + knobSize);
    // Berechnung der Position für den zweiten Knob
    IRECT knob2Bounds(startX + knobSize + padding, startY, startX + knobSize + padding + knobSize, startY + knobSize);
    IRECT knob3Bounds(startX + 2 * (knobSize + padding), startY, startX + 2 * (knobSize + padding) + knobSize, startY + knobSize);
    GetUI()->AttachControl(new IVKnobControl(knob1Bounds, kRate));
    GetUI()->AttachControl(new IVKnobControl(knob2Bounds, kDepth));
    GetUI()->AttachControl(new IVKnobControl(knob3Bounds, kDelay));
  };
#endif
}

#if IPLUG_DSP
void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double rate = GetParam(kRate)->Value();
  const double depthMs = GetParam(kDepth)->Value();
  const double delayMs = GetParam(kDelay)->Value();

  mSampleRate = GetSampleRate();
  const double depthSamples = (depthMs / 1000.0f) * mSampleRate;
  const double delaySamples = (delayMs / 1000.0f) * mSampleRate;
  const double twoPi = 2.f * float(PI);

  for (int s = 0; s < nFrames; s++)
  {
    double lfoL = std::sin(mLFOPhaseL) * 0.5f + 0.5f;
    double lfoR = std::sin(mLFOPhaseR) * 0.5f + 0.5f;

     // --- Linear Interpolation for Left Channel ---
    int lfoLInt = static_cast<int>(lfoL * depthSamples + delaySamples);
    double fracL = lfoL * depthSamples + delaySamples - lfoLInt; // Fractional part

    int readPos1L = (mWritePos - lfoLInt + kMaxSamples) % kMaxSamples;
    int readPos2L = (mWritePos - (lfoLInt + 1) + kMaxSamples) % kMaxSamples;

    double wetL1 = mChorusBufferL[readPos1L];
    double wetL2 = mChorusBufferL[readPos2L];

    double wetL = wetL1 + fracL * (wetL2 - wetL1); // Linear interpolation

     // --- Linear Interpolation for Right Channel ---
    int lfoRInt = static_cast<int>(lfoR * depthSamples + delaySamples);
    double fracR = lfoR * depthSamples + delaySamples - lfoRInt; // Fractional part

    int readPos1R = (mWritePos - lfoRInt + kMaxSamples) % kMaxSamples;
    int readPos2R = (mWritePos - (lfoRInt + 1) + kMaxSamples) % kMaxSamples;

    double wetR1 = mChorusBufferL[readPos1R];
    double wetR2 = mChorusBufferL[readPos2R];

    double wetR = wetR1 + fracR * (wetR2 - wetR1); // Linear interpolation

    outputs[0][s] = inputs[0][s] + wetL;
    outputs[1][s] = inputs[1][s] + wetR;

    mChorusBufferL[mWritePos] = inputs[0][s];
    mChorusBufferR[mWritePos] = inputs[1][s];

    mWritePos = (mWritePos + 1) % kMaxSamples;

    mLFOPhaseL += twoPi * rate / mSampleRate;
    mLFOPhaseR += twoPi * rate / mSampleRate;

    if (mLFOPhaseL >= twoPi)
      mLFOPhaseL -= twoPi;
    if (mLFOPhaseR >= twoPi)
      mLFOPhaseR -= twoPi;
  }
}
#endif
