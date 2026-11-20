#include "PluginEditor.h"
#include <OutputSkinData.h>
#include <cmath>
namespace {
thread_local bool compactPaint=false,diamondPaint=false,adsPaint=false,flPaint=false;
thread_local int activeSkinId=4;
namespace FLColours {
 const juce::Colour background    {0xff282c30};
 const juce::Colour tabFill       {0xff212529};
 const juce::Colour tabBorder     {0xff393f45};
 const juce::Colour comboFill     {0xff1d2024};
 const juce::Colour comboBorder   {0xff3a4046};
 const juce::Colour textMain      {0xffe6e9ec};
 const juce::Colour highlightFill {0xff8c9aa8};
 const juce::Colour highlightText {0xffffffff};
 const juce::Colour sectionBg     {0xff31363b};
 const juce::Colour sectionText   {0xff8a929a};
}
static constexpr int kTotalSkins = 18;
struct SkinPalette {juce::uint32 winBg,hdrBg,pnlBg,pnlBorder,recBg,txtBright,txtDim,accent;bool light;};
static constexpr SkinPalette kSkins[19] = {
 {0xff282c30,0xff212529,0xff212529,0xff393f45,0xff1d2024,0xffe6e9ec,0xff8a929a,0xff36d477,false}, // 0 fallback
 {0xff000000,0xff101010,0xff000000,0xff414141,0xff101010,0xffdddddd,0xff777777,0xff36d477,false}, // 1 BLACK
 {0xffc0c4c6,0xff101010,0xffc0c4c6,0xff626a6e,0xff101010,0xff111519,0xff41474b,0xff641096,true},  // 2 DIAMOND II
 {0xff080808,0xff101010,0xff080808,0xff414141,0xff101010,0xffdddddd,0xffdddddd,0xff36d477,false}, // 3 Ads
 {0xff282c30,0xff212529,0xff212529,0xff393f45,0xff1d2024,0xffe6e9ec,0xff8a929a,0xff36d477,false}, // 4 FL STUDIO
 {0xff14161a,0xff101216,0xff1b1e23,0xff282d35,0xff111317,0xffe2e6ec,0xff848e9c,0xff52ff7a,false}, // 5 FL DARK
 {0xff2b2b2b,0xff1e1e1e,0xff383838,0xff141414,0xff1f1f1f,0xffe2e6ec,0xff9e9e9e,0xffffb000,false}, // 6 ABLETON LIVE
 {0xff1b1d20,0xff141518,0xff26292e,0xff3a3e47,0xff14161a,0xffe2e6ec,0xff8e96a3,0xff26d6ff,false}, // 7 LOGIC PRO X
 {0xff25272b,0xff1a1c20,0xff31343a,0xff454a54,0xff14171a,0xffe2e6ec,0xff98a4b3,0xff4eff4e,false}, // 8 PRO TOOLS
 {0xff1c1f24,0xff131519,0xff262a31,0xff3a404a,0xff13161a,0xffe2e6ec,0xff95a0ad,0xffffa019,false}, // 9 MAGIX SEQUOIA
 {0xff191a1c,0xff121315,0xff232528,0xff34383d,0xff141517,0xffe2e6ec,0xff8a9492,0xff00f5b8,false}, // 10 ADOBE AUDITION
 {0xff171a1f,0xff111317,0xff21252d,0xff323844,0xff13161b,0xffe2e6ec,0xff848e9c,0xff00b7ff,false}, // 11 CUBASE 14
 {0xff242829,0xff1b1e1f,0xff2e3436,0xff3d4547,0xff181b1c,0xffe2e6ec,0xff8d9999,0xff35d0a3,false}, // 12 REAPER 7
 {0xff21212e,0xff181924,0xff2a2b3b,0xff525573,0xff080a08,0xff00ff00,0xffb5a67c,0xff00ff00,false}, // 13 WINAMP CLASSIC
 {0xffd6dad4,0xff101410,0xffe8ece6,0xff000000,0xffffffff,0xff000000,0xff303830,0xff000000,true},  // 14 ATARI PC (GEM TOS)
 {0xffc0c0c0,0xff000080,0xffc0c0c0,0xff808080,0xffffffff,0xff000000,0xff424242,0xff000080,true},  // 15 WINDOWS 98 (Default)
 {0xffdcdde0,0xffc8cacf,0xffeceef2,0xffa6acb5,0xffffffff,0xff111827,0xff525a66,0xff0066ee,true},  // 16 MAC OS X AQUA
 {0xff050506,0xff0a0a0c,0xff0b0b0e,0xff222226,0xff030304,0xffe2e6ec,0xff8e8e96,0xffffffff,false}, // 17 BLACK STEALTH
 {0xfff4f5f7,0xffe9ecf0,0xffffffff,0xffcfd6e0,0xffe2e6ec,0xff0f172a,0xff647080,0xff10b981,true}   // 18 WHITE STUDIO
};
inline const SkinPalette& pal(int id) noexcept {return kSkins[juce::jlimit(1,kTotalSkins,id)];}
juce::Colour ink(){return juce::Colour(pal(activeSkinId).txtBright);}
juce::Colour dim(){return juce::Colour(pal(activeSkinId).txtDim);}
void txt(juce::Graphics& g,juce::String s,float x,float y,float w,float h,float size=12,juce::Colour c=ink()) {if(compactPaint){size=juce::jmax(size,18.f);h=juce::jmax(h,22.f);}g.setColour(c);g.setFont(juce::Font(juce::FontOptions(activeSkinId>=4?"Segoe UI":"Arial",size,juce::Font::plain)));g.drawText(s,juce::Rectangle<float>(x,y,w,h),juce::Justification::centredLeft);}
float db(float x){return juce::Decibels::gainToDecibels(x,-100.f);}float clamp(float x){return juce::jlimit(-1.f,1.f,x);}
}
ScopeLook::ScopeLook(){setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffdddddd));setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101010));setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff292929));setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff101010));setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff333333));setColour(juce::ComboBox::textColourId,juce::Colour(0xffdddddd));setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff101010));setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff444444));setColour(juce::ToggleButton::textColourId,ink());setColour(juce::ToggleButton::tickColourId,juce::Colour(0xff36d477));setColour(juce::TextButton::buttonColourId,juce::Colour(0xff161616));setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff444444));}
void ScopeLook::setDiamond(bool enabled){diamond=enabled;const auto text=juce::Colour(diamond?0xff111519:0xffdddddd);setColour(juce::ToggleButton::textColourId,text);setColour(juce::ToggleButton::tickDisabledColourId,juce::Colour(diamond?0xff4b5054:0xff777777));setColour(juce::ToggleButton::tickColourId,juce::Colour(diamond?0xff641096:0xff36d477));setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffeeeeee));setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101010));setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(diamond?0xff777777:0xff292929));
 if(diamond&&!metalPanel.isValid()){metalPanel=juce::Image(juce::Image::RGB,900,540,true);juce::Graphics g(metalPanel);g.setGradientFill(juce::ColourGradient(juce::Colour(0xffeef0f0),0,0,juce::Colour(0xff8e9294),0,540,false));g.fillAll();for(int y=0;y<540;y++){g.setColour((y%3==0?juce::Colours::white:juce::Colours::black).withAlpha(y%3==0?.25f:.1f));g.drawHorizontalLine(y,1,899);}g.setColour(juce::Colours::white.withAlpha(.75f));g.drawRoundedRectangle(1,1,898,538,5,2);for(float x:{16.f,884.f})for(float y:{16.f,524.f}){g.setColour(juce::Colour(0xff44494b));g.fillEllipse(x-5,y-5,10,10);g.setColour(juce::Colour(0xff111517));g.fillEllipse(x-3,y-3,6,6);g.setColour(juce::Colour(0xff9a9fa0));g.drawLine(x-2,y+2,x+2,y-2,1);}}
 if(!purpleKnob.isValid()){auto source=juce::ImageCache::getFromMemory(OutputSkin::diamond_png,OutputSkin::diamond_pngSize);purpleKnob=source.getClippedImage({508,91,74,74}).createCopy();signatureDark=juce::Image(juce::Image::ARGB,162,51,true);signatureLight=juce::Image(juce::Image::ARGB,162,51,true);for(int y=0;y<51;y++)for(int x=0;x<162;x++){float alpha=juce::jlimit(0.f,1.f,(.62f-source.getPixelAt(1517+x,108+y).getBrightness())*2.5f);signatureDark.setPixelAt(x,y,juce::Colour(0xff050607).withAlpha(alpha));signatureLight.setPixelAt(x,y,juce::Colour(0xffdddddd).withAlpha(alpha));}}

}
void ScopeLook::setAds(bool enabled){ads=enabled;if(ads&&!adsPanel.isValid()){const auto photo=juce::ImageCache::getFromMemory(OutputSkin::ads_jpg,OutputSkin::ads_jpgSize);adsPanel=juce::Image(juce::Image::RGB,900,540,true);juce::Graphics g(adsPanel);g.fillAll(juce::Colour(0xff080808));g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImageWithin(photo,0,0,900,540,juce::RectanglePlacement::centred|juce::RectanglePlacement::onlyReduceInSize);}}
void ScopeLook::setSkinId(int id){
 skinId=juce::jlimit(1,kTotalSkins,id);
 setDiamond(skinId==2);
 setAds(skinId==3);
 setFlStudio(skinId>=4);
 const auto& sp=pal(skinId);
 setColour(juce::ComboBox::backgroundColourId,juce::Colour(sp.recBg));
 setColour(juce::ComboBox::outlineColourId,juce::Colour(sp.pnlBorder));
 setColour(juce::ComboBox::textColourId,juce::Colour(sp.txtBright));
 setColour(juce::ComboBox::arrowColourId,juce::Colour(sp.txtBright));
 setColour(juce::PopupMenu::backgroundColourId,juce::Colour(sp.recBg));
 setColour(juce::PopupMenu::textColourId,juce::Colour(sp.txtBright));
 setColour(juce::PopupMenu::headerTextColourId,juce::Colour(sp.txtDim));
 setColour(juce::PopupMenu::highlightedBackgroundColourId,skinId==6?juce::Colour(0xffffb000):skinId==14?juce::Colour(0xff000000):skinId==15?juce::Colour(0xff000080):skinId==16?juce::Colour(0xff0066ee):skinId==18?juce::Colour(0xff10b981):FLColours::highlightFill);
 setColour(juce::PopupMenu::highlightedTextColourId,skinId==6?juce::Colours::black:juce::Colours::white);
 setColour(juce::Slider::textBoxTextColourId,juce::Colour(skinId==2?0xffeeeeee:sp.txtBright));
 setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(sp.recBg));
 setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(sp.pnlBorder));
 setColour(juce::ToggleButton::textColourId,juce::Colour(sp.txtBright));
 setColour(juce::ToggleButton::tickDisabledColourId,juce::Colour(sp.txtDim));
 setColour(juce::ToggleButton::tickColourId,juce::Colour(sp.accent));
}
void ScopeLook::setFlStudio(bool enabled){flStudio=enabled;}
juce::Font ScopeLook::getTextButtonFont(juce::TextButton&,int){return juce::Font(juce::FontOptions(skinId==14?"Consolas":skinId>=4?"Segoe UI":"Arial",compact?20.f:14.f,juce::Font::plain));}
juce::Font ScopeLook::getComboBoxFont(juce::ComboBox&){return juce::Font(juce::FontOptions(skinId==14?"Consolas":skinId>=4?"Segoe UI":"Arial",compact?20.f:14.f,juce::Font::plain));}
juce::Font ScopeLook::getLabelFont(juce::Label&){return juce::Font(juce::FontOptions(skinId==14?"Consolas":skinId>=4?"Segoe UI":"Arial",compact?20.f:14.f,juce::Font::plain));}
juce::Font ScopeLook::getPopupMenuFont(){return juce::Font(juce::FontOptions("Segoe UI",13.f,juce::Font::plain));}
void ScopeLook::drawComboBox(juce::Graphics& g,int width,int height,bool isButtonDown,int buttonX,int buttonY,int buttonW,int buttonH,juce::ComboBox& box){
 if(skinId<4){juce::LookAndFeel_V4::drawComboBox(g,width,height,isButtonDown,buttonX,buttonY,buttonW,buttonH,box);return;}
 const auto& sp=pal(skinId);
 const juce::Rectangle<float> bounds(.5f,.5f,(float)width-1.f,(float)height-1.f);
 const bool square=(skinId==14||skinId==15);
 if(skinId==14){g.setColour(juce::Colours::black);g.fillRect(2,2,width-2,height-2);}
 g.setColour(juce::Colour(sp.recBg));g.fillRoundedRectangle(bounds,square?0.f:2.f);
 g.setColour(juce::Colour(sp.pnlBorder));g.drawRoundedRectangle(bounds,square?0.f:2.f,1.f);
 const float ax=(float)width-11.f,ay=(float)height*.5f;
 juce::Path arrow;arrow.addTriangle(ax-3.5f,ay-1.8f,ax+3.5f,ay-1.8f,ax,ay+2.5f);
 g.setColour(box.isEnabled()?juce::Colour(sp.txtBright):juce::Colour(sp.txtDim));g.fillPath(arrow);
}
void ScopeLook::positionComboBoxText(juce::ComboBox& box,juce::Label& label){
 if(skinId<4){juce::LookAndFeel_V4::positionComboBoxText(box,label);return;}
 label.setBounds(7,1,box.getWidth()-24,box.getHeight()-2);
 label.setFont(getComboBoxFont(box));
 label.setColour(juce::Label::textColourId,juce::Colour(pal(skinId).txtBright));
}
void ScopeLook::drawPopupMenuBackground(juce::Graphics& g,int width,int height){
 const auto& sp=pal(skinId);
 g.fillAll(juce::Colour(skinId==2?0xff101010:sp.recBg));
 g.setColour(juce::Colour(sp.pnlBorder));
 g.drawRect(0,0,width,height,1);
}
void ScopeLook::drawPopupMenuSectionHeader(juce::Graphics& g,const juce::Rectangle<int>& area,const juce::String& sectionName){
 const auto& sp=pal(skinId);
 g.setColour(skinId>=14&&sp.light?juce::Colour(sp.hdrBg):FLColours::sectionBg);g.fillRect(area);
 g.setColour((skinId==14||skinId==15)?juce::Colours::white:FLColours::sectionText);g.setFont(juce::Font(juce::FontOptions("Segoe UI",12.f,juce::Font::plain)));
 g.drawFittedText(sectionName,area.getX()+8,area.getY(),area.getWidth()-12,area.getHeight(),juce::Justification::centredLeft,1);
}
void ScopeLook::drawPopupMenuItem(juce::Graphics& g,const juce::Rectangle<int>& area,bool isSeparator,bool isActive,bool isHighlighted,bool isTicked,bool,const juce::String& text,const juce::String&,const juce::Drawable*,const juce::Colour*){
 const auto& sp=pal(skinId);
 if(isSeparator){g.setColour(juce::Colour(sp.pnlBorder));g.fillRect(area.reduced(4,0).withHeight(1));return;}
 if(!isActive){drawPopupMenuSectionHeader(g,area,text);return;}
 const auto hiBg=skinId==6?juce::Colour(0xffffb000):skinId==14?juce::Colour(0xff000000):skinId==15?juce::Colour(0xff000080):skinId==16?juce::Colour(0xff0066ee):skinId==18?juce::Colour(0xff10b981):FLColours::highlightFill;
 g.setColour(isHighlighted?hiBg:juce::Colour(skinId==2?0xff101010:sp.recBg));g.fillRect(area);
 g.setColour(isHighlighted?(skinId==6?juce::Colours::black:juce::Colours::white):juce::Colour(skinId==2?0xffdddddd:sp.txtBright));g.setFont(getPopupMenuFont());
 if(isTicked){const float dotY=(float)area.getCentreY();g.fillEllipse((float)(area.getX()+7),dotY-2.f,4.f,4.f);}
 g.drawFittedText(text,area.getX()+18,area.getY(),area.getWidth()-24,area.getHeight(),juce::Justification::centredLeft,1);
}
void ScopeLook::drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float position,float,float,juce::Slider::SliderStyle,juce::Slider&){
 const auto& sp=pal(skinId);
 float cx=x+width*.5f;g.setColour(juce::Colour(sp.pnlBorder));g.fillRect(cx-1,(float)y,2.f,(float)height);
 g.setColour(juce::Colour(sp.txtDim));g.drawLine(cx-5,y+height*.5f,cx+5,y+height*.5f,1);
 g.setColour(juce::Colour(sp.recBg));g.fillRoundedRectangle(cx-13,position-7,26,14,2);
 g.setColour(juce::Colour(skinId==2?0xffdddddd:sp.txtBright));g.drawRoundedRectangle(cx-13,position-7,26,14,2,1.2f);
 g.setColour(juce::Colour(sp.accent));g.drawLine(cx-8,position,cx+8,position,2);
}
void ScopeLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float a,float b,juce::Slider& slider){
 const float r=juce::jmin(w,h)*.41f,cx=x+w*.5f,cy=y+h*.5f;
 const auto& sp=pal(skinId);
 const float angle=a+pos*(b-a);
 const bool isBipolar=slider.getMinimum()<0.0;
 const float startA=isBipolar?(a+b)*.5f:a;
 const float a0=juce::jmin(startA,angle),a1=juce::jmax(startA,angle);
 if(!slider.isEnabled()){
  g.setColour(juce::Colour(sp.recBg));g.fillEllipse(cx-r,cy-r,2*r,2*r);
  g.setColour(juce::Colour(sp.pnlBorder));g.drawEllipse(cx-r,cy-r,2*r,2*r,1);
  g.drawLine(cx+std::sin(angle)*r*.60f,cy-std::cos(angle)*r*.60f,cx+std::sin(angle)*r*.88f,cy-std::cos(angle)*r*.88f,2);
  return;
 }
 if(diamond&&purpleKnob.isValid()){
  juce::Graphics::ScopedSaveState saved(g);juce::Path face;face.addEllipse(cx-r,cy-r,2*r,2*r);g.reduceClipRegion(face);
  g.addTransform(juce::AffineTransform::rotation(angle,cx,cy));g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
  g.drawImage(purpleKnob,cx-r,cy-r,2*r,2*r,0,0,purpleKnob.getWidth(),purpleKnob.getHeight());return;
 }
 if(skinId<=3){
  g.setColour(juce::Colour(0xff2b2b2b));g.drawEllipse(cx-r-5,cy-r-5,2*r+10,2*r+10,1);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff252525),cx-r,cy-r,juce::Colour(0xff080808),cx+r,cy+r,false));
  g.fillEllipse(cx-r,cy-r,2*r,2*r);g.setColour(juce::Colour(0xff454545));g.drawEllipse(cx-r,cy-r,2*r,2*r,1);
  g.setColour(juce::Colour(0xffdddddd));g.drawLine(cx+std::sin(angle)*r*.60f,cy-std::cos(angle)*r*.60f,cx+std::sin(angle)*r*.88f,cy-std::cos(angle)*r*.88f,2);
  return;
 }
 if(skinId==14){ // ATARI PC (GEM TOS / Cubase Atari ST monochrome retro dial with hard 2px shadow)
  g.setColour(juce::Colours::black);
  for(int t=0;t<=10;++t){float ta=a+(b-a)*((float)t/10.f);g.drawLine(cx+std::sin(ta)*(r+1.f),cy-std::cos(ta)*(r+1.f),cx+std::sin(ta)*(r+4.5f),cy-std::cos(ta)*(r+4.5f),1.3f);}
  g.fillEllipse(cx-r+2.5f,cy-r+2.5f,2*r,2*r);
  g.setColour(juce::Colours::white);g.fillEllipse(cx-r,cy-r,2*r,2*r);
  g.setColour(juce::Colours::black);g.drawEllipse(cx-r,cy-r,2*r,2*r,1.5f);
  g.drawEllipse(cx-r+3.f,cy-r+3.f,2*r-6.f,2*r-6.f,1.f);
  g.drawLine(cx,cy,cx+std::sin(angle)*r*.82f,cy-std::cos(angle)*r*.82f,2.4f);
  g.fillRect(cx-2.f,cy-2.f,4.f,4.f);
  return;
 }
 if(skinId==15){ // WINDOWS 98
  g.setColour(juce::Colour(0xff404040));
  for(int t=0;t<=8;++t){float ta=a+(b-a)*((float)t/8.f);g.drawLine(cx+std::sin(ta)*(r+1.f),cy-std::cos(ta)*(r+1.f),cx+std::sin(ta)*(r+4.f),cy-std::cos(ta)*(r+4.f),1.2f);}
  g.setColour(juce::Colour(0xff404040));g.fillEllipse(cx-r+1.5f,cy-r+1.5f,2*r,2*r);
  g.setColour(juce::Colours::white);g.fillEllipse(cx-r-1.2f,cy-r-1.2f,2*r,2*r);
  g.setColour(juce::Colour(0xffc0c0c0));g.fillEllipse(cx-r,cy-r,2*r,2*r);
  g.setColour(juce::Colour(0xff808080));g.drawEllipse(cx-r,cy-r,2*r,2*r,1.f);
  g.setColour(juce::Colour(0xff000080));g.drawLine(cx,cy,cx+std::sin(angle)*r*.82f,cy-std::cos(angle)*r*.82f,2.2f);
  return;
 }
 if(skinId==16){ // MAC OS X AQUA
  g.setColour(juce::Colour(0xffb4bac2));g.fillEllipse(cx-r-3,cy-r-3,2*r+6,2*r+6);
  if(std::abs(a1-a0)>.015f){juce::Path arc;arc.addCentredArc(cx,cy,r+1.5f,r+1.5f,0,a0,a1,true);g.setColour(juce::Colour(0xff0066ee));g.strokePath(arc,juce::PathStrokeType(2.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffdfeff),cx,cy-r,juce::Colour(0xffc9d0da),cx,cy+r,false));
  g.fillEllipse(cx-r,cy-r,2*r,2*r);g.setColour(juce::Colour(0xff7c8591));g.drawEllipse(cx-r,cy-r,2*r,2*r,1.f);
  g.setColour(juce::Colours::white.withAlpha(.75f));g.fillEllipse(cx-r*.55f,cy-r*.82f,r*1.1f,r*.55f);
  g.setColour(juce::Colour(0xff0066ee));g.drawLine(cx,cy,cx+std::sin(angle)*r*.82f,cy-std::cos(angle)*r*.82f,2.f);
  return;
 }
 if(skinId==8||skinId==13){ // PRO TOOLS (8) & WINAMP CLASSIC (13) 15-LED ring
  g.setColour(juce::Colour(skinId==13?0xff07080d:0xff14171a));g.fillEllipse(cx-r-4,cy-r-4,2*r+8,2*r+8);
  const int numLeds=15;
  for(int i=0;i<numLeds;++i){
   float u=(float)i/(float)(numLeds-1),la=a+u*(b-a);
   bool lit=(la>=a0-.03f&&la<=a1+.03f)&&(std::abs(angle-startA)>.015f);
   float lx=cx+std::sin(la)*(r+2.2f),ly=cy-std::cos(la)*(r+2.2f);
   juce::Colour ledCol=skinId==13?(u<.55f?juce::Colour(0xff00ff00):u<.82f?juce::Colour(0xffffb800):juce::Colour(0xffff2a00)):juce::Colour(0xff4eff4e);
   g.setColour(lit?ledCol:juce::Colour(skinId==13?0xff1b1d29:0xff171a1e));
   g.fillEllipse(lx-1.6f,ly-1.6f,3.2f,3.2f);
  }
  g.setGradientFill(juce::ColourGradient(juce::Colour(skinId==13?0xff6b708b:0xff59606b),cx,cy-r,juce::Colour(skinId==13?0xff232533:0xff292d33),cx,cy+r,false));
  g.fillEllipse(cx-r,cy-r,2*r,2*r);g.setColour(juce::Colour(skinId==13?0xffb59b52:0xff15171a));g.drawEllipse(cx-r,cy-r,2*r,2*r,1.1f);
  g.setColour(skinId==13?juce::Colour(0xff00ff00):juce::Colours::white);g.drawLine(cx,cy,cx+std::sin(angle)*r*.85f,cy-std::cos(angle)*r*.85f,2.f);
  return;
 }
 // All other skins (FL STUDIO, FL DARK, ABLETON LIVE, LOGIC PRO X, MAGIX SEQUOIA, ADOBE AUDITION, CUBASE 14, REAPER 7, BLACK STEALTH, WHITE STUDIO):
 g.setColour(juce::Colour(skinId==18?0xffe2e8f0:sp.recBg));g.fillEllipse(cx-r-3,cy-r-3,2*r+6,2*r+6);
 juce::Path bgArc;bgArc.addCentredArc(cx,cy,r+1.5f,r+1.5f,0,a,b,true);
 g.setColour(juce::Colour(sp.pnlBorder));g.strokePath(bgArc,juce::PathStrokeType(2.4f));
 if(std::abs(a1-a0)>.015f){juce::Path valArc;valArc.addCentredArc(cx,cy,r+1.5f,r+1.5f,0,a0,a1,true);g.setColour(juce::Colour(sp.accent));g.strokePath(valArc,juce::PathStrokeType(2.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
 const auto topCap=skinId==18?juce::Colour(0xffffffff):skinId==6?juce::Colour(0xff2b2b2b):juce::Colour(sp.pnlBg).brighter(.18f);
 const auto botCap=skinId==18?juce::Colour(0xfff1f5f9):skinId==6?juce::Colour(0xff2b2b2b):juce::Colour(sp.recBg);
 g.setGradientFill(juce::ColourGradient(topCap,cx-r,cy-r,botCap,cx+r,cy+r,false));
 g.fillEllipse(cx-r,cy-r,2*r,2*r);
 g.setColour(juce::Colour(sp.pnlBorder));g.drawEllipse(cx-r,cy-r,2*r,2*r,1.f);
 g.setColour(juce::Colour(skinId==18?sp.accent:sp.txtBright));
 g.drawLine(cx+std::sin(angle)*r*(skinId==6?0.f:.55f),cy-std::cos(angle)*r*(skinId==6?0.f:.55f),cx+std::sin(angle)*r*.88f,cy-std::cos(angle)*r*.88f,2.f);
}
Phobos67Editor::Phobos67Editor(Phobos67Processor& proc):AudioProcessorEditor(proc),p(proc){setLookAndFeel(&look);addAndMakeVisible(panel);p.scopeActive=true;
 const char* ids[]{"input","even","odd","effect","curve","air","sub","mid","side","output"};const char* names[]{"INPUT / dB","EVEN / %","ODD / %","INFLATE / AMOUNT","CURVE","AIR / AMOUNT","SUB / dB","MID / dB","SIDE / dB","OUTPUT / dB"};
 for(int i=0;i<10;i++){auto& k=knobs[i];panel.addAndMakeVisible(k);k.setName(ids[i]);k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,86,20);k.setScrollWheelEnabled(false);k.setPopupDisplayEnabled(true,true,this);k.setTooltip(p.state.getParameter(ids[i])->getName(80)+": double click resets");auto* par=p.state.getParameter(ids[i]);k.setDoubleClickReturnValue(true,par->convertFrom0to1(par->getDefaultValue()));sa.push_back(std::make_unique<SA>(p.state,ids[i],k));panel.addAndMakeVisible(labels[i]);labels[i].setText(names[i],juce::dontSendNotification);labels[i].setJustificationType(juce::Justification::centred);labels[i].setColour(juce::Label::textColourId,dim());}
 for(int index:{0,9}){knobs[index].setSliderStyle(juce::Slider::LinearVertical);knobs[index].setTextBoxStyle(juce::Slider::TextBoxBelow,false,86,20);}
 knobs[0].setTooltip("Input gain before IN meter. L/R bars: peak in dBFS, RMS in other continuous scales; markers/readouts: held peak dBFS.");knobs[9].setTooltip("Final Output gain. L/R bars: peak in dBFS, RMS in other continuous scales; markers/readouts: held peak dBFS.");
 const char* bids[]{"volumeComp","clip","bandSplit","mono","delta","reverseOrder"};const char* bnames[]{"COMP VOL","CLIP 0","BAND SPLIT","MONO","DELTA",""};
 for(int i=0;i<5;i++){panel.addAndMakeVisible(switches[i]);switches[i].setButtonText(bnames[i]);ba.push_back(std::make_unique<BA>(p.state,bids[i],switches[i]));}
 switches[0].setTooltip("Volume compensation: feed-forward RMS match to original input, +/-12 dB. 500ms signal warm-up; makeup rises at most 2 dB/s. Below -60 dBFS input RMS, returns to unity instead of boosting noise. Suspended for Delta and solo monitoring. Does not alter SCULP/INFLATE settings.");
 switches[1].setTooltip("Hard clipping at 0 dBFS. Clipping peaks inherently generates distortion. No additional colour stage.");
 for(int i=0;i<2;i++){auto& c=frequencies[i];panel.addAndMakeVisible(c);c.setScrollWheelEnabled(true);c.setTooltip("Mouse wheel: one preset per discrete event. Trackpad: accumulates small movements, ignores inertia. Stops at endpoints; disabled with SCULP. Hz changes crossfade over 20 ms.");c.addItemList(i==0?juce::StringArray{"50 kHz","55 kHz","60 kHz","65 kHz"}:juce::StringArray{"40 Hz","50 Hz","60 Hz","100 Hz"},1);ca.push_back(std::make_unique<CA>(p.state,i==0?"airFreq":"subFreq",c));}

 panel.addAndMakeVisible(oversampling);oversampling.setName("OVERSAMPLING");oversampling.addSectionHeading("OVERSAMPLING");oversampling.addItemList({"OS 1x / OFF","OS 2x","OS 4x","OS 8x"},1);oversampling.setSelectedId(p.oversamplingChoice()+1,juce::dontSendNotification);oversampling.onChange=[this]{p.requestOversampling(oversampling.getSelectedId()-1);};oversampling.setTooltip("Oversampling 1x/2x/4x/8x. Default 4x preserves the original engine. Lower values save CPU; higher values reduce aliasing in SCULP. INFLATE retains its native processing. Changing this setting briefly suspends processing and updates host latency; saved with the session. At low rates, Air design is limited to internal Nyquist.");
 panel.addAndMakeVisible(order);order.setName("stageOrder");order.addItemList({"PRE","POST"},1);ca.push_back(std::make_unique<CA>(p.state,"reverseOrder",order));order.setTooltip("PRE: SCULP before INFLATE. POST: SCULP after INFLATE.");
 for(auto* button:{&sculpPower,&inflatePower,&bypassButton})panel.addAndMakeVisible(*button);ba.push_back(std::make_unique<BA>(p.state,"sculpEnabled",sculpPower));ba.push_back(std::make_unique<BA>(p.state,"inflateEnabled",inflatePower));sculpPower.setTooltip("SCULP on/off; settings retained, 5ms transition, latency unchanged.");inflatePower.setTooltip("INFLATE on/off; final Clip is separate. Settings retained, 5ms transition.");
 bypassButton.setTooltip("BYPASS: bypasses the audio engine with latency-compensated dry signal.");
 bypassButton.onClick=[this]{if(restoringUi)return;if(auto* par=p.state.getParameter("power")){par->beginChangeGesture();par->setValueNotifyingHost(bypassButton.getToggleState()?0.f:1.f);par->endChangeGesture();}updateStates();repaint();};
 panel.addMouseListener(this,true);
 for(int i=0;i<10;i++){auto& k=knobs[i];k.setVelocityModeParameters(.25,1,0,true,juce::ModifierKeys::shiftModifier);k.setWantsKeyboardFocus(true);k.setExplicitFocusOrder(i+1);bool gain=i==0||i>=6,percent=i==1||i==2||i==3;k.textFromValueFunction=[gain,percent](double v){return juce::String(v,gain||percent?1:2)+(gain?" dB":percent?" %":"");};k.valueFromTextFunction=[](const juce::String& text){return text.trim().getDoubleValue();};k.updateText();k.setTooltip(k.getTooltip()+" / Shift-drag: fine adjustment. Tab: next control. Arrow keys: step.");}
 for(auto* help:{&harmonicMetersTip,&stereoMetersTip})panel.addAndMakeVisible(*help);harmonicMetersTip.setTooltip("DENSITY L/R: relative RMS of the nonlinear Even/Odd contribution versus its linear contribution, mapped to 0-100%. 300ms average. Includes DC/fundamental components; not THD or a laboratory harmonic analyser. Zero with SCULP off.");stereoMetersTip.setTooltip("Final OUTPUT, AC-coupled at 5 Hz: L/R correlation -1..+1 and Side-energy percentage S/(M+S). 0% is mono; 50% can be uncorrelated or one-sided audio; 100% is anti-phase. No reading on silence. Not a certified stereo-width value.");
 panel.addChildComponent(skin);skin.setName("SKIN");skin.addSectionHeading("SKIN");
 skin.addItemList({"BLACK","DIAMOND II","Ads","FL STUDIO","FL DARK","ABLETON LIVE","LOGIC PRO X","PRO TOOLS","MAGIX SEQUOIA","ADOBE AUDITION","CUBASE 14","REAPER 7","WINAMP CLASSIC","ATARI PC","WINDOWS 98","MAC OS X AQUA","BLACK STEALTH","WHITE STUDIO"},1);
 skin.setTooltip("Visual skin only. Right-click the papielguapo signature to change skin.");skin.onChange=[this]{applySkin();saveUi();};
 panel.addAndMakeVisible(resampling);resampling.setName("RESAMPLING");
 if(auto* rootMenu=resampling.getRootMenu()){
  rootMenu->addItem(1,"RESAMPLING OFF");
  rootMenu->addSectionHeader("DAW");
  rootMenu->addItem(2,"Native Daw");
  rootMenu->addSectionHeader("Low quality / fastest");
  rootMenu->addItem(3,"2-point linear");
  rootMenu->addItem(4,"6-point Hermite");
  rootMenu->addSectionHeader("Average quality");
  rootMenu->addItem(5,"16-point sinc");
  rootMenu->addItem(6,"24-point sinc");
  rootMenu->addItem(7,"32-point sinc");
  rootMenu->addSectionHeader("High quality / slowest");
  rootMenu->addItem(8,"64-point sinc");
  rootMenu->addItem(9,"128-point sinc");
  rootMenu->addItem(10,"256-point sinc");
  rootMenu->addItem(11,"512-point sinc");
 }
 ca.push_back(std::make_unique<CA>(p.state,"resampling",resampling));
 resampling.setTooltip("FL Studio 1:1 FLEngine_x64.dll Resampling quality (2-point linear, 6-point Hermite, 16..512-point sinc) or OFF.");
 panel.addAndMakeVisible(scale);scale.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff181818));scale.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff353535));
 scale.setName("I/O SCALES");scale.addSectionHeading("I/O SCALES");scale.addItemList({"dBFS / PEAK","LINEAR / RMS","VU / -18 dBFS","K-12 / RMS","K-14 / RMS","K-20 / RMS","SCULP SWEET SPOT"},1);scale.setSelectedId(1,juce::dontSendNotification);scale.onChange=[this]{saveUi();repaint();};scale.setTooltip("Choose the Input/Output meter scale directly. Peak readouts stay in dBFS, held 2 seconds. Changes the display only. Click the Input/Output peak numbers to clear retained peaks/OVER.");
 switches[3].setTooltip("MONO: listen to the equal-channel mono sum.");switches[4].setTooltip("DELTA: listen to processed minus latency-aligned original. COMP VOL is suspended.");
 knobs[3].setTooltip("Inflate amount: density/saturation, not a conventional compressor.");knobs[5].setTooltip("Air amount 0-10; internal shelf boost 0-20 dB, not direct audible dB. Design frequencies 50/55/60/65 kHz.");
 setWantsKeyboardFocus(true);setResizable(true,true);getConstrainer()->setFixedAspectRatio(900.0/540.0);setResizeLimits(450,270,1350,810);setSize(450,270);restoringUi=false;restoreUi();startTimerHz(20);
}
Phobos67Editor::~Phobos67Editor(){if(seenUiRevision==p.uiRevision.load())saveUi();stopTimer();p.scopeActive=false;setLookAndFeel(nullptr);}
void Phobos67Editor::resized(){look.compact=getWidth()<=600;panel.setBounds(0,0,900,540);panel.setTransform(juce::AffineTransform::scale(getWidth()/900.f));order.setBounds(762,434,110,26);
 for(int i=0;i<10;i++){int x=28+(i%5)*172,y=603+(i/5)*119;labels[i].setBounds(x,y,156,20);labels[i].setFont(juce::Font(juce::FontOptions(look.compact?20.f:14.f)));const char* shortNames[]{"INPUT","EVEN","ODD","INFLATE","CURVE","AIR","SUB","MID","SIDE","OUTPUT"};if(look.compact)labels[i].setText(shortNames[i],juce::dontSendNotification);else {const char* longNames[]{"INPUT / dB","EVEN / %","ODD / %","INFLATE / AMOUNT","CURVE","AIR / AMOUNT","SUB / dB","MID / dB","SIDE / dB","OUTPUT / dB"};labels[i].setText(longNames[i],juce::dontSendNotification);}knobs[i].setBounds(x+27,y+20,102,92);}
 const int moduleX[]{28,140,298,456,614,772};const int moduleW[]{102,148,148,148,148,100};
 auto place=[&](int index,int module,int y,int height){int width=moduleW[module];labels[index].setBounds(moduleX[module]+3,y,width-6,20);knobs[index].setBounds(moduleX[module]+(width-102)/2,y+20,102,height);};
 place(0,0,491,286);knobs[0].setBounds(36,555,86,290);
 place(1,1,497,124);place(2,1,681,136);
 place(6,2,497,124);place(5,2,681,136);
 place(7,3,497,124);place(8,3,681,136);
 place(4,4,497,124);place(3,4,681,136);
 place(9,5,491,286);knobs[9].setBounds(779,555,86,290);
 frequencies[1].setBounds(310,644,124,22);frequencies[0].setBounds(310,839,124,22);
 switches[3].setBounds(30,461,105,21);
 switches[4].setBounds(145,461,110,21);
 switches[1].setBounds(265,461,118,21);
 switches[0].setBounds(392,461,135,21);
 bypassButton.setBounds(740,461,132,21);
 switches[2].setBounds(622,838,130,20);
 for(auto& k:knobs)k.setTopLeftPosition(k.getX(),k.getY()-360);for(auto& l:labels)l.setTopLeftPosition(l.getX(),l.getY()-360);for(auto& c:frequencies)c.setTopLeftPosition(c.getX(),c.getY()-360);for(auto& b:switches)b.setTopLeftPosition(b.getX(),b.getY()-360);bypassButton.setTopLeftPosition(bypassButton.getX(),bypassButton.getY()-360);order.setTopLeftPosition(order.getX(),order.getY()-360);
 oversampling.setBounds(734,27,138,28);scale.setBounds(346,27,172,28);skin.setBounds(526,27,200,28);resampling.setBounds(526,27,200,28);harmonicMetersTip.setBounds(148,282,136,39);stereoMetersTip.setBounds(464,282,136,39);sculpPower.setBounds(454,74,108,26);inflatePower.setBounds(566,74,130,26);if(!restoringUi)saveUi();
}
void Phobos67Editor::timerCallback(){oversampling.setSelectedId(p.oversamplingChoice()+1,juce::dontSendNotification);
 if(seenUiRevision!=p.uiRevision.load())restoreUi();updateStates();
 const double phaseDcAlpha=1-std::exp(-juce::MathConstants<double>::twoPi*5/juce::jmax(1.,p.getSampleRate()));double energies[4]{},outputCross=0,phaseLeftEnergy=0,phaseRightEnergy=0,midEnergy=0,sideEnergy=0;float peaks[4]{};int a,n,b,m;const int count=p.scopeFifo.getNumReady();p.scopeFifo.prepareToRead(count,a,n,b,m);
 const auto read=[&](int start,int size){for(int i=0;i<size;i++){const auto& s=p.scopeData[(size_t)(start+i)];const float values[]{s.input,s.rightIn,s.output,s.rightOut};for(int ch=0;ch<4;ch++){energies[ch]+=(double)values[ch]*values[ch];peaks[ch]=juce::jmax(peaks[ch],std::abs(values[ch]));}phaseDcLeft+=phaseDcAlpha*(s.output-phaseDcLeft);phaseDcRight+=phaseDcAlpha*(s.rightOut-phaseDcRight);double left=s.output-phaseDcLeft,right=s.rightOut-phaseDcRight;phaseLeftEnergy+=left*left;phaseRightEnergy+=right*right;outputCross+=left*right;double mid=(left+right)*.5,side=(left-right)*.5;midEnergy+=mid*mid;sideEnergy+=side*side;}};read(a,n);read(b,m);p.scopeFifo.finishedRead(n+m);
 const double now=juce::Time::getMillisecondCounterHiRes();const float elapsed=meterTickMs>0?(float)((now-meterTickMs)*.001):.05f;meterTickMs=now;if(count>0)lastData=now;correlationValid=count>0&&p.getTotalNumOutputChannels()>1&&phaseLeftEnergy>count*1e-12&&phaseRightEnergy>count*1e-12;stereoValid=count>0&&midEnergy+sideEnergy>count*1e-12;if(correlationValid)outputCorrelation=juce::jlimit(-1.f,1.f,(float)(outputCross/std::sqrt(phaseLeftEnergy*phaseRightEnergy)));if(stereoValid)outputSidePercent=juce::jlimit(0.f,100.f,(float)(100*sideEnergy/(midEnergy+sideEnergy)));
 if(count>0||now-lastData>250){for(int ch=0;ch<2;ch++){rmsInputLR[ch]=count?(float)std::sqrt(energies[ch]/count):0; rmsOutputLR[ch]=count?(float)std::sqrt(energies[ch+2]/count):0;peakInputLR[ch]=peaks[ch];peakOutputLR[ch]=peaks[ch+2];}}
 const float alpha=1.f-std::exp(-elapsed/.3f),release=std::pow(10.f,-1.5f*elapsed);for(int ch=0;ch<2;ch++){if(peakInputLR[ch]>1.f)overload[ch]=true;if(peakOutputLR[ch]>1.f)overload[ch+2]=true;vuInputLR[ch]+=alpha*(rmsInputLR[ch]-vuInputLR[ch]);vuOutputLR[ch]+=alpha*(rmsOutputLR[ch]-vuOutputLR[ch]);meterPeaks[ch].update(peakInputLR[ch],elapsed);meterPeaks[ch+2].update(peakOutputLR[ch],elapsed);barPeaks[ch]=juce::jmax(peakInputLR[ch],barPeaks[ch]*release);barPeaks[ch+2]=juce::jmax(peakOutputLR[ch],barPeaks[ch+2]*release);compDisplayLR[ch]=p.compActivityLR[ch].load();harmonicDisplay[ch]=(now-lastData<250&&rmsInputLR[ch]>1e-7f)?p.harmonicDensityLR[ch].load(std::memory_order_relaxed):0;}repaint();
}
void Phobos67Editor::paint(juce::Graphics& g){
 compactPaint=getWidth()<=600;diamondPaint=look.diamond;adsPaint=look.ads;flPaint=look.flStudio;activeSkinId=look.skinId;
 const auto& sp=pal(look.skinId);
 g.fillAll(juce::Colour(sp.winBg));
 g.addTransform(juce::AffineTransform::scale(getWidth()/900.f));
 if(look.diamond)g.drawImageAt(look.metalPanel,0,0);
 if(look.ads){g.drawImageAt(look.adsPanel,0,0);g.setColour(juce::Colours::black.withAlpha(.62f));g.fillRect(8,8,884,112);g.fillRect(8,501,884,31);const int x[]{28,140,298,456,614,772},w[]{102,148,148,148,148,100};g.setColour(juce::Colours::black.withAlpha(.5f));for(int i=0;i<6;i++)g.fillRoundedRectangle((float)x[i],123.f,(float)w[i],378.f,5.f);}
 if(look.skinId>=4){
  const bool square=(look.skinId==14||look.skinId==15);
  g.setColour(juce::Colour(sp.hdrBg));
  g.fillRoundedRectangle(8.f,8.f,884.f,57.f,square?0.f:6.f);
  const int x[]{28,140,298,456,614,772},w[]{102,148,148,148,148,100};
  if(look.skinId==14){g.setColour(juce::Colours::black);for(int i=0;i<6;i++)g.fillRect(x[i]+3,126,w[i],378);}
  g.setColour(juce::Colour(sp.pnlBg));
  for(int i=0;i<6;i++)g.fillRoundedRectangle((float)x[i],123.f,(float)w[i],378.f,square?0.f:4.f);
 }
 const bool square=(look.skinId==14||look.skinId==15);
 g.setColour(juce::Colour(look.skinId==1?0xff171717:sp.pnlBorder));
 g.drawRoundedRectangle(8,8,884,524.f,square?0.f:12.f,look.skinId==14?2.f:1.f);
 txt(g,"OUTPUT",28,15,200,43,34,(look.skinId==14||look.skinId==15)?juce::Colours::white:ink());
 g.drawImage((sp.light&&look.skinId!=14&&look.skinId!=15)?look.signatureDark:look.signatureLight,juce::Rectangle<float>(228,23,108,34));
 g.setColour(juce::Colour(look.skinId==1?0xff252525:sp.pnlBorder));
 g.drawHorizontalLine(65,28,872);
 g.addTransform(juce::AffineTransform::translation(0,-360));
 drawMeters(g);drawModuleMeters(g);drawStatus(g);
 g.setColour(juce::Colour(sp.pnlBorder));
 const float moduleX[]{28,140,298,456,614,772},moduleW[]{102,148,148,148,148,100};
 for(int i=0;i<6;i++)g.drawRoundedRectangle(moduleX[i],483,moduleW[i],378,square?0.f:5.f,1.2f);
 g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(12.f)));g.drawText("MOD %",juce::Rectangle<float>(624,642,120,17),juce::Justification::centred);
 for(int ch=0;ch<2;ch++){float y=662+ch*10.f,activity=compDisplayLR[ch]/100.f;g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(11.f)));g.drawText(ch?"R":"L",juce::Rectangle<float>(622,y-3,14,12),juce::Justification::centred);g.setColour(juce::Colour(0xff1b1b1b));g.fillRect(638.f,y,94.f,7.f);for(int zone=0;zone<4;zone++){float from=zone*.25f,to=juce::jmin(activity,from+.25f);if(to>from){const juce::uint32 colours[]{0xff36d477,0xffffda36,0xffff982a,0xffff3838};g.setColour(juce::Colour(colours[zone]));g.fillRect(638+94*from,y,94*(to-from),7.f);}}}
 txt(g,"SCULP",700,434,62,26,11,dim());
 txt(g,p.state.getRawParameterValue("reverseOrder")->load()>.5f?"IN > INFLATE > SCULP > OUT / CLIP > COMP VOL":"IN > SCULP > INFLATE > OUT / CLIP > COMP VOL",28,434,424,26,12.5f,ink());
 g.setColour(juce::Colour(0xff292929));
}
void Phobos67Editor::drawMeters(juce::Graphics& g){
 const int mode=scale.getSelectedId();const bool sweet=mode==7;const float offset=meterOffset(mode);
 const auto signedDb=[](float value){return value<=-99.9f?juce::String("-inf"):(value>0?juce::String("+"):juce::String())+juce::String(value,1);};
 const auto pos=[&](float amplitude){return meterPosition(mode,amplitude);};
 const auto atDb=[&](float d){return pos(juce::Decibels::decibelsToGain(d));};
 const float top=577,bottom=805,height=bottom-top;
 const juce::Colour green(0xff36d477),yellow(0xffffda36),orange(0xffff982a),red(0xffff3838);
 for(int group=0;group<2;group++){
  const float x=group?772.f:28.f;g.setColour((overload[group*2]||overload[group*2+1])?red:dim());g.setFont(juce::Font(juce::FontOptions(12.f)));g.drawText((overload[group*2]||overload[group*2+1])?"OVER / dBFS":"PK 2s / dBFS",juce::Rectangle<float>(x+3,512,96,14),juce::Justification::centred);
  for(int channel=0;channel<2;channel++){
   const bool present=channel==0||(group?p.getTotalNumOutputChannels():p.getTotalNumInputChannels())>1;
   const float amplitude=group?(mode==3?vuOutputLR[channel]:rmsOutputLR[channel]):(mode==3?vuInputLR[channel]:rmsInputLR[channel]);
   const float livePeak=group?peakOutputLR[channel]:peakInputLR[channel];
   const float peak=juce::jmax(livePeak,meterPeaks[group*2+channel].value),peakDb=db(peak);
   const float barX=x+(channel?66:14),barWidth=12;
   txt(g,channel?"R":"L",barX-3,552,20,22,18,present?ink():dim());
   g.setColour(juce::Colour(0xff1b1b1b));g.fillRect(barX,top,barWidth,height);
   if(present){
    if(sweet){int lit=sweetSpotLedCount(db(livePeak));for(int led=0;led<9;led++){float y=bottom-height*(led+1)/9.f;g.setColour(juce::Colour(0xff1b1b1b));g.fillRect(barX,y+1,barWidth,height/9-2);if(led<lit){g.setColour(led<2?green:led<4?yellow:led<5?orange:red);g.fillRect(barX,y+1,barWidth,height/9-2);}}g.setColour(ink());g.drawRect(juce::Rectangle<float>(barX-1,bottom-height*2/9.f,barWidth+2,height/9.f),1.f);}
    else {float fill=pos(mode==1?juce::jmax(livePeak,barPeaks[group*2+channel]):amplitude);
     const auto segment=[&](float from,float to,juce::Colour colour){float end=juce::jmin(fill,to);if(end>from){g.setColour(colour);g.fillRect(barX,bottom-height*end,barWidth,height*(end-from));}};
     segment(0,atDb(-12),green);segment(atDb(-12),atDb(-3),yellow);segment(atDb(-3),atDb(0),orange);segment(atDb(0),1,red);
    }
    // Full scale is always absolute 0 dBFS, including VU and K references.
    g.setColour(juce::Colour(0xffeeeeee));g.fillRect(barX-2,bottom-height*atDb(0)-1,barWidth+4,2.f);
    if(mode>=3&&mode<=6){g.setColour(juce::Colour(0xff999999));g.drawHorizontalLine(juce::roundToInt(bottom-height*atDb(-offset)),barX-1,barX+barWidth+1);}
    if(peak>0){float py=bottom-height*pos(peak);g.setColour(peakDb>0?red:juce::Colours::white);g.fillRect(barX-2,py-1,barWidth+4,2.f);}
   }
   float readoutX=x+(channel?54:4);g.setColour(juce::Colour(0xff101010));g.fillRect(readoutX,528.f,44.f,22.f);g.setColour(present&&(peakDb>0||overload[group*2+channel])?red:juce::Colour(0xffdddddd));g.setFont(juce::Font(juce::FontOptions(compactPaint?17.f:16.f)));if(present&&overload[group*2+channel])g.drawRect(juce::Rectangle<float>(readoutX,528,44,22),1.f);g.drawText(present?signedDb(peakDb):juce::String("--"),juce::Rectangle<float>(readoutX,528,44,22),juce::Justification::centred);
  }
  auto tick=[&](float absoluteDb,const juce::String& label,bool emphasis=false){float y=bottom-height*atDb(absoluteDb);g.setColour(emphasis?ink():dim());g.setFont(juce::Font(juce::FontOptions(compactPaint?14.f:12.f)));g.drawText(label,juce::Rectangle<float>(x+79,y-8,23,16),juce::Justification::centred);};
  if(sweet){for(int led=0;led<9;led++){float value=-15+led*3.f;tick(value,led==1?juce::String("SP"):(value>0?juce::String("+"):juce::String())+juce::String((int)value),led==1||value==0);}}
  else if(mode==2){tick(-100,"0");tick(0,"1",true);tick(6.0206f,"2");tick(9,"2.8");}
  else if(mode>=3&&mode<=6){tick(-12-offset,"-12");tick(-offset,"0",true);tick(0,"FS",true);tick(9,"+9");}
  else {tick(-60,"-60");tick(-48,"-48");tick(-36,"-36");tick(-24,"-24");tick(-12,"-12");tick(0,"0",true);tick(9,"+9");}
  const char* captions[]{"","PEAK","RMS LIN","VU -18","K-12","K-14","K-20","SP -12"};
  g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(13.f)));g.drawText(captions[juce::jlimit(1,7,mode)],juce::Rectangle<float>(x+2,806,98,18),juce::Justification::centred);
 }
}

juce::String Phobos67Editor::monitorStatus() const {if(p.state.getRawParameterValue("power")->load()<.5f)return "ENGINE BYPASS / COMP SUSPENDED";juce::String text;int listen=juce::roundToInt(p.state.getRawParameterValue("listen")->load());bool delta=p.state.getRawParameterValue("delta")->load()>.5f,mono=p.state.getRawParameterValue("mono")->load()>.5f,comp=p.state.getRawParameterValue("volumeComp")->load()>.5f;if(listen){const char* names[]{"NORMAL","MID SOLO","SIDE SOLO","CLIP REMOVED"};text="LISTEN: "+juce::String(names[juce::jlimit(0,3,listen)]);}else if(delta)text="DELTA";else if(mono)text="MONO";if(mono&&(listen||delta))text="MONO+"+text;if(comp){if(text.isNotEmpty())text+=" / ";text+=(listen||delta)?"COMP SUSPENDED":"COMP "+juce::String(p.volumeMatchDb.load(),1)+" dB";}else if(text.isEmpty())text="COMP OFF";return text;}
void Phobos67Editor::updateStates(){
 const auto& sp=pal(look.skinId);
 sculpPower.setButtonText(p.state.getRawParameterValue("sculpEnabled")->load()>.5f?"SCULP ON":"SCULP OFF");
 inflatePower.setButtonText(p.state.getRawParameterValue("inflateEnabled")->load()>.5f?"INFLATE ON":"INFLATE OFF");
 bool engine=p.state.getRawParameterValue("power")->load()>.5f,sculp=engine&&p.state.getRawParameterValue("sculpEnabled")->load()>.5f,inflate=engine&&p.state.getRawParameterValue("inflateEnabled")->load()>.5f;
 if(bypassButton.getToggleState()!=!engine)bypassButton.setToggleState(!engine,juce::dontSendNotification);
 bypassButton.setColour(juce::ToggleButton::textColourId,bypassButton.getToggleState()?juce::Colour(sp.light?0xffd97706:0xffffda36):juce::Colour(sp.txtBright));
 for(int index:{1,2,5,6,7,8}){knobs[index].setEnabled(sculp);labels[index].setAlpha(sculp?1.f:.4f);}
 for(auto& f:frequencies){f.setEnabled(sculp);f.setScrollWheelEnabled(sculp);}
 for(int index:{3,4}){knobs[index].setEnabled(inflate);labels[index].setAlpha(inflate?1.f:.4f);}
 switches[2].setEnabled(inflate);resampling.setEnabled(engine);
 for(int index:{0,3,4,1}){auto& button=switches[index];button.setColour(juce::ToggleButton::textColourId,button.getToggleState()?juce::Colour(sp.accent):juce::Colour(sp.txtBright));}
}
void Phobos67Editor::drawStatus(juce::Graphics& g){
 const auto& sp=pal(look.skinId);
 const juce::Rectangle<float> box(530.f,461.f,200.f,21.f);
 const bool square=(look.skinId==14||look.skinId==15);
 g.setColour(look.diamond?juce::Colour(0x33000000):juce::Colour(sp.recBg));
 g.fillRoundedRectangle(box,square?0.f:3.f);
 g.setColour(juce::Colour(sp.pnlBorder));
 g.drawRoundedRectangle(box,square?0.f:3.f,1.f);
 const bool alert=p.state.getRawParameterValue("power")->load()<.5f||p.state.getRawParameterValue("delta")->load()>.5f||p.state.getRawParameterValue("listen")->load()>.5f;
 const bool active=p.state.getRawParameterValue("volumeComp")->load()>.5f&&!alert;
 g.setColour(alert?juce::Colour(sp.light?0xffd97706:0xffffda36):active?juce::Colour(sp.accent):dim());
 g.setFont(juce::Font(juce::FontOptions(look.skinId>=4?"Segoe UI":"Arial",compactPaint?14.f:11.5f,juce::Font::plain)));
 g.drawFittedText(monitorStatus(),box.reduced(6.f,1.f).toNearestInt(),juce::Justification::centred,1);
}
void Phobos67Editor::saveUi(){if(restoringUi)return;p.ui[Phobos67Processor::Width]=getWidth();p.ui[Phobos67Processor::Scale]=scale.getSelectedId();p.ui[Phobos67Processor::Theme]=juce::jlimit(0,kTotalSkins-1,skin.getSelectedId()-1);}
void Phobos67Editor::restoreUi(){restoringUi=true;seenUiRevision=p.uiRevision.load();oversampling.setSelectedId(p.oversamplingChoice()+1,juce::dontSendNotification);scale.setSelectedId(juce::jlimit(1,7,p.ui[Phobos67Processor::Scale].load()),juce::dontSendNotification);skin.setSelectedId(juce::jlimit(0,kTotalSkins-1,p.ui[Phobos67Processor::Theme].load())+1,juce::dontSendNotification);applySkin();int w=juce::jlimit(450,1350,p.ui[Phobos67Processor::Width].load());setSize(w,juce::roundToInt(w*.6));resized();updateStates();restoringUi=false;repaint();}

juce::File Phobos67Editor::userSculpPresetsDir(){
 auto dir=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("papielguapo").getChildFile("OUTPUT").getChildFile("Presets");
 if(!dir.exists())dir.createDirectory();
 return dir;
}
void Phobos67Editor::applySculpPreset(const SculpPresetData& pr){
 auto setParam=[this](const char* id,float val){
  if(auto* par=p.state.getParameter(id)){
   par->beginChangeGesture();
   par->setValueNotifyingHost(par->convertTo0to1(juce::jlimit(par->getNormalisableRange().start,par->getNormalisableRange().end,val)));
   par->endChangeGesture();
  }
 };
 p.state.copyState();
 p.undoManager.beginNewTransaction("Load Sculp Preset");
 setParam("sculpEnabled",1.f);
 setParam("even",pr.even);
 setParam("odd",pr.odd);
 setParam("sub",pr.sub);
 setParam("subFreq",pr.subFreq);
 setParam("air",pr.air);
 setParam("airFreq",pr.airFreq);
 setParam("mid",pr.mid);
 setParam("side",pr.side);
 if(!p.ui[Phobos67Processor::LockInput].load())setParam("input",pr.input);
 if(!p.ui[Phobos67Processor::LockOutput].load())setParam("output",pr.output);
 p.state.copyState();
 p.undoManager.beginNewTransaction();
 updateStates();repaint();
}
bool Phobos67Editor::saveUserSculpPresetFile(const juce::File& file){
 juce::XmlElement xml("SCULP_PRESET");
 const auto val=[this](const char* id){return p.state.getRawParameterValue(id)->load();};
 xml.setAttribute("even",val("even"));
 xml.setAttribute("odd",val("odd"));
 xml.setAttribute("sub",val("sub"));
 xml.setAttribute("subFreq",val("subFreq"));
 xml.setAttribute("air",val("air"));
 xml.setAttribute("airFreq",val("airFreq"));
 xml.setAttribute("mid",val("mid"));
 xml.setAttribute("side",val("side"));
 xml.setAttribute("input",val("input"));
 xml.setAttribute("output",val("output"));
 return xml.writeTo(file);
}
bool Phobos67Editor::loadUserSculpPresetFile(const juce::File& file){
 auto xml=juce::XmlDocument::parse(file);
 if(!xml)return false;
 SculpPresetData pr;
 pr.name=file.getFileNameWithoutExtension();
 if(xml->hasTagName("SCULP_PRESET")){
  pr.even=(float)xml->getDoubleAttribute("even",0.0);
  pr.odd=(float)xml->getDoubleAttribute("odd",0.0);
  pr.sub=(float)xml->getDoubleAttribute("sub",0.0);
  pr.subFreq=(float)xml->getDoubleAttribute("subFreq",2.0);
  pr.air=(float)xml->getDoubleAttribute("air",0.0);
  pr.airFreq=(float)xml->getDoubleAttribute("airFreq",0.0);
  pr.mid=(float)xml->getDoubleAttribute("mid",0.0);
  pr.side=(float)xml->getDoubleAttribute("side",0.0);
  pr.input=(float)xml->getDoubleAttribute("input",0.0);
  pr.output=(float)xml->getDoubleAttribute("output",0.0);
  applySculpPreset(pr);
  return true;
 }
 if(xml->hasTagName("Parameters")){
  for(auto* child:xml->getChildIterator()){
   const auto id=child->getStringAttribute("id");
   const float v=(float)child->getDoubleAttribute("value",0.0);
   if(id=="EvenGain")pr.even=(float)juce::roundToInt(v*100.f);
   else if(id=="OddGain")pr.odd=(float)juce::roundToInt(v*100.f);
   else if(id=="SubGain")pr.sub=v;
   else if(id=="SubFreq")pr.subFreq=v;
   else if(id=="AirGain")pr.air=v;
   else if(id=="AirFreq")pr.airFreq=v;
   else if(id=="MidGain")pr.mid=v;
   else if(id=="SideGain")pr.side=v;
   else if(id=="InputGain")pr.input=v;
   else if(id=="OutputGain")pr.output=v;
  }
  applySculpPreset(pr);
  return true;
 }
 return p.loadPreset(file);
}
void Phobos67Editor::promptSaveSculpPreset(){
 auto* aw=new juce::AlertWindow("Save Sculp Preset","Enter a name for your Sculp preset:",juce::MessageBoxIconType::NoIcon,this);
 aw->setLookAndFeel(&look);
 aw->addTextEditor("presetName","My Sculp Preset","Preset Name:");
 aw->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));
 aw->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
 aw->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<Phobos67Editor>(this),aw](int result){
  if(result==1&&safe!=nullptr){
   auto name=juce::File::createLegalFileName(aw->getTextEditorContents("presetName").trim());
   if(name.isNotEmpty()){
    auto file=userSculpPresetsDir().getChildFile(name+".xml");
    safe->saveUserSculpPresetFile(file);
   }
  }
  delete aw;
 }),false);
}
void Phobos67Editor::openSculpPresetMenu(){
 static const SculpPresetData kFactoryPresets[]{
  {"Default (Reset Sculp)",    0.f,  0.f,  0.f, 2.f, 0.f, 0.f,   0.f,   0.f,  0.f,   0.f},
  {"TMC_Mixbus",               0.f,  6.f,  2.f, 0.f, 2.f, 0.f,   0.f,   2.f,  0.f,   0.f},
  {"TMC_Mixbus (Clarity)",     0.f, 12.f,  1.f, 0.f, 1.f, 0.f,   0.f,   2.f,  0.f,   0.f},
  {"TMC_Mixbus (Density)",     9.f,  0.f,  2.f, 0.f, 0.f, 1.f,   0.f,   2.f,  0.f,   0.f},
  {"TMC_Mixbus (MachineHead)", 0.f, 10.f,  0.f, 3.f, 4.f, 1.f,   0.f,   0.f,  0.f,  -2.f},
  {"TMC_Mixbus (MixHead)",     0.f, 10.f,  0.f, 3.f, 3.f, 3.f,   0.f,   0.f,  0.f,  -2.f},
  {"TMC_Trans_sienten",      100.f,  0.f, 10.f, 0.f, 0.f, 1.f, -20.f, -20.f, 20.f, -10.f}
 };
 const auto matches=[this](const SculpPresetData& pr){
  const auto v=[this](const char* id){return p.state.getRawParameterValue(id)->load();};
  return std::abs(v("even")-pr.even)<.5f&&std::abs(v("odd")-pr.odd)<.5f&&std::abs(v("sub")-pr.sub)<.5f&&std::abs(v("subFreq")-pr.subFreq)<.5f&&std::abs(v("air")-pr.air)<.5f&&std::abs(v("airFreq")-pr.airFreq)<.5f&&std::abs(v("mid")-pr.mid)<.5f&&std::abs(v("side")-pr.side)<.5f;
 };
 juce::PopupMenu m;
 m.setLookAndFeel(&look);
 m.addSectionHeader("SCULP PRESETS (TMC FOBOS)");
 for(int i=0;i<7;++i)
  m.addItem(100+i,kFactoryPresets[i].name,true,matches(kFactoryPresets[i]));

 auto dir=userSculpPresetsDir();
 auto userFiles=dir.findChildFiles(juce::File::findFiles,false,"*.xml;*.outputpreset");
 userFiles.sort();

 m.addSectionHeader("MY SCULP PRESETS");
 if(userFiles.isEmpty())
  m.addItem(999,"(No user presets saved yet)",false,false);
 else
  for(int i=0;i<userFiles.size();++i)
   m.addItem(200+i,userFiles[i].getFileNameWithoutExtension());

 m.addSectionHeader("MANAGE PRESETS");
 m.addItem(1,"Save New Sculp Preset...");
 if(!userFiles.isEmpty()){
  juce::PopupMenu overwriteMenu,deleteMenu;
  for(int i=0;i<userFiles.size();++i){
   overwriteMenu.addItem(300+i,"Overwrite '"+userFiles[i].getFileNameWithoutExtension()+"'");
   deleteMenu.addItem(400+i,"Delete '"+userFiles[i].getFileNameWithoutExtension()+"'");
  }
  m.addSubMenu("Overwrite Preset",overwriteMenu);
  m.addSubMenu("Delete Preset",deleteMenu);
 }
 m.addItem(2,"Open Presets Folder...");

 m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&panel),[safe=juce::Component::SafePointer<Phobos67Editor>(this),userFiles](int res){
  if(safe==nullptr||res<=0)return;
  if(res==1){safe->promptSaveSculpPreset();return;}
  if(res==2){userSculpPresetsDir().revealToUser();return;}
  if(res>=100&&res<107){safe->applySculpPreset(kFactoryPresets[res-100]);return;}
  if(res>=200&&res<200+userFiles.size()){safe->loadUserSculpPresetFile(userFiles[res-200]);return;}
  if(res>=300&&res<300+userFiles.size()){safe->saveUserSculpPresetFile(userFiles[res-300]);return;}
  if(res>=400&&res<400+userFiles.size()){userFiles[res-400].deleteFile();return;}
 });
}
void Phobos67Editor::openSkinPopupMenu(){
 juce::PopupMenu m;
 m.setLookAndFeel(&look);
 m.addSectionHeader("OUTPUT SKINS");
 m.addItem(1,"BLACK",true,skin.getSelectedId()==1);
 m.addItem(2,"DIAMOND II",true,skin.getSelectedId()==2);
 m.addItem(3,"Ads",true,skin.getSelectedId()==3);
 m.addSectionHeader("FL SAMPLER / DAW SKINS");
 for(int id=4;id<=kTotalSkins;++id)
  m.addItem(id,skin.getItemText(id-1),true,skin.getSelectedId()==id);
 m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&panel),[safe=juce::Component::SafePointer<Phobos67Editor>(this)](int res){
  if(safe!=nullptr&&res>=1&&res<=kTotalSkins)safe->skin.setSelectedId(res,juce::sendNotificationSync);
 });
}
void Phobos67Editor::clearMeterHolds(){for(auto& peak:meterPeaks)peak={};barPeaks.fill(0);overload.fill(false);repaint();}
void Phobos67Editor::mouseDown(const juce::MouseEvent& event){
 auto point=event.getEventRelativeTo(&panel).position;
 if(juce::Rectangle<float>(20,12,195,48).contains(point)&&(event.mods.isPopupMenu()||event.mods.isRightButtonDown())){openSculpPresetMenu();return;}
 if(juce::Rectangle<float>(220,16,120,44).contains(point)&&(event.mods.isPopupMenu()||event.mods.isRightButtonDown())){openSkinPopupMenu();return;}
 if(juce::Rectangle<float>(28,168,102,22).contains(point)||juce::Rectangle<float>(772,168,100,22).contains(point))clearMeterHolds();
}

void Phobos67Editor::applySkin(){
 const int sid=juce::jlimit(1,kTotalSkins,skin.getSelectedId());
 look.setSkinId(sid);
 activeSkinId=sid;
 const auto& sp=pal(sid);
 const auto labelText=juce::Colour(sid==2?0xff111519:sid==3?0xffeeeeee:sid>=4?sp.txtBright:0xff777777);
 for(auto& label:labels)label.setColour(juce::Label::textColourId,labelText);
 for(auto* combo:{&scale,&skin,&resampling,&oversampling,&order}){
  combo->setColour(juce::ComboBox::backgroundColourId,juce::Colour(sid>=4?sp.recBg:0xff181818));
  combo->setColour(juce::ComboBox::outlineColourId,juce::Colour(sid>=4?sp.pnlBorder:0xff353535));
  combo->setColour(juce::ComboBox::textColourId,juce::Colour(sid>=4?sp.txtBright:0xffdddddd));
 }
 panel.sendLookAndFeelChange();updateStates();repaint();
}



void Phobos67Editor::drawModuleMeters(juce::Graphics& g){
 const juce::Colour green(0xff36d477),yellow(0xffffda36),orange(0xffff982a),red(0xffff3838),background(0xff1b1b1b);
 g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(12.f)));g.drawText("DENSITY %",juce::Rectangle<float>(150,642,130,17),juce::Justification::centred);
 for(int ch=0;ch<2;ch++){float y=662+ch*10.f;bool present=ch==0||p.getTotalNumInputChannels()>1;float level=present?harmonicDisplay[ch]/100.f:0;g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(11.f)));g.drawText(ch?"R":"L",juce::Rectangle<float>(148,y-3,14,12),juce::Justification::centred);g.setColour(background);g.fillRect(164.f,y,90.f,7.f);const juce::Colour colours[]{green,yellow,orange,red};for(int zone=0;zone<4;zone++){float start=zone*.25f,end=juce::jmin(level,start+.25f);if(end>start){g.setColour(colours[zone]);g.fillRect(164+90*start,y,90*(end-start),7.f);}}g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(10.f)));g.drawText(present?juce::String(juce::roundToInt(harmonicDisplay[ch])):juce::String("--"),juce::Rectangle<float>(258,y-3,27,12),juce::Justification::centred);}
 g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(12.f)));g.drawText("CORR / SIDE %",juce::Rectangle<float>(466,642,130,17),juce::Justification::centred);
 for(int row=0;row<2;row++){const float y=662+row*10.f;g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(11.f)));g.drawText(row?"S":"C",juce::Rectangle<float>(464,y-3,14,12),juce::Justification::centred);g.setColour(background);g.fillRect(480.f,y,94.f,7.f);
  if(row==0){if(correlationValid){const float position=527+47*outputCorrelation;g.setColour(outputCorrelation<0?red:outputCorrelation<.5f?yellow:green);g.fillRect(juce::jmin(527.f,position),y,std::abs(position-527),7.f);}g.setColour(juce::Colour(0xff999999));g.drawVerticalLine(527,(int)y,(int)y+7);}
  else if(stereoValid){g.setColour(outputSidePercent>75?red:outputSidePercent>50?orange:green);g.fillRect(480.f,y,.94f*outputSidePercent,7.f);}
  const auto value=row?(stereoValid?juce::String(juce::roundToInt(outputSidePercent)):juce::String("--")):(correlationValid?(outputCorrelation>0?juce::String("+"):juce::String())+juce::String(outputCorrelation,1):juce::String("--"));g.setColour(dim());g.setFont(juce::Font(juce::FontOptions(10.f)));g.drawText(value,juce::Rectangle<float>(576,y-3,26,12),juce::Justification::centred);
 }
}
