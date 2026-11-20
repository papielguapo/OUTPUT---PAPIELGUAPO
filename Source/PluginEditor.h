#pragma once
#include "PluginProcessor.h"
class ScopeLook final : public juce::LookAndFeel_V4 {
public: bool compact=true,diamond=false,ads=false,flStudio=false;int skinId=4;juce::Image adsPanel,metalPanel,purpleKnob,signatureDark,signatureLight;ScopeLook();void setSkinId(int id);void setDiamond(bool);void setAds(bool);void setFlStudio(bool);juce::Font getTextButtonFont(juce::TextButton&,int) override;juce::Font getComboBoxFont(juce::ComboBox&) override;juce::Font getLabelFont(juce::Label&) override;juce::Font getPopupMenuFont() override;void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;void positionComboBoxText(juce::ComboBox&,juce::Label&) override;void drawPopupMenuBackground(juce::Graphics&,int,int) override;void drawPopupMenuSectionHeader(juce::Graphics&,const juce::Rectangle<int>&,const juce::String&) override;void drawPopupMenuItem(juce::Graphics&,const juce::Rectangle<int>&,bool,bool,bool,bool,bool,const juce::String&,const juce::String&,const juce::Drawable*,const juce::Colour*) override;void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class FrequencyBox final : public juce::ComboBox {
 float accumulated=0;double lastWheel=0;int direction=0;bool wheelAllowed=true;
public:
 void setScrollWheelEnabled(bool enabled) noexcept {wheelAllowed=enabled;juce::ComboBox::setScrollWheelEnabled(false);if(!enabled){accumulated=0;direction=0;}}
 void mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel) override {
  if(event.eventComponent!=this){juce::Component::mouseWheelMove(event,wheel);return;}
  if(!wheelAllowed||!isEnabled()||isPopupActive()||wheel.deltaY==0)return;
  if(wheel.isInertial){accumulated=0;return;}
  const double now=juce::Time::getMillisecondCounterHiRes();const int nextDirection=wheel.deltaY>0?-1:1;
  if(now-lastWheel>300||direction!=nextDirection)accumulated=0;lastWheel=now;direction=nextDirection;
  if(wheel.isSmooth){accumulated+=juce::jlimit(-.12f,.12f,wheel.deltaY);if(std::abs(accumulated)<.12f-1e-6f)return;accumulated=0;}else accumulated=0;
  setSelectedItemIndex(juce::jlimit(0,getNumItems()-1,getSelectedItemIndex()+nextDirection),juce::sendNotificationSync);
 }
};
class Phobos67Editor final : public juce::AudioProcessorEditor,private juce::Timer {
public: explicit Phobos67Editor(Phobos67Processor&);~Phobos67Editor() override;void paint(juce::Graphics&) override;void resized() override;void mouseDown(const juce::MouseEvent&) override;
private:
 friend struct ScopeProbeAccess;Phobos67Processor& p;ScopeLook look;juce::Component panel;
 std::array<juce::Slider,10> knobs;std::array<juce::Label,10> labels;std::array<juce::ToggleButton,6> switches;std::array<FrequencyBox,2> frequencies;
 juce::Label harmonicMetersTip,stereoMetersTip;std::array<float,2> harmonicDisplay{};float outputCorrelation=0,outputSidePercent=0;double phaseDcLeft=0,phaseDcRight=0;bool correlationValid=false,stereoValid=false;
 juce::ComboBox scale,order,skin,resampling,oversampling;
 juce::ToggleButton sculpPower{"SCULP"},inflatePower{"INFLATE"},bypassButton{"BYPASS"};std::array<bool,4> overload{};juce::String monitorStatus() const;void updateStates();
 juce::TooltipWindow tips{this,450};bool restoringUi=true;unsigned seenUiRevision=0;
 using SA=juce::AudioProcessorValueTreeState::SliderAttachment;using BA=juce::AudioProcessorValueTreeState::ButtonAttachment;using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;std::vector<std::unique_ptr<SA>> sa;std::vector<std::unique_ptr<BA>> ba;std::vector<std::unique_ptr<CA>> ca;
 struct MeterPeak {float value=0,remaining=0;void update(float peak,float seconds){if(peak>=value){value=peak;remaining=2.f;}else {float decay=juce::jmax(0.f,seconds-remaining);remaining=juce::jmax(0.f,remaining-seconds);value=juce::jmax(peak,value*std::pow(10.f,-decay));}}};
 std::array<float,4> barPeaks{};std::array<MeterPeak,4> meterPeaks{};double meterTickMs=0;
 static float meterOffset(int mode){return mode==3?18.f:mode==4?15.0103f:mode==5?17.0103f:mode==6?23.0103f:0.f;}
 static float meterPosition(int mode,float amplitude){float d=juce::Decibels::gainToDecibels(amplitude,-100.f),offset=meterOffset(mode);return juce::jlimit(0.f,1.f,mode==2?amplitude/2.8183829f:mode==7?(d+16.5f)/27.f:(d+offset+(mode==1?60.f:36.f))/((mode==1?69.f:45.f)+offset));}
 std::array<float,2> rmsInputLR{},rmsOutputLR{},peakInputLR{},peakOutputLR{},vuInputLR{},vuOutputLR{},compDisplayLR{};double lastData=0;
 static int sweetSpotLedCount(float peakDb){return peakDb<-15.f-1e-4f?0:juce::jlimit(0,9,(int)std::floor((peakDb+15.f)/3.f+1e-5f)+1);}
 struct SculpPresetData {juce::String name;float even=0,odd=0,sub=0,subFreq=2,air=0,airFreq=0,mid=0,side=0,input=0,output=0;};
 static juce::File userSculpPresetsDir();
 void applySculpPreset(const SculpPresetData&);
 bool loadUserSculpPresetFile(const juce::File&);
 bool saveUserSculpPresetFile(const juce::File&);
 void promptSaveSculpPreset();
 void openSculpPresetMenu();
 void openSkinPopupMenu();void applySkin();void clearMeterHolds();void saveUi();void restoreUi();void drawStatus(juce::Graphics&);void drawMeters(juce::Graphics&);void drawModuleMeters(juce::Graphics&);void timerCallback() override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Phobos67Editor)
};

