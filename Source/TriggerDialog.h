/*
  ==============================================================================

    TriggerDialog.h
    Created: 16 Jul 2024 11:34:50am
    Author:  René-Yves

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Enums.h"
#include "..\GRAPE\Source\GRAPE.h"
struct triggerDlgData
{
    bool enable;
    int channel;
    double threshold;
    int direction;
    int pretrigger;
};

//==============================================================================
class TriggerSettings : public grape::Panel
{
public:
    TriggerSettings() : Panel("Trigger settings")
    {
        addAndMakeVisible(triggerEnableChkBx);

        addAndMakeVisible(triggerChannelCmbBx);
        std::string ChanText;
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            triggerChannelCmbBx.addItem(juce::String(idx + 1), idx + 1);
        }
        triggerChannelCmbBx.setSelectedItemIndex(0, true); //triggerChannelCmbBx.setSelectedItemIndex(0, true);

        addAndMakeVisible(thresholdSlider);
        thresholdSlider.setRange(-1.0f, 1.0f, 0.010f);
        thresholdSlider.setValue(0.5);
        thresholdSlider.setNumDecimalPlacesToDisplay(4);
        thresholdSlider.setSliderSnapsToMousePosition(false);

        addAndMakeVisible(triggerConditionCmbBx);
        triggerConditionCmbBx.addItem("Up", ThresholdRising);
        triggerConditionCmbBx.addItem("Down", ThresholdFalling);
        triggerConditionCmbBx.addItem("Up & down", ThresholdRisingOrFalling);
        triggerConditionCmbBx.addItem("Clipping", Clipping);
        triggerConditionCmbBx.setSelectedItemIndex(0, true);

        addAndMakeVisible(preTriggerSlider);
        preTriggerSlider.setRange(0, 100, 1);
        preTriggerSlider.setValue(50);

        setWidth(300, 600, 10000);
        setHeight(40, 40, 40);

    }

    void resized() override
    {
        auto area = getLocalBounds();
        double width;
        triggerEnableChkBx.setBounds(20, 10, 80, 20);
        triggerChannelCmbBx.setBounds(triggerEnableChkBx.getX() + triggerEnableChkBx.getWidth() + triggerChannelCmbBx.getLabel()->getWidth() + 10, triggerEnableChkBx.getY(), 50, triggerEnableChkBx.getHeight());
        thresholdSlider.setBounds(triggerChannelCmbBx.getX() + triggerChannelCmbBx.getWidth() + thresholdSlider.getLabel()->getWidth() + 10, triggerEnableChkBx.getY(), 150, triggerEnableChkBx.getHeight());
        triggerConditionCmbBx.setBounds(thresholdSlider.getX() + thresholdSlider.getWidth() + triggerConditionCmbBx.getLabel()->getWidth() + 10, triggerEnableChkBx.getY(), 110, triggerEnableChkBx.getHeight());
        preTriggerSlider.setBounds(triggerConditionCmbBx.getX() + triggerConditionCmbBx.getWidth() + preTriggerSlider.getLabel()->getWidth() + 10, triggerEnableChkBx.getY(), 150, triggerEnableChkBx.getHeight());

    }
    private:
        //------------------------------------------------------------------------------
        grape::CheckBox triggerEnableChkBx{ "Enable" };
        grape::ComboBox triggerChannelCmbBx{ "Channel", grape::ComboBox::HORIZONTAL };
        grape::Slider thresholdSlider{ "Threshold", grape::Slider::HORIZONTAL2 };
        grape::ComboBox triggerConditionCmbBx{ "Condition", grape::ComboBox::HORIZONTAL };
        grape::Slider preTriggerSlider{ "Pre-trigger", grape::Slider::HORIZONTAL2 };

};