#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
//#include "eScope.h"
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"
#include "CommandList.h"
#include "..\GRAPE\Source\GRAPE.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent;
class Header  : public grape::Panel                                                                  
{
public:
    //==============================================================================
    Header();
    ~Header() override {};

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    friend MainComponent;
    //==============================================================================
    // Your private member variables go here...
    // *** Controls ***
    // * Command only controls (state not saved in configuration) *
    // This button issues a synchronous record command
    grape::ToggleButton recordButton{ "Record", grape::ToggleButton::HORIZONTAL, "Record", "Stop", RECORD, true}; 
    // This button issues an open file command
    grape::PushButton openButton{ "Open file", grape::PushButton::NAME, OPEN_FILE};
    // This button issues a load settings command
    grape::PushButton load_button{ "Load", grape::PushButton::NAME, LOAD_SETTINGS};
    // This button issues a save settings command
    grape::PushButton save_button{ "Save", grape::PushButton::NAME, SAVE_SETTINGS};
    // This button issues a go live command
    grape::PushButton LiveButton{ "Live", grape::PushButton::NAME, GO_LIVE };
    // This button issues a save wav command
    grape::PushButton saveWav_button{ "Save Wav", grape::PushButton::NAME, SAVE_WAV };


    // * Controls whose values are saved in the settings *
    // This button issues a display mode command
    grape::ComboBox menu{ "Mode", grape::ComboBox::HORIZONTAL, grape::ComboBox::ValueType::ID, "", DISPLAY_MODE};
    // This button issues a window size command
    grape::Slider oscWinSizeSlider{ "Buffer Size in s", grape::Slider::HORIZONTAL2, "", WIN_SIZE };
    // This button issues a y scale command
    grape::Slider yScaleSlider{ "Y Scale", grape::Slider::HORIZONTAL2, "", Y_SCALE };
   

 //-------------------------------------------------------------------------------------
    //void openButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};
