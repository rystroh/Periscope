#pragma once
// Class ChannelControl

#include <JuceHeader.h>
#include <string>
//#include "eScope.h"
#include "CommandList.h"
#include "..\GRAPE\Source\GRAPE.h"

class ChannelControl : public grape::Panel
{
public:
	ChannelControl(const juce::String& name);
	~ChannelControl();

	void resized() override;
private:
	// This button issues a window size command
	grape::Slider oscWinSizeSlider{ "Win size", grape::Slider::HORIZONTAL2, "", WIN_SIZE };
	/*
    grape::ToggleButton chkBoxXLink{ "x link",  };
	grape::ToggleButton chkBoxYLink;
	grape::ComboBox cmbBoxXMode;
	grape::ComboBox cmbBoxYMode;
	grape::ComboBox cmbBoxGroupe;*/

    juce::Colour gridColour = juce::Colour(0xff8a8a8a);

    juce::Slider xGridModeSlider;
    juce::Label eScopeChannelLabel;
    juce::Label xGridModeLabel;
    //juce::Slider yGridModeSlider;
    juce::Label yGridModeLabel;
    juce::Label GroupeLabel;

    juce::ToggleButton chkBoxXLink;
    juce::ToggleButton chkBoxYLink;
    juce::ComboBox cmbBoxXMode;
    juce::ComboBox cmbBoxYMode;
    juce::ComboBox cmbBoxGroupe;

    juce::Slider sliderOffset;
    juce::Label sliderLabel;

    juce::Slider sliderXOffset;
    juce::Label sliderXLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelControl)
};