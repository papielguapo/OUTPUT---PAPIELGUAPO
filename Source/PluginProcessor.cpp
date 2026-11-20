#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstring>
namespace {
using AP=juce::AudioParameterFloat;using ID=juce::ParameterID;
inline float bitCastFloat(uint32_t bits) noexcept {float f=0.f;std::memcpy(&f,&bits,sizeof(float));return f;}
inline double bitCastDouble(uint64_t bits) noexcept {double d=0.0;std::memcpy(&d,&bits,sizeof(double));return d;}
static const double kFlPi = bitCastDouble(0x400921FB54442D18ULL);
static const double kFlHalfPi = bitCastDouble(0x3FF921FB54442D18ULL);
static const double kFlMinSincFrac = bitCastDouble(0x3ABEF2D0F5DA7DD9ULL);
static constexpr int kFlHalfTapsTable[9] = {1,3,8,12,16,32,64,128,256};
static const float kHermite_13E9EB4 = bitCastFloat(0xbd2aaaabU), kHermite_13E9EB8 = bitCastFloat(0x3e555555U), kHermite_13E9EBC = bitCastFloat(0xbed55555U);
static const float kHermite_13E9EC0 = bitCastFloat(0x3caaaaabU), kHermite_13E9EC4 = bitCastFloat(0xbd800000U), kHermite_13E9EC8 = bitCastFloat(0x3d2aaaabU);
static const float kHermite_13E9ECC = bitCastFloat(0x3dd55555U), kHermite_13E9ED0 = bitCastFloat(0xbf300000U), kHermite_13E9ED4 = bitCastFloat(0x3fc55555U);
static const float kHermite_13E9ED8 = bitCastFloat(0xbd555555U), kHermite_13E9EDC = bitCastFloat(0x3ed00000U), kHermite_13E9EE0 = bitCastFloat(0xbeb55555U);
static const float kHermite_13E9EE4 = bitCastFloat(0xbcc00000U), kHermite_13E9EE8 = bitCastFloat(0x3e22aaabU), kHermite_13E9EEC = bitCastFloat(0xbfae0000U);
static const float kHermite_13E9EF0 = bitCastFloat(0x3c400000U), kHermite_13E9EF4 = bitCastFloat(0xbdc80000U), kHermite_13E9EF8 = bitCastFloat(0x3f160000U);
static const float kMinusTwoPiF = bitCastFloat(0xC0C90FDBU), kPlusTwoPiF = bitCastFloat(0x40C90FDBU);
static const float kWinC0 = bitCastFloat(0x3EB851ECU), kWinC2 = bitCastFloat(0x3F23D70AU), kHalfF = bitCastFloat(0x3F000000U);
}
const juce::StringArray& Phobos67Processor::getResamplingChoices(){
 static const juce::StringArray choices{"RESAMPLING OFF","Native Daw","2-point linear","6-point Hermite","16-point sinc","24-point sinc","32-point sinc","64-point sinc","128-point sinc","256-point sinc","512-point sinc"};
 return choices;
}
juce::AudioProcessorValueTreeState::ParameterLayout Phobos67Processor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout l;
 auto f=[&](const char* id,const char* name,float lo,float hi,float step,float def){l.add(std::make_unique<AP>(ID{id,1},name,juce::NormalisableRange<float>(lo,hi,step),def));};
 f("input","Input",-24,24,.01f,0);f("even","Even Harmonics",0,100,1,0);f("odd","Odd Harmonics",0,100,1,0);
 f("air","Air Gain",0,10,1,0);f("sub","Sub Gain",0,10,1,0);f("mid","Mid Gain",-20,20,1,0);f("side","Side Gain",-20,20,1,0);
 f("effect","Inflate Effect",0,100,.01f,0);f("curve","Inflate Curve",-50,50,.01f,0);f("output","Output",-24,24,.01f,0);
 l.add(std::make_unique<juce::AudioParameterChoice>(ID{"airFreq",1},"Air Frequency",juce::StringArray{"50 kHz","55 kHz","60 kHz","65 kHz"},0));
 l.add(std::make_unique<juce::AudioParameterChoice>(ID{"subFreq",1},"Sub Frequency",juce::StringArray{"40 Hz","50 Hz","60 Hz","100 Hz"},2));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"fobosOn",1},"Meter Input / Output",true));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"inflateOn",1},"Meter K-12 / K-14",true));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"clip",1},"Clip 0 dB",false));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"power",1},"Power",true));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"bandSplit",1},"Inflator Band Split",false));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"reverseOrder",1},"Order B: Inflate before Color",false));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"mono",1},"Monitor Mono",false));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"delta",1},"Monitor Delta",false));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"volumeComp",1},"Volume Compensation",false));
 l.add(std::make_unique<juce::AudioParameterChoice>(ID{"listen",1},"Listen",juce::StringArray{"Normal","Mid solo","Side solo","Final Clip removed"},0));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"sculpEnabled",1},"SCULP Enabled",true));
 l.add(std::make_unique<juce::AudioParameterBool>(ID{"inflateEnabled",1},"INFLATE Enabled",true));
 l.add(std::make_unique<juce::AudioParameterChoice>(ID{"resampling",1},"Resampling quality",getResamplingChoices(),0));
 return l;
}
Phobos67Processor::Phobos67Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,&undoManager,"PHOBOS67",layout()){buildFLSincConfigs();resetFlResampler();}
void Phobos67Processor::buildFLSincConfigs() noexcept {
 constexpr double downStep=8.0/7.0,upStep=7.0/8.0;
 const auto makeConfig=[](int halfTaps,double step) noexcept -> FLSincKernelConfig {
  FLSincKernelConfig cfg;cfg.halfTaps=halfTaps;cfg.deltaW=kFlPi/(double)halfTaps;cfg.twoCosDeltaW=2.0*std::cos(cfg.deltaW);
  const double absStep=std::abs(step);
  if(absStep>1.0){cfg.deltaS=kFlPi/absStep;cfg.twoCosDeltaS=2.0*std::cos(cfg.deltaS);}else{cfg.deltaS=kFlPi;cfg.twoCosDeltaS=-2.0;}
  return cfg;
 };
 for(int m=0;m<(int)NumFlQualityModes;++m){const int ht=kFlHalfTapsTable[m];downSincConfigs[(size_t)m]=makeConfig(ht,downStep);upSincConfigs[(size_t)m]=makeConfig(ht,upStep);}
}
void Phobos67Processor::resetFlResampler() noexcept {
 constexpr int maxHalfTaps=256,downInitialLead=maxHalfTaps+1,upInitialLead=((maxHalfTaps+8)/7)*7;
 for(auto& ch:flChannels){ch.downStage.reset(downInitialLead);ch.upStage.reset(upInitialLead);}
}
float Phobos67Processor::evaluateFlLinear2(const RationalResamplerState& st,int64_t baseIndex,double frac) const noexcept {
 const double y0=(double)st.readAt(baseIndex),y1=(double)st.readAt(baseIndex+1);
 return (float)(y0*(1.0-frac)+y1*frac);
}
float Phobos67Processor::evaluateFlHermite6(const RationalResamplerState& st,int64_t baseIndex,double frac) const noexcept {
 const double xmm0=frac-0.5;
 const float ym2=st.readAt(baseIndex-2),ym1=st.readAt(baseIndex-1),y0=st.readAt(baseIndex),y1=st.readAt(baseIndex+1),y2=st.readAt(baseIndex+2),y3=st.readAt(baseIndex+3);
 const float xmm1=ym2+y3,xmm2=ym2-y3,xmm3=ym1+y2,xmm4=ym1-y2,xmm5=y0+y1,xmm6=y0-y1;
 float xmm7=(kHermite_13E9EB4*xmm2+kHermite_13E9EB8*xmm4)+kHermite_13E9EBC*xmm6;xmm7=(float)((double)xmm7*xmm0);
 float xmm8=((kHermite_13E9EC0*xmm1+kHermite_13E9EC4*xmm3)+kHermite_13E9EC8*xmm5)+xmm7;xmm7=(float)((double)xmm8*xmm0);
 xmm8=((kHermite_13E9ECC*xmm2+kHermite_13E9ED0*xmm4)+kHermite_13E9ED4*xmm6)+xmm7;xmm7=(float)((double)xmm8*xmm0);
 xmm8=((kHermite_13E9ED8*xmm1+kHermite_13E9EDC*xmm3)+kHermite_13E9EE0*xmm5)+xmm7;xmm7=(float)((double)xmm8*xmm0);
 xmm8=((kHermite_13E9EE4*xmm2+kHermite_13E9EE8*xmm4)+kHermite_13E9EEC*xmm6)+xmm7;const float tail=(float)((double)xmm8*xmm0);
 return ((kHermite_13E9EF0*xmm1+kHermite_13E9EF4*xmm3)+kHermite_13E9EF8*xmm5)+tail;
}
float Phobos67Processor::evaluateFlSincSSE(const RationalResamplerState& st,int64_t baseIndex,double frac,const FLSincKernelConfig& cfg) const noexcept {
 if(frac<kFlMinSincFrac)frac=kFlMinSincFrac;
 const double a0=kFlHalfPi-frac*cfg.deltaW,a1=a0-cfg.deltaW,a2=frac*cfg.deltaS,a3=a2+cfg.deltaS;
 const double r0=std::sin(a0),r1=std::sin(a1),r2=std::sin(a2),r3=std::sin(a3);
 const double w_p1_d=cfg.twoCosDeltaW*r0-r1,w_p2_d=cfg.twoCosDeltaW*w_p1_d-r0,s_p1_d=cfg.twoCosDeltaS*r2-r3,s_p2_d=cfg.twoCosDeltaS*s_p1_d-r2;
 __m128 xmm5=_mm_set_ps((float)r1,(float)w_p2_d,(float)r3,(float)s_p2_d),xmm6=_mm_set_ps((float)r0,(float)w_p1_d,(float)r2,(float)s_p1_d);
 const __m128 xmm8=_mm_set_ps((float)cfg.twoCosDeltaW,(float)cfg.twoCosDeltaW,(float)cfg.twoCosDeltaS,(float)cfg.twoCosDeltaS);
 const double fp=frac*kFlPi;
 __m128 xmm4=_mm_set_ps((float)(fp+kFlPi),(float)(fp-2.0*kFlPi),(float)fp,(float)(fp-kFlPi));
 const __m128 xmm9=_mm_set_ps(0.0f,0.0f,kWinC0,kWinC0),xmm10=_mm_set_ps(1.0f,1.0f,kWinC2,kWinC2),xmm11=_mm_set_ps(kPlusTwoPiF,kMinusTwoPiF,kPlusTwoPiF,kMinusTwoPiF);
 __m128 xmm3=_mm_setzero_ps();const int numLoops=cfg.halfTaps>>1;
 for(int k=0;k<numLoops;++k){
  __m128 xmm7=_mm_rcp_ps(xmm4),xmm1=_mm_mul_ps(_mm_mul_ps(xmm4,xmm7),xmm7),xmm0=xmm6;xmm6=xmm5;
  xmm5=_mm_sub_ps(_mm_mul_ps(xmm5,xmm8),xmm0);xmm7=_mm_sub_ps(_mm_add_ps(xmm7,xmm7),xmm1);
  __m128 xmm13=_mm_movehl_ps(xmm0,xmm0),xmm12=_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_mul_ps(xmm10,xmm13),xmm13),xmm13),xmm9);
  xmm4=_mm_add_ps(xmm4,xmm11);xmm0=_mm_mul_ps(_mm_mul_ps(xmm0,xmm7),xmm12);xmm7=_mm_movehl_ps(xmm7,xmm7);
  const int64_t twoK=(int64_t)(k*2);
  const float sPos1=st.readAt(baseIndex+twoK+1),sPos2=st.readAt(baseIndex+twoK+2),sNeg1=st.readAt(baseIndex-twoK-1),sNeg0=st.readAt(baseIndex-twoK);
  xmm1=_mm_set_ps(sNeg0,sNeg0,sPos1,sPos1);const __m128 xmm14=_mm_set_ps(sNeg1,sNeg1,sPos2,sPos2);
  xmm0=_mm_shuffle_ps(xmm0,xmm0,0x50);xmm3=_mm_add_ps(xmm3,_mm_mul_ps(xmm0,xmm1));
  xmm0=xmm6;xmm13=_mm_movehl_ps(xmm6,xmm6);xmm12=_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_mul_ps(xmm10,xmm13),xmm13),xmm13),xmm9);
  xmm6=xmm5;xmm5=_mm_sub_ps(_mm_mul_ps(xmm5,xmm8),xmm0);xmm0=_mm_mul_ps(_mm_mul_ps(xmm0,xmm7),xmm12);
  xmm0=_mm_shuffle_ps(xmm0,xmm0,0x50);xmm3=_mm_add_ps(xmm3,_mm_mul_ps(xmm0,xmm14));
 }
 __m128 xmm0=_mm_add_ps(_mm_movehl_ps(xmm3,xmm3),xmm3);return _mm_cvtss_f32(xmm0)*kHalfF;
}
float Phobos67Processor::evaluateFlSample(const RationalResamplerState& st,int64_t baseIndex,double frac,FlQualityMode mode,bool isDownStage) const noexcept {
 switch(mode){
  case Linear2Point: return evaluateFlLinear2(st,baseIndex,frac);
  case Hermite6Point: return evaluateFlHermite6(st,baseIndex,frac);
  default: return evaluateFlSincSSE(st,baseIndex,frac,isDownStage?downSincConfigs[(size_t)mode]:upSincConfigs[(size_t)mode]);
 }
}
float Phobos67Processor::processFlResamplerSample(int ch,float inSample,FlQualityMode mode) noexcept {
 constexpr int64_t kFixedLookaheadHalfTaps=256;constexpr double invSeven=1.0/7.0,invEight=1.0/8.0;
 auto& pipe=flChannels[(size_t)juce::jlimit(0,1,ch)];pipe.downStage.pushSample(std::isfinite(inSample)?inSample:0.0f);
 while((pipe.downStage.stepIndex*8)/7+kFixedLookaheadHalfTaps<pipe.downStage.totalWritten){
  const int64_t downNum=pipe.downStage.stepIndex*8,downBase=downNum/7;const double downFrac=(double)(downNum%7)*invSeven;
  const float midSample=evaluateFlSample(pipe.downStage,downBase,downFrac,mode,true);++pipe.downStage.stepIndex;
  pipe.upStage.pushSample(std::isfinite(midSample)?midSample:0.0f);
 }
 const int64_t upNum=pipe.upStage.stepIndex*7,upBase=upNum/8;const double upFrac=(double)(upNum%8)*invEight;
 const float outSample=evaluateFlSample(pipe.upStage,upBase,upFrac,mode,false);++pipe.upStage.stepIndex;
 return std::isfinite(outSample)?outSample:0.0f;
}
void Phobos67Processor::requestOversampling(int stages){requestedStages.store(juce::jlimit(0,3,stages));triggerAsyncUpdate();}
void Phobos67Processor::handleAsyncUpdate(){cancelPendingUpdate();if(!os||osFactor==(1<<requestedStages.load()))return;const bool wasSuspended=isSuspended();suspendProcessing(true);{const juce::ScopedLock lock(getCallbackLock());prepareToPlay(rate,capacity);}suspendProcessing(wasSuspended);}
bool Phobos67Processor::isBusesLayoutSupported(const BusesLayout& l) const {return l.getMainInputChannelSet()==l.getMainOutputChannelSet()&&(l.getMainInputChannelSet()==juce::AudioChannelSet::mono()||l.getMainInputChannelSet()==juce::AudioChannelSet::stereo());}
void Phobos67Processor::prepareToPlay(double sr,int bs){rate=sr;capacity=juce::jmax(32,bs);scratch.setSize(2,capacity);meterInputBuffer.setSize(2,capacity);dry.setSize(2,capacity);control.setSize(5,capacity);inflateControl.setSize(2,capacity);detectorAlpha=1-std::exp(-1/(rate*.6));compDetectorAlpha=1-std::exp(-1/(rate*.3));
 osFactor=1<<juce::jlimit(0,3,requestedStages.load());os=std::make_unique<juce::dsp::Oversampling<float>>(2,(size_t)requestedStages.load(),juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,false,true);os->initProcessing((size_t)capacity);osDelay=juce::roundToInt(os->getLatencyInSamples());
 const int resChoice=juce::roundToInt(value("resampling"));lastResamplingChoice=resChoice;
 delay=osDelay+(resChoice>=2?kFlResamplerLatency:0);setLatencySamples(delay);
 // Canonical coefficients keep rounding deterministic at standard sample rates.
 struct Coefficients {double rate,lowB,lowPole,highB,highPole,midHighB,midHighPole,midLowB,midLowPole,attenuation;};
 static constexpr Coefficients canonical[]{
  {22050.0,0.03307608755085591,0.9338478248982881,0.7375047612300502,0.4750095224601002,0.966923912449144,0.9338478248982881,0.26249523876994996,0.4750095224601002,0.9038907407185084},
  {24000.0,0.030468747091253887,0.9390625058174921,0.754762724747214,0.509525449494428,0.969531252908746,0.9390625058174922,0.24523727525278605,0.5095254494944279,0.9032798983634885},
  {44100.0,0.016811316150682133,0.9663773676986357,0.8527645468945084,0.7055290937890168,0.9831886838493179,0.9663773676986358,0.14723545310549152,0.7055290937890168,0.9009666241496598},
  {48000.0,0.015466291403103396,0.9690674171937933,0.8632712640026802,0.7265425280053603,0.9845337085968967,0.9690674171937933,0.13672873599731983,0.7265425280053603,0.9008156654057937},
  {88200.0,0.008476297847396978,0.9830474043052061,0.921069534665058,0.842139069330116,0.9915237021526031,0.9830474043052061,0.07893046533494198,0.8421390693301161,0.9002412811373851},
  {96000.0,0.007792936291951569,0.9844141274160969,0.9270403427317333,0.8540806854634664,0.9922070637080485,0.9844141274160969,0.07295965726826684,0.8540806854634665,0.9002036493759147},
  {176400.0,0.004256109851111122,0.9914877802977778,0.9589853421750296,0.9179706843500591,0.995743890148889,0.9914877802977778,0.04101465782497046,0.9179706843500592,0.9000602969135534},
  {192000.0,0.003911649911246553,0.9921767001775069,0.9621952458291035,0.9243904916582069,0.9960883500887534,0.9921767001775068,0.03780475417089656,0.924390491658207,0.9000508956943478},
  {352800.0,0.0021325834814465345,0.995734833037107,0.9790726808815688,0.9581453617631377,0.9978674165185535,0.995734833037107,0.020927319118431206,0.9581453617631377,0.9000150727686558},
  {384000.0,0.001959650162753165,0.9960806996744936,0.9807407257976642,0.9614814515953284,0.9980403498372468,0.9960806996744936,0.019259274202335794,0.9614814515953284,0.9000127228835515},
 };
 const double lowK=std::tan(3.1415926535898*240.0/rate),highK=std::tan(3.1415926535898*2400.0/rate);
 splitLow={lowK/(1+lowK),(1-lowK)/(1+lowK)};splitHigh={1/(1+highK),(1-highK)/(1+highK)};
 splitMidHigh={1/(1+lowK),(1-lowK)/(1+lowK)};splitMidLow={highK/(1+highK),(1-highK)/(1+highK)};splitMidGain=1/(1-lowK/highK);
 for(const auto& c:canonical)if(rate==c.rate){splitLow={c.lowB,c.lowPole};splitHigh={c.highB,c.highPole};splitMidHigh={c.midHighB,c.midHighPole};splitMidLow={c.midLowB,c.midLowPole};splitMidGain=1/c.attenuation;break;}

 const float defaults[]{1,0,0,0,0,0,.5f,1};for(size_t i=0;i<smoothed.size();i++){smoothed[i].reset((i>=1&&i<=4)?rate*osFactor/4.:rate,.05);smoothed[i].setCurrentAndTargetValue(defaults[i]);}
 fobosInputStart.reset(rate,.05);fobosInputStart.setCurrentAndTargetValue(0);fobosInputStart.setTargetValue(1);fobosOutputStart.reset(rate,.05);fobosOutputStart.setCurrentAndTargetValue(0);fobosOutputStart.setTargetValue(1);effectCurrent=value("effect")/100.f;curveCurrent=(value("curve")+50)/100.f;int rateFactor=1;while(rate>48000.0*rateFactor)rateFactor*=2;nativeAlpha=1.0/(256*rateFactor);reset();oldFilter.fill(-1);updateFilters();}
void Phobos67Processor::reset(){harmonicLinearPower.fill(0);harmonicAddedPower.fill(0);for(auto& value:harmonicDensityLR)value=0;airTransition=subTransition=0;filterTransitionLength=juce::jmax(1,juce::roundToInt(rate*osFactor*.02));filterTransitionStep=1.f/filterTransitionLength;sculpMix.reset(rate,.005);inflateMix.reset(rate,.005);sculpMix.setCurrentAndTargetValue(value("sculpEnabled")>.5f?1.f:0.f);inflateMix.setCurrentAndTargetValue(value("inflateEnabled")>.5f?1.f:0.f);matchWasEnabled=false;matchValidSamples=0;matchInputPower=matchOutputPower=0;matchGain.reset(rate,.5);matchGain.setCurrentAndTargetValue(1);matchAlpha=1-std::exp(-1/(rate*.4));volumeMatchDb=0;meterInputPos=0;for(auto& ring:meterInputDelay)ring.fill(0);compInputPower.fill(0);compDeltaPower.fill(0);for(auto& a:compActivityLR)a=0;nativePolarity.fill(false);inputMeterLR.fill(0);outputMeterLR.fill(0);for(int ch=0;ch<2;ch++){inPeakLR[ch].store(-100);outPeakLR[ch].store(-100);inK12LR[ch].store(-100);outK12LR[ch].store(-100);}for(auto& s:splitState)s.clear();if(os)os->reset();for(auto& b:air)b.clear();for(auto& b:sub)b.clear();for(auto& d:dryDelay)d.fill(0);dryPos=0;resetFlResampler();inputMeter=outputMeter=0;inK12.store(-100);outK12.store(-100);}
void Phobos67Processor::updateFilters(){std::array<float,4> v{value("air"),value("airFreq"),value("sub"),value("subFreq")};const bool initialized=oldFilter[0]>=0;
 if(initialized&&airTransition>0)v[1]=oldFilter[1];if(initialized&&subTransition>0)v[3]=oldFilter[3];if(v==oldFilter)return;
 const bool changeAir=initialized&&v[1]!=oldFilter[1],changeSub=initialized&&v[3]!=oldFilter[3];if(changeAir)previousAir=air;if(changeSub)previousSub=sub;oldFilter=v;
 const float sf[]{40,50,60,100};float sr=(float)(rate*osFactor);
 auto ac=juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(sr,juce::jmin(sr*.49f,50000.f+5000.f*v[1]),.1f,juce::Decibels::decibelsToGain(v[0]*2));
 auto sc=juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(sr,sf[juce::jlimit(0,3,(int)v[3])],.5f,juce::Decibels::decibelsToGain(v[2]));
 const float ai=1.f/ac[3],si=1.f/sc[3];
 for(auto& b:air)for(int i=0;i<5;i++)b.c[(size_t)i]=ac[(size_t)(i<3?i:i+1)]*ai;
 for(auto& b:sub)for(int i=0;i<5;i++)b.c[(size_t)i]=sc[(size_t)(i<3?i:i+1)]*si;
 // Float coefficient rounding can move a very-low shelf pole outside the unit circle.
 // Preserve the measured float path except when the rounded denominator is unstable.
 for(auto& b:sub){bool useDouble=1.0+(double)b.c[3]+b.c[4]<=0||1.0-(double)b.c[3]+b.c[4]<=0||1.0-(double)b.c[4]<=0;
  if(useDouble!=b.robust){if(useDouble){b.dz1=b.z1;b.dz2=b.z2;}else{b.z1=(float)b.dz1;b.z2=(float)b.dz2;}}b.robust=useDouble;
  if(useDouble){auto d=juce::dsp::IIR::ArrayCoefficients<double>::makeLowShelf(rate*osFactor,(double)sf[juce::jlimit(0,3,(int)v[3])],.5,juce::Decibels::decibelsToGain((double)v[2]));for(int i=0;i<5;i++)b.dc[(size_t)i]=d[(size_t)(i<3?i:i+1)]/d[3];}
 }
 if(changeAir)airTransition=filterTransitionLength;if(changeSub)subTransition=filterTransitionLength;
}
double Phobos67Processor::inflateCore(double z,double c,double e) noexcept{
 z=juce::jlimit(-1.9999998,1.9999998,z);if(e==0.0)return z;const double a=std::abs(z);
 double y=a<=1?a*((1+c)+a*((.75-1.5*c-.25*c*c)+a*((-.5+.5*c*c)+a*(-.25*(1-c)*(1-c))))):1-(a-1)*(a-1);
 return (1-e)*z+e*std::copysign(y,z)*.9999995529651642;
}
float Phobos67Processor::inflate(float x,float c,float e,bool clip) noexcept{
 return (float)inflateCore(juce::jlimit(clip?-1.0:-1.9999998,clip?1.0:1.9999998,(double)x),c,e);
}
float Phobos67Processor::processInflator(float x,double c,double e,bool clip,int ch,bool split) noexcept{
 const double input=x==0.f?(nativePolarity[(size_t)ch]?-7e-15:7e-15):(double)x;nativePolarity[(size_t)ch]=!nativePolarity[(size_t)ch];
 const double z=juce::jlimit(clip?-1.0:-1.9999998,clip?1.0:1.9999998,input);
 const auto report=[&](float output){if(compMeterEnabled){double delta=(double)output-(double)(float)z;compInputPower[(size_t)ch]+=compDetectorAlpha*(z*z-compInputPower[(size_t)ch]);compDeltaPower[(size_t)ch]+=compDetectorAlpha*(delta*delta-compDeltaPower[(size_t)ch]);}return output;};
 auto& s=splitState[(size_t)ch];
 const double low=splitLow.process(z,s.low,false),high=splitHigh.process(z,s.high,true);
 const double midHigh=splitMidHigh.process(z,s.midHigh,true);
 const double mid=splitMidLow.process(midHigh,s.midLow,false);
 // Preserve all filter history even at zero effect; only skip the unused shaping math.
 if(e==0.0)return report((float)z);
 if(!split)return report((float)inflateCore(z,c,e));
 return report((float)((1-e)*z+e*(inflateCore(low,c,1)+inflateCore(mid,c,1)/splitMidGain+inflateCore(high,c,1))));
}
void Phobos67Processor::delayDry(juce::AudioBuffer<float>& b){if(delay==0)return;for(int i=0;i<b.getNumSamples();i++){for(int c=0;c<b.getNumChannels();c++){auto& ring=dryDelay[(size_t)c];float x=b.getSample(c,i);b.setSample(c,i,ring[(size_t)dryPos]);ring[(size_t)dryPos]=x;}dryPos=(dryPos+1)%juce::jmax(1,delay);}}
void Phobos67Processor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&){harmonicLinearPower.fill(0);harmonicAddedPower.fill(0);for(auto& value:harmonicDensityLR)value=0;
 matchWasEnabled=false;matchValidSamples=0;matchInputPower=matchOutputPower=0;matchGain.setCurrentAndTargetValue(1);volumeMatchDb=0;
 compMeterEnabled=false;compInputPower.fill(0);compDeltaPower.fill(0);for(auto& a:compActivityLR)a=0;
 for(int j=0;j<b.getNumSamples();j++){for(int ch=0;ch<2;ch++)meterInputDelay[(size_t)ch][(size_t)meterInputPos]=b.getSample(juce::jmin(ch,b.getNumChannels()-1),j);meterInputPos=(meterInputPos+1)%juce::jmax(1,delay);}
 juce::ScopedNoDenormals nd;delayDry(b);float peak=0;
 for(int j=0;j<b.getNumSamples();j++){double energy=0;for(int ch=0;ch<2;ch++){float x=b.getSample(juce::jmin(ch,b.getNumChannels()-1),j);inputMeterLR[ch]+=detectorAlpha*((double)x*x-inputMeterLR[ch]);outputMeterLR[ch]=inputMeterLR[ch];if(ch<b.getNumChannels()){peak=juce::jmax(peak,std::abs(x));energy+=(double)x*x;}}energy/=b.getNumChannels();inputMeter+=detectorAlpha*(energy-inputMeter);outputMeter+=detectorAlpha*(energy-outputMeter);}
 for(int j=0;j<b.getNumSamples();j++){float l=b.getSample(0,j),r=b.getSample(juce::jmin(1,b.getNumChannels()-1),j);pushScope(l,l,r,r);}
 float peakDb=juce::Decibels::gainToDecibels(peak,-100.f);inPeak.store(peakDb);outPeak.store(peakDb);inK12.store((float)(10*std::log10(juce::jmax(1e-20,inputMeter)/targetPower)));outK12.store((float)(10*std::log10(juce::jmax(1e-20,outputMeter)/targetPower)));
 for(int ch=0;ch<2;ch++){float db=juce::Decibels::gainToDecibels(b.getMagnitude(juce::jmin(ch,b.getNumChannels()-1),0,b.getNumSamples()),-100.f);inPeakLR[ch].store(db);outPeakLR[ch].store(db);inK12LR[ch].store((float)(10*std::log10(juce::jmax(1e-20,inputMeterLR[ch])/targetPower)));outK12LR[ch].store((float)(10*std::log10(juce::jmax(1e-20,outputMeterLR[ch])/targetPower)));}
}
void Phobos67Processor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&){juce::ScopedNoDenormals nd;if(!os){b.clear();return;}
 float normalizedPeak=0,peak=0;float inPeaks[2]{},outPeaks[2]{};double densityLinearSum[2]{},densityAddedSum[2]{};uint64_t densitySamples=0;
 sculpMix.setTargetValue(value("sculpEnabled")>.5f?1.f:0.f);inflateMix.setTargetValue(value("inflateEnabled")>.5f?1.f:0.f);
 updateFilters();const bool clip=value("clip")>.5f,power=value("power")>.5f,split=value("bandSplit")>.5f,reverse=value("reverseOrder")>.5f,mono=value("mono")>.5f,delta=value("delta")>.5f;
 const int resamplingChoice=juce::jlimit(0,10,juce::roundToInt(value("resampling")));
 if((resamplingChoice>=2)!=(lastResamplingChoice>=2)){
  resetFlResampler();
  delay=osDelay+(resamplingChoice>=2?kFlResamplerLatency:0);
  setLatencySamples(delay);
  dryPos=0;meterInputPos=0;
  for(auto& d:dryDelay)d.fill(0);
  for(auto& ring:meterInputDelay)ring.fill(0);
 }
 lastResamplingChoice=resamplingChoice;
 const int listen=juce::roundToInt(value("listen"));const bool matchAllowed=power&&!delta&&listen==0;const bool matchEnabled=matchAllowed&&value("volumeComp")>.5f;
 // Feed-forward detector only: never measure the compensated output.
 // Warm up on real signal; do not turn silence/noise or startup ramps into makeup gain.
 double matchBlockPower=0;if(matchEnabled)for(int ch=0;ch<b.getNumChannels();ch++){const double rms=b.getRMSLevel(ch,0,b.getNumSamples());matchBlockPower+=rms*rms/b.getNumChannels();}
 const bool matchSignalPresent=matchEnabled&&std::isfinite(matchBlockPower)&&matchBlockPower>1e-6;
 if(matchEnabled&&!matchWasEnabled){matchInputPower=matchOutputPower=0;matchValidSamples=0;}
 matchWasEnabled=matchEnabled;
 float desiredMatchDb=0;
 if(matchSignalPresent){matchValidSamples+=b.getNumSamples();if(matchValidSamples>=(uint64_t)(rate*.5)&&matchInputPower>1e-6&&matchOutputPower>1e-8&&std::isfinite(matchInputPower)&&std::isfinite(matchOutputPower))desiredMatchDb=juce::jlimit(-12.f,12.f,(float)(10*std::log10(matchInputPower/matchOutputPower)));}
 else {matchInputPower=matchOutputPower=0;matchValidSamples=0;}
 if(!matchAllowed)matchGain.setCurrentAndTargetValue(1);
 else {const float currentDb=juce::Decibels::gainToDecibels(matchGain.getCurrentValue());const float seconds=(float)(b.getNumSamples()/rate);const float rise=(!matchSignalPresent||!matchEnabled)?120.f:2.f,fall=(!matchSignalPresent||!matchEnabled)?120.f:20.f;const float nextDb=currentDb+juce::jlimit(-fall*seconds,rise*seconds,desiredMatchDb-currentDb);matchGain.reset(juce::jmax(1,b.getNumSamples()));matchGain.setTargetValue(juce::Decibels::decibelsToGain(nextDb));}
 const bool applyMatch=matchAllowed&&(matchEnabled||matchGain.isSmoothing()||matchGain.getCurrentValue()!=1.f);
 volumeMatchDb=juce::Decibels::gainToDecibels(matchGain.getCurrentValue());
 harmonicMeterEnabled=power&&scopeActive.load(std::memory_order_relaxed)&&(sculpMix.getCurrentValue()>0||sculpMix.getTargetValue()>0)&&(value("even")>0||value("odd")>0||smoothed[1].getCurrentValue()>0||smoothed[2].getCurrentValue()>0);if(!harmonicMeterEnabled){harmonicLinearPower.fill(0);harmonicAddedPower.fill(0);for(auto& v:harmonicDensityLR)v=0;}
 compMeterEnabled=power&&(inflateMix.getCurrentValue()>0||inflateMix.getTargetValue()>0)&&scopeActive.load(std::memory_order_relaxed)&&(effectCurrent>0||value("effect")>0);if(!compMeterEnabled){compInputPower.fill(0);compDeltaPower.fill(0);for(auto& a:compActivityLR)a=0;}
 const float targets[]{juce::Decibels::decibelsToGain(value("input")),juce::roundToInt(value("even"))*.01f,juce::roundToInt(value("odd"))*.01f,juce::Decibels::decibelsToGain(value("mid")),juce::Decibels::decibelsToGain(value("side")),value("effect")/100.f,(value("curve")+50)/100.f,juce::Decibels::decibelsToGain(value("output"))};for(size_t k=0;k<smoothed.size();k++)smoothed[k].setTargetValue(targets[k]);
 for(int offset=0;offset<b.getNumSamples();offset+=capacity){int n=juce::jmin(capacity,b.getNumSamples()-offset);scratch.setSize(2,n,false,false,true);dry.setSize(b.getNumChannels(),n,false,false,true);
  for(int ch=0;ch<b.getNumChannels();ch++){scratch.copyFrom(ch,0,b,ch,offset,n);dry.copyFrom(ch,0,b,ch,offset,n);}if(b.getNumChannels()==1)scratch.copyFrom(1,0,scratch,0,0,n);delayDry(dry);
  for(int j=0;j<n;j++){
   const float sculp=sculpMix.getNextValue(),inflate=inflateMix.getNextValue();control.setSample(0,j,sculp);control.setSample(4,j,inflate);const float gain=smoothed[0].getNextValue(),tmcInputStart=fobosInputStart.getNextValue();control.setSample(3,j,gain);control.setSample(2,j,smoothed[7].getNextValue());control.setSample(1,j,fobosOutputStart.getNextValue());
   inflateControl.setSample(0,j,effectCurrent);inflateControl.setSample(1,j,curveCurrent);effectCurrent+=(targets[5]-effectCurrent)*nativeAlpha;curveCurrent+=(targets[6]-curveCurrent)*nativeAlpha;
   for(int ch=0;ch<2;ch++){float x=scratch.getSample(ch,j)*gain;meterInputBuffer.setSample(ch,j,delay>0?meterInputDelay[(size_t)ch][(size_t)meterInputPos]:x);meterInputDelay[(size_t)ch][(size_t)meterInputPos]=x;if(reverse){float wet=processInflator(x,inflateControl.getSample(1,j),inflateControl.getSample(0,j),clip,ch,split);if(inflate==1.f)x=wet;else if(inflate>0.f)x+=inflate*(wet-x);}if(sculp==1.f)x*=tmcInputStart;else if(sculp>0.f)x*=1+sculp*(tmcInputStart-1);scratch.setSample(ch,j,x);}
   meterInputPos=(meterInputPos+1)%juce::jmax(1,delay);
  }
  juce::dsp::AudioBlock<float> block(scratch);auto up=os->processSamplesUp(block);if(harmonicMeterEnabled)densitySamples+=up.getNumSamples();
  for(size_t j=0;j<up.getNumSamples();j++){
   float v[5];for(size_t k=1;k<5;k++)v[k]=smoothed[k].getNextValue();
   float l=up.getChannelPointer(0)[j],r=up.getChannelPointer(1)[j];const float rawL=l,rawR=r,sculp=control.getSample(0,juce::jmin(n-1,(int)(j/osFactor)));
   for(int ch=0;ch<2;ch++){float& x=ch==0?l:r;const float harmonicInput=x;if(v[1]==0.f&&v[2]==0.f)x+=0.f;else x=(x+(2*v[1])*(x*x)+(2*v[2])*(x*x*x))*(1.f/(1+1.2f*(v[1]+v[2])));if(harmonicMeterEnabled){const float normalization=1.f/(1+1.2f*(v[1]+v[2]));const double linear=(double)harmonicInput*(1+sculp*(normalization-1)),added=sculp*((double)x-(double)harmonicInput*normalization);densityLinearSum[ch]+=linear*linear;densityAddedSum[ch]+=added*added;}}
   float mid=(l+r)*.5f*v[3],side=(l-r)*.5f*v[4];l=mid+side;r=mid-side;
   const float airBlend=1.f-(airTransition-1)*filterTransitionStep,subBlend=1.f-(subTransition-1)*filterTransitionStep;
   for(int ch=0;ch<2;ch++){float& x=ch==0?l:r;const float airInput=x;float airOutput=air[(size_t)ch].tick(x);if(airTransition>0){float prior=previousAir[(size_t)ch].tick(airInput);x=prior+airBlend*(airOutput-prior);}else x=airOutput;const float subInput=x;float subOutput=sub[(size_t)ch].tick(x);if(subTransition>0){float prior=previousSub[(size_t)ch].tick(subInput);x=prior+subBlend*(subOutput-prior);}else x=subOutput;}if(airTransition>0)--airTransition;if(subTransition>0)--subTransition;if(sculp==0.f){l=rawL;r=rawR;}else if(sculp<1.f){l=rawL+sculp*(l-rawL);r=rawR+sculp*(r-rawR);}up.getChannelPointer(0)[j]=l;up.getChannelPointer(1)[j]=r;
  }
  for(auto& f:air){JUCE_SNAP_TO_ZERO(f.z1);JUCE_SNAP_TO_ZERO(f.z2);}for(auto& f:sub){JUCE_SNAP_TO_ZERO(f.z1);JUCE_SNAP_TO_ZERO(f.z2);}os->processSamplesDown(block);
  for(int j=0;j<n;j++){
   float processed[2]{},original[2]{},meterInput[2]{};double inEnergy=0,outEnergy=0;
   for(int ch=0;ch<2;ch++){
    const int channel=juce::jmin(ch,b.getNumChannels()-1);original[ch]=dry.getSample(channel,j);const float input=meterInputBuffer.getSample(ch,j);meterInput[ch]=power?input:original[ch];inPeaks[ch]=juce::jmax(inPeaks[ch],std::abs(input));inputMeterLR[ch]+=detectorAlpha*((double)input*input-inputMeterLR[ch]);if(ch<b.getNumChannels())inEnergy+=(double)input*input;
    float x=scratch.getSample(ch,j);const float sculp=control.getSample(0,j),inflate=control.getSample(4,j);if(sculp==1.f)x*=control.getSample(1,j);else if(sculp>0.f)x*=1+sculp*(control.getSample(1,j)-1);if(!reverse){float wet=processInflator(x,inflateControl.getSample(1,j),inflateControl.getSample(0,j),clip,ch,split);if(inflate==1.f)x=wet;else if(inflate>0.f)x+=inflate*(wet-x);}if(resamplingChoice>=2)x=processFlResamplerSample(ch,x,static_cast<FlQualityMode>(resamplingChoice-2));processed[ch]=x*control.getSample(2,j);
   }
   if(mono&&power){processed[0]=processed[1]=(processed[0]+processed[1])*.5f;original[0]=original[1]=(original[0]+original[1])*.5f;}
   float match=applyMatch?matchGain.getNextValue():1.f;
   if(matchSignalPresent){double ref=0,wet=0;for(int ch=0;ch<b.getNumChannels();ch++){float output=(power&&clip)?juce::jlimit(-1.f,1.f,processed[ch]):processed[ch];ref+=(double)original[ch]*original[ch];wet+=(double)output*output;}matchInputPower+=matchAlpha*(ref/b.getNumChannels()-matchInputPower);matchOutputPower+=matchAlpha*(wet/b.getNumChannels()-matchOutputPower);}
   float monitored[2]{};
   if(power&&listen!=0){float limited[2]{processed[0],processed[1]};if(clip)for(auto& x:limited)x=juce::jlimit(-1.f,1.f,x);
    if(listen==1)monitored[0]=monitored[1]=(limited[0]+limited[1])*.5f;
    else if(listen==2){monitored[0]=b.getNumChannels()>1?(limited[0]-limited[1])*.5f:0.f;monitored[1]=-monitored[0];}
    else for(int ch=0;ch<2;ch++)monitored[ch]=clip?processed[ch]-limited[ch]:0.f;
   }
   for(int ch=0;ch<2;ch++){float wet=processed[ch];if(power&&clip)wet=juce::jlimit(-1.f,1.f,wet);float x=power?(delta?wet-original[ch]:wet):dry.getSample(juce::jmin(ch,b.getNumChannels()-1),j);if(applyMatch)x*=match;if(power&&listen!=0)x=monitored[ch];outPeaks[ch]=juce::jmax(outPeaks[ch],std::abs(x));outputMeterLR[ch]+=detectorAlpha*((double)x*x-outputMeterLR[ch]);if(ch<b.getNumChannels()){b.setSample(ch,offset+j,x);outEnergy+=(double)x*x;}}
   pushScope(meterInput[0],b.getSample(0,offset+j),meterInput[1],b.getSample(juce::jmin(1,b.getNumChannels()-1),offset+j));
   inputMeter+=detectorAlpha*(inEnergy/b.getNumChannels()-inputMeter);outputMeter+=detectorAlpha*(outEnergy/b.getNumChannels()-outputMeter);
  }
 }
 const double densityAlpha=harmonicMeterEnabled?1-std::exp(-(double)densitySamples/(rate*osFactor*.3)):0;for(int ch=0;ch<2;ch++)if(harmonicMeterEnabled&&densitySamples>0){harmonicLinearPower[ch]+=densityAlpha*(densityLinearSum[ch]/densitySamples-harmonicLinearPower[ch]);harmonicAddedPower[ch]+=densityAlpha*(densityAddedSum[ch]/densitySamples-harmonicAddedPower[ch]);double ratio=harmonicLinearPower[ch]>1e-20?std::sqrt(harmonicAddedPower[ch]/harmonicLinearPower[ch]):0;harmonicDensityLR[ch].store(juce::jlimit(0.f,100.f,(float)(100*ratio/(1+ratio))),std::memory_order_relaxed);}
 for(int ch=0;ch<2;ch++)if(compMeterEnabled)compActivityLR[(size_t)ch].store(compInputPower[(size_t)ch]>1e-20?juce::jlimit(0.f,100.f,(float)(100*std::sqrt(compDeltaPower[(size_t)ch]/compInputPower[(size_t)ch]))):0.f);
 normalizedPeak=juce::jmax(inPeaks[0],inPeaks[1]);peak=juce::jmax(outPeaks[0],outPeaks[1]);inPeak.store(juce::Decibels::gainToDecibels(normalizedPeak,-100.f));outPeak.store(juce::Decibels::gainToDecibels(peak,-100.f));inK12.store((float)(10*std::log10(juce::jmax(1e-20,inputMeter)/targetPower)));outK12.store((float)(10*std::log10(juce::jmax(1e-20,outputMeter)/targetPower)));
 for(int ch=0;ch<2;ch++){inPeakLR[ch].store(juce::Decibels::gainToDecibels(inPeaks[ch],-100.f));outPeakLR[ch].store(juce::Decibels::gainToDecibels(outPeaks[ch],-100.f));inK12LR[ch].store((float)(10*std::log10(juce::jmax(1e-20,inputMeterLR[ch])/targetPower)));outK12LR[ch].store((float)(10*std::log10(juce::jmax(1e-20,outputMeterLR[ch])/targetPower)));}
}
juce::AudioProcessorEditor* Phobos67Processor::createEditor(){return new Phobos67Editor(*this);}
void Phobos67Processor::applyParameters(const juce::ValueTree& tree,bool preserveGains){
 for(const char* id:{"sculpEnabled","inflateEnabled"}){bool found=false;for(auto child:tree)if(child.getProperty("id").toString()==id)found=true;if(!found)state.getParameter(id)->setValueNotifyingHost(1.f);}
 for(auto child:tree){auto id=child.getProperty("id").toString();if(preserveGains&&((id=="input"&&ui[LockInput].load())||(id=="output"&&ui[LockOutput].load())))continue;if(auto* parameter=state.getParameter(id)){float v=(float)child.getProperty("value");if(std::isfinite(v)){if(preserveGains)parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(juce::jlimit(parameter->getNormalisableRange().start,parameter->getNormalisableRange().end,v)));if(preserveGains)parameter->endChangeGesture();}}}
}
void Phobos67Processor::undoEdit(){state.copyState();undoManager.undo();}
void Phobos67Processor::redoEdit(){state.copyState();undoManager.redo();}
bool Phobos67Processor::savePreset(const juce::File& file){auto tree=state.copyState();for(auto child:tree)if(child.getProperty("id")==juce::var("listen"))child.setProperty("value",0.f,nullptr);return tree.createXml()->writeTo(file);}
bool Phobos67Processor::loadPreset(const juce::File& file){auto xml=juce::XmlDocument::parse(file);if(!xml||!xml->hasTagName("PHOBOS67"))return false;auto tree=juce::ValueTree::fromXml(*xml);for(auto child:tree)if(child.getProperty("id")==juce::var("listen"))child.setProperty("value",0.f,nullptr);state.copyState();undoManager.beginNewTransaction("Load preset");applyParameters(tree,true);state.copyState();undoManager.beginNewTransaction();return true;}
void Phobos67Processor::getStateInformation(juce::MemoryBlock& b){auto tree=state.copyState();auto xml=tree.createXml();xml->setAttribute("oversamplingStages",requestedStages.load());xml->setAttribute("analysisPairs",1);xml->setAttribute("directIoScales",1);for(int i=0;i<UiCount;i++)xml->setAttribute("ui"+juce::String(i),ui[(size_t)i].load());copyXmlToBinary(*xml,b);}
void Phobos67Processor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName("PHOBOS67")){requestOversampling(xml->getIntAttribute("oversamplingStages",2));applyParameters(juce::ValueTree::fromXml(*xml),false);for(int i=0;i<UiCount;i++)if(xml->hasAttribute("ui"+juce::String(i)))ui[(size_t)i]=xml->getIntAttribute("ui"+juce::String(i));if(!xml->hasAttribute("analysisPairs")&&xml->hasAttribute("ui4")){int old=ui[Channel].load();ui[Channel]=(ui[View].load()==6||ui[View].load()==10)?3:(old>=3?2:1);}
 if(!xml->hasAttribute("directIoScales")){state.getParameter("power")->setValueNotifyingHost(1.f);state.getParameter("listen")->setValueNotifyingHost(0.f);ui[LockInput]=0;ui[LockOutput]=0;}
 uiRevision.fetch_add(1);state.copyState();undoManager.clearUndoHistory();}}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Phobos67Processor();}
