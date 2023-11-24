#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
#include "eScope.h"
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
    // This button issues a load settings command
    grape::PushButton LiveButton{ "Live", grape::PushButton::NAME, GO_LIVE };


    // * Controls whose values are saved in the settings *
    // This button issues a display mode command
    grape::ComboBox menu{ "Mode", grape::ComboBox::HORIZONTAL, grape::ComboBox::ValueType::ID, "", DISPLAY_MODE};
    // This button issues a window size command
    grape::Slider oscWinSizeSlider{ "Win size", grape::Slider::HORIZONTAL2, "", WIN_SIZE };
    // This button issues a window size command
    grape::Slider ThesholdSlider{ "Threshold", grape::Slider::HORIZONTAL2, "", THRESHOLD_LEVEL };

 //-------------------------------------------------------------------------------------
    //void openButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};