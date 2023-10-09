#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
#include "eScope.h"
#include "CommandList.h"
#include "..\SGUL\Source\SGUL.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent;
class Header  : public sgul::Panel                                                                  
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
    sgul::ToggleButton recordButton{ "Record", sgul::ToggleButton::HORIZONTAL, "Record", "Stop", RECORD, true}; 
    // This button issues an open file command
    sgul::PushButton openButton{ "Open file", sgul::PushButton::NAME, OPEN_FILE};
    // This button issues a load settings command
    sgul::PushButton load_button{ "Load", sgul::PushButton::NAME, LOAD_SETTINGS};
    // This button issues a save settings command
    sgul::PushButton save_button{ "Save", sgul::PushButton::NAME, SAVE_SETTINGS};

    // * Controls whose values are saved in the settings *
    // This button issues a display mode command
    sgul::ComboBox menu{ "Mode", sgul::ComboBox::HORIZONTAL, sgul::ComboBox::ValueType::ID, "", DISPLAY_MODE};
    // This button issues a window size command
    sgul::Slider oscWinSizeSlider{ "Win size", sgul::Slider::HORIZONTAL2, "", WIN_SIZE };

 //-------------------------------------------------------------------------------------
    //void openButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};
