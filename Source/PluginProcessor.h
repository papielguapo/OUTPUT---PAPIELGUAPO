#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <atomic>
#include <cstdint>
#include <immintrin.h>

class Phobos67Processor final : public juce::AudioProcessor, private juce::AsyncUpdater {
public:
 enum FlQualityMode {
  Linear2Point = 0,
  Hermite6Point,
  Sinc16Point,
  Sinc24Point,
  Sinc32Point,
  Sinc64Point,
  Sinc128Point,
  Sinc256Point,
  Sinc512Point,
  NumFlQualityModes
 };
 static const juce::StringArray& getResamplingChoices();

 Phobos67Processor();
 ~Phobos67Processor() override {cancelPendingUpdate();}
 void requestOversampling(int stages);
 int oversamplingChoice() const noexcept {return requestedStages.load();}
 void applyOversamplingNow(){handleAsyncUpdate();}
 const juce::String getName() const override{return "OUTPUT";}
 void prepareToPlay(double,int) override;void releaseResources() override{};void reset() override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 juce::AudioProcessorEditor* createEditor() override;bool hasEditor() const override{return true;}
 bool acceptsMidi() const override{return false;}bool producesMidi() const override{return false;}bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return .15;}int getNumPrograms() override{return 1;}int getCurrentProgram() override{return 0;}
 void setCurrentProgram(int) override{};const juce::String getProgramName(int) override{return "Default";}void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;void setStateInformation(const void*,int) override;
 enum UiKey {Width,Collapsed,View,Scale,Channel,Quality,Focused,Slope,Notes,Bass,LockInput,LockOutput,Theme,UiCount};
 std::array<std::atomic<int>,UiCount> ui{{450,0,1,1,1,0,0,0,0,0,0,0,13}};std::atomic<unsigned> uiRevision{0};
 juce::UndoManager undoManager;
 juce::AudioProcessorValueTreeState state;
 bool loadPreset(const juce::File&);bool savePreset(const juce::File&);void undoEdit();void redoEdit();

 std::atomic<float> volumeMatchDb{0};
 std::array<std::atomic<float>,2> harmonicDensityLR{{0.f,0.f}};
 std::array<std::atomic<float>,2> compActivityLR{{0.f,0.f}};
 std::atomic<float> inPeak{-100},outPeak{-100};
 std::atomic<float> inK12{-100},outK12{-100};
 std::array<std::atomic<float>,2> inPeakLR{{-100.f,-100.f}},outPeakLR{{-100.f,-100.f}},inK12LR{{-100.f,-100.f}},outK12LR{{-100.f,-100.f}};
 struct ScopeSample {float input=0,output=0,rightIn=0,rightOut=0;};

 std::atomic<bool> scopeActive{false};
 std::atomic<uint64_t> scopeDropped{0};
 juce::AbstractFifo scopeFifo{32768};
 std::vector<ScopeSample> scopeData = std::vector<ScopeSample>(32768);
 void pushScope(float input,float output,float rightIn=0,float rightOut=0) noexcept {if(!scopeActive.load(std::memory_order_relaxed))return;int a,n,b,m;scopeFifo.prepareToWrite(1,a,n,b,m);if(n){scopeData[(size_t)a]={input,output,rightIn,rightOut};scopeFifo.finishedWrite(1);}else scopeDropped.fetch_add(1,std::memory_order_relaxed);}
 static float inflate(float x,float curve,float effect,bool clip) noexcept;
private:
 friend struct DspProbeAccess;
 void applyParameters(const juce::ValueTree&,bool preserveGains);
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 struct Biquad {std::array<float,5> c{1,0,0,0,0};float z1=0,z2=0;bool robust=false;std::array<double,5> dc{1,0,0,0,0};double dz1=0,dz2=0;
 float tick(float x) noexcept {if(robust){double y=dc[0]*x+dz1;dz1=dc[1]*x-dc[3]*y+dz2;dz2=dc[2]*x-dc[4]*y;return (float)y;}float y=c[0]*x+z1;z1=c[1]*x-c[3]*y+z2;z2=c[2]*x-c[4]*y;return y;}
 void clear(){z1=z2=0;dz1=dz2=0;}};
 struct SplitState {double low=0,high=0,midHigh=0,midLow=0;void clear(){low=high=midHigh=midLow=0;}};
 struct SplitFilter {double b=0,pole=0;double process(double x,double& state,bool highpass) const noexcept {const double y=b*x+state;state=(highpass?-b:b)*x+pole*y;return y;}};
 std::array<SplitState,2> splitState;std::array<bool,2> nativePolarity{};
 SplitFilter splitLow,splitHigh,splitMidHigh,splitMidLow;
 double splitMidGain=1;

 static constexpr int kFlRingBufferSize = 4096;
 static constexpr int kFlRingBufferMask = kFlRingBufferSize - 1;
 static constexpr int kFlResamplerLatency = 553;
 struct FLSincKernelConfig {int halfTaps=256;double deltaW=0,deltaS=0,twoCosDeltaS=-2,twoCosDeltaW=0;};
 struct RationalResamplerState {
  std::array<float,kFlRingBufferSize> history{};int writePos=0;int64_t stepIndex=0,totalWritten=0;
  void reset(int initialDelay) noexcept {history.fill(0.f);writePos=initialDelay&kFlRingBufferMask;stepIndex=0;totalWritten=initialDelay;}
  inline void pushSample(float s) noexcept {history[(size_t)writePos]=s;writePos=(writePos+1)&kFlRingBufferMask;++totalWritten;}
  inline float readAt(int64_t idx) const noexcept {return history[(size_t)(idx&kFlRingBufferMask)];}
 };
 struct FlChannelPipeline {RationalResamplerState downStage,upStage;};
 std::array<FLSincKernelConfig,NumFlQualityModes> downSincConfigs{},upSincConfigs{};
 std::array<FlChannelPipeline,2> flChannels{};
 int lastResamplingChoice=0;
 void buildFLSincConfigs() noexcept;
 void resetFlResampler() noexcept;
 float processFlResamplerSample(int ch,float inSample,FlQualityMode mode) noexcept;
 float evaluateFlSample(const RationalResamplerState&,int64_t,double,FlQualityMode,bool) const noexcept;
 float evaluateFlLinear2(const RationalResamplerState&,int64_t,double) const noexcept;
 float evaluateFlHermite6(const RationalResamplerState&,int64_t,double) const noexcept;
 float evaluateFlSincSSE(const RationalResamplerState&,int64_t,double,const FLSincKernelConfig&) const noexcept;

 float processInflator(float,double,double,bool,int,bool) noexcept;
 static double inflateCore(double,double,double) noexcept;
 std::array<Biquad,2> air,sub,previousAir,previousSub;int airTransition=0,subTransition=0,filterTransitionLength=1;float filterTransitionStep=1;
 void handleAsyncUpdate() override;
 std::atomic<int> requestedStages{2};int osFactor=4;
 std::unique_ptr<juce::dsp::Oversampling<float>> os;
 juce::AudioBuffer<float> scratch,dry,control,meterInputBuffer;juce::AudioBuffer<double> inflateControl;
 double effectCurrent=0,curveCurrent=.5,nativeAlpha=1.0/256;
 std::array<double,2> inputMeterLR{},outputMeterLR{};
 int capacity=0,osDelay=0,delay=0,dryPos=0;double rate=48000;
 std::array<std::array<float,1024>,2> dryDelay{},meterInputDelay{};int meterInputPos=0;
 static constexpr double targetPower=0.03154786722400966; // Measurement reference only: K-12 zero (AES17).
 double inputMeter=0,outputMeter=0,detectorAlpha=0,compDetectorAlpha=0;std::array<double,2> compInputPower{},compDeltaPower{};bool compMeterEnabled=false;
 std::array<juce::SmoothedValue<float>,8> smoothed;juce::SmoothedValue<float> fobosOutputStart,fobosInputStart;
 std::array<float,4> oldFilter{-1,-1,-1,-1};
 juce::SmoothedValue<float> sculpMix,inflateMix;
 std::array<double,2> harmonicLinearPower{},harmonicAddedPower{};bool harmonicMeterEnabled=false;
 bool matchWasEnabled=false;uint64_t matchValidSamples=0;double matchInputPower=0,matchOutputPower=0,matchAlpha=0;juce::SmoothedValue<float> matchGain;
 float value(const char* id) const noexcept{return state.getRawParameterValue(id)->load();}
 void updateFilters();void delayDry(juce::AudioBuffer<float>&);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Phobos67Processor)
};

