#pragma once
// Class ChannelControl

#include <JuceHeader.h>
#include <string>
//#include "eScope.h"
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"
#include "CommandList.h"
#include "..\GRAPE\Source\GRAPE.h"

class ChannelControl : public grape::Panel
{
public:
	ChannelControl(const juce::String& name);
	~ChannelControl();

	void resized() override;

    bool executeCommand(int id, grape::Control* source);
    //juce::AudioThumbnail* thmbNail;
    std::unique_ptr<RecordingThumbnail>* thmbNail;
    RecordingThumbnail* pointeur;
private:
	// This button issues a window size command
	grape::Slider oscWinSizeSlider{ "Win size", grape::Slider::HORIZONTAL2, "", WIN_SIZE };

    juce::Colour gridColour = juce::Colour(0xff8a8a8a);    

    juce::Slider xGridModeSlider;
    juce::Label eScopeChannelLabel;
    juce::Label GroupeLabel;

grape::Slider sliderOffset{"Offset", grape::Slider::HORIZONTAL2, }; //juce::Slider sliderOffset;
//juce::Label sliderLabel;

grape::Slider sliderXOffset{ "Width", grape::Slider::HORIZONTAL2, }; //juce::Slider sliderXOffset;
//juce::Label sliderXLabel;

grape::ComboBox cmbBoxGroupe{ "groups", };
grape::CheckBox chkBoxXLink{ "x link", };    //grape::ToggleButton chkBoxXLink{ "x link", };
grape::CheckBox chkBoxYLink{ "y link", };    //grape::ToggleButton chkBoxYLink{ "y link", };
//grape::ComboBox cmbBoxYMode{ "y mode", };
grape::ToggleButton cmbBoxYMode{ "Scale", grape::ToggleButton::HORIZONTAL, "Lin", "dB", Y_SCALE};


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelControl)
};