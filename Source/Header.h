#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
#include "eScope.h"
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
    sgul::ToggleButton recordButton{ "Record", false, sgul::ToggleButton::HORIZONTAL, "Record", "Stop", " " };
    sgul::PushButton openButton{ "Open file", false, sgul::PushButton::NAME};
    sgul::ComboBox menu{ "Mode", true, sgul::ComboBox::HORIZONTAL};
    sgul::Slider oscWinSizeSlider{ "Win size", true, sgul::Slider::HORIZONTAL2, " "};

    sgul::PushButton load_button{ "Load", false, sgul::PushButton::NAME };
    sgul::PushButton save_button{ "Save", false, sgul::PushButton::NAME };
 //-------------------------------------------------------------------------------------
    //void openButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};
