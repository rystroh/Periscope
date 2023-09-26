#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
#include "eScope.h"
#include "ListenerComponent.h"
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
    Header(MainComponent* mainComp);
    ~Header() override {};

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    int getRecMode() { return recmode; };
    int getOscilloWinSize() { return oscilloWinSize; };
private:
    friend MainComponent;
    //==============================================================================
    // Your private member variables go here...
    MainComponent* main;
    juce::TextButton recordButton{ "Record" };
    juce::TextButton openButton{ "Open File" };
    sgul::ComboBox menu{ "Mode", true, sgul::ComboBox::HORIZONTAL};
    sgul::Slider oscWinSizeSlider{ "Win size", true, sgul::Slider::HORIZONTAL2, " "};

    std::unique_ptr<sgul::PushButton> save_button;

    std::unique_ptr<juce::FileChooser> chooser;

    ListenerComponent listenerComponent;
    int recmode; // can be 1= track view or 2= oscilloscope
    double oscilloWinSize = 0.05;
 //-------------------------------------------------------------------------------------
    void openButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Header)
};
