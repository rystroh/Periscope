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
struct triggerDlgData
{
    bool enable;
    int channel;
    float threshold;
    int direction;
    int pretrigger;
};

//==============================================================================
class MyDialogBoxComponent : public juce::Component
{
public:
    MyDialogBoxComponent(triggerDlgData* bufferDlgData)
    {
        // Add and configure the label
        inittrigDlgDataAlpha(bufferDlgData);
        int diaWidth = 500;
        setSize(diaWidth, 200);
        addAndMakeVisible(triggerEnableLabel);
        triggerEnableLabel.setText("Trigger enable:", juce::dontSendNotification);
        triggerEnableLabel.setJustificationType(juce::Justification::centredLeft);
        triggerEnableChkBx.setToggleState(trigDlgData.enable, false); //triggerEnableChkBx.setToggleState(true, false);

        triggerEnableChkBx.onClick = [this]() { checkBoxClick(); };
        triggerEnableChkBx.onStateChange = [this]() { checkBoxChange(); };

        addAndMakeVisible(triggerChannelLabel);
        triggerChannelLabel.setText("Trigger channel:", juce::dontSendNotification);
        addAndMakeVisible(triggerChannelCmbBx);
        std::string ChanText;
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            ChanText = "Channel " + std::to_string(idx+1);
            triggerChannelCmbBx.addItem(ChanText, idx+1);
        }
 
        triggerChannelCmbBx.setSelectedItemIndex(trigDlgData.channel-1, true); //triggerChannelCmbBx.setSelectedItemIndex(0, true);
        triggerChannelCmbBx.onChange = [this]() { comboBoxChange(); };

        addAndMakeVisible(ThresholdLabel);
        ThresholdLabel.setText("Threshold:", juce::dontSendNotification);
        addAndMakeVisible(ThresholdSlider);
        ThresholdSlider.setRange(-1.0f, 1.0f, 0.01f);
        ThresholdSlider.setValue(trigDlgData.threshold);//ThresholdSlider.setValue(0.5f);
        ThresholdSlider.setNumDecimalPlacesToDisplay(2);
        ThresholdSlider.setSliderSnapsToMousePosition(false); 

        addAndMakeVisible(DirectionLabel);
        DirectionLabel.setText("Direction:", juce::dontSendNotification);
        addAndMakeVisible(triggerUpChkBx);
        addAndMakeVisible(trigUpLabel);
        trigUpLabel.setText("Up", juce::dontSendNotification);
        
        addAndMakeVisible(triggerDownChkBx);
        addAndMakeVisible(trigDownLabel);
        trigDownLabel.setText("Down", juce::dontSendNotification);
        
        addAndMakeVisible(triggerUpDownChkBx);
        addAndMakeVisible(trigUpDownLabel);
        trigUpDownLabel.setText("Up & Down", juce::dontSendNotification);

        addAndMakeVisible(triggerClippingChkBx);
        addAndMakeVisible(trigClippingLabel);
        trigClippingLabel.setText("Clipping", juce::dontSendNotification);

        triggerUpChkBx.setRadioGroupId(1);
        triggerDownChkBx.setRadioGroupId(1);
        triggerUpDownChkBx.setRadioGroupId(1);
        
        switch (trigDlgData.direction) {
        case Clipping:
            triggerClippingChkBx.setToggleState(true, false);
            triggerMode = Clipping;
            break;
        case ThresholdRising:
            triggerUpChkBx.setToggleState(true, false);
            triggerMode = ThresholdRising;
            break;
        case ThresholdFalling:
            triggerMode = ThresholdFalling;
            triggerDownChkBx.setToggleState(true, false);
            break;
        case ThresholdRisingOrFalling:
            triggerMode = ThresholdRisingOrFalling;
            triggerUpDownChkBx.setToggleState(true, false);
            break;
        }
        triggerClippingChkBx.onClick = [this] {checkboxClicked(triggerClippingChkBx); };
        triggerUpChkBx.onClick = [this] {checkboxClicked(triggerUpChkBx); };
        triggerDownChkBx.onClick = [this] {checkboxClicked(triggerDownChkBx); };
        triggerUpDownChkBx.onClick = [this] {checkboxClicked(triggerUpDownChkBx); };

        addAndMakeVisible(preTriggerLabel);
        preTriggerLabel.setText("Pre-trigger:", juce::dontSendNotification);
        addAndMakeVisible(preTriggerSlider);
        preTriggerSlider.setRange(0, 100, 1);
        preTriggerSlider.setValue(trigDlgData.pretrigger);  //preTriggerSlider.setValue(50);

        addAndMakeVisible(okButton);
        okButton.setButtonText("OK");
        okButton.onClick = [this]()   { OKbuttonClicked(); };

        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [this]() { CancelbuttonClicked(); };

        addAndMakeVisible(triggerEnableChkBx);
        triggerEnableChkBx.onClick = [this]() { bool isON = triggerEnableChkBx.getToggleState(); };       
    }

    void resized() override
    {
        int chkBox = 25;
        int buttonWidth = 80;
        int thirdWidth;
        int leftX, rightX;
        int largerWidth = 120;
        auto area = getLocalBounds().reduced(10);
        thirdWidth = (area.getWidth() - 2 * buttonWidth) / 3;
        auto subarea = getLocalBounds();
        auto subarea2 = getLocalBounds();
        //DBG("LocalBounds: x = " << subarea.getX() << " y = " << subarea.getY() << " w = " << subarea.getWidth() << " h = " << subarea.getHeight());
        //DBG("area: x = " << area.getX() << " y = " << area.getY() << " w = " << area.getWidth() << " h = " << area.getHeight());
        subarea = area.removeFromTop(30);
        subarea2 = area.removeFromTop(30);

        triggerChannelLabel.setBounds(subarea2.getX(), subarea2.getY(), 120, 20);
        triggerChannelLabel.setJustificationType(juce::Justification::centredRight);
        triggerChannelCmbBx.setBounds(triggerChannelLabel.getRight(), triggerChannelLabel.getY(), largerWidth, 20);
        //DBG("subarea2: x = " << subarea2.getX() << " y = " << subarea2.getY() << " w = " << subarea2.getWidth() << " h = " << subarea2.getHeight());
        largerWidth = triggerChannelLabel.getWidth();

        rightX = triggerChannelLabel.getRight();

        triggerEnableLabel.setBounds(subarea.getX(), subarea.getY(), largerWidth, 20); // 100, 20);
        triggerEnableLabel.setJustificationType(juce::Justification::centredRight);
        triggerEnableChkBx.setBounds(triggerEnableLabel.getRight(), subarea.getY(), chkBox, 20);
        //DBG("subarea: x = " << subarea.getX() << " y = " << subarea.getY() << " w = " << subarea.getWidth() << " h = " << subarea.getHeight());

        subarea = area.removeFromTop(30);
        ThresholdLabel.setBounds(subarea.getX(), subarea.getY(), largerWidth, 20);
        ThresholdLabel.setJustificationType(juce::Justification::centredRight);
        //DBG("subarea: x = " << subarea.getX() << " y = " << subarea.getY() << " w = " << subarea.getWidth() << " h = " << subarea.getHeight());
        ThresholdSlider.setBounds(triggerEnableChkBx.getX(), ThresholdLabel.getY(), 200, 20);

        subarea = area.removeFromTop(30);
        DirectionLabel.setBounds(subarea.getX(), subarea.getY(), largerWidth, 20);
        DirectionLabel.setJustificationType(juce::Justification::centredRight);
        //DBG("subarea: x = " << subarea.getX() << " y = " << subarea.getY() << " w = " << subarea.getWidth() << " h = " << subarea.getHeight());
        triggerUpChkBx.setBounds(triggerEnableChkBx.getX(), DirectionLabel.getY(), chkBox, 20);
        trigUpLabel.setBounds(triggerUpChkBx.getX() + triggerUpChkBx.getWidth(), DirectionLabel.getY(), 50, 20);
        triggerDownChkBx.setBounds(trigUpLabel.getX() + trigUpLabel.getWidth(), DirectionLabel.getY(), chkBox, 20);
        trigDownLabel.setBounds(triggerDownChkBx.getX() + triggerDownChkBx.getWidth() , DirectionLabel.getY(), 50, 20);
        triggerUpDownChkBx.setBounds(trigDownLabel.getX() + trigDownLabel.getWidth(), DirectionLabel.getY(), chkBox, 20);
        trigUpDownLabel.setBounds(triggerUpDownChkBx.getX() + triggerUpDownChkBx.getWidth(), DirectionLabel.getY(), 80, 20);
        triggerClippingChkBx.setBounds(trigUpDownLabel.getX() + trigUpDownLabel.getWidth(), DirectionLabel.getY(), chkBox, 20);
        trigClippingLabel.setBounds(triggerClippingChkBx.getX() + triggerClippingChkBx.getWidth(), DirectionLabel.getY(), 80, 20);
        
        subarea = area.removeFromTop(30);
        preTriggerLabel.setBounds(subarea.getX(), subarea.getY(), largerWidth, 20);
        preTriggerLabel.setJustificationType(juce::Justification::centredRight);
        //DBG("subarea: x = " << subarea.getX() << " y = " << subarea.getY() << " w = " << subarea.getWidth() << " h = " << subarea.getHeight());
        preTriggerSlider.setBounds(triggerEnableChkBx.getX(), preTriggerLabel.getY(), 200, 20);

        subarea = area.removeFromTop(200);
        okButton.setBounds(thirdWidth, preTriggerSlider.getY() + 40, buttonWidth, 20);        
        cancelButton.setBounds( 2 * thirdWidth + buttonWidth, preTriggerSlider.getY() + 40, buttonWidth, 20);
    }
    void checkBoxClick()
    {
        DBG("checkBoxClick");

    }
    void checkboxClicked(juce::ToggleButton& button) 
    {
        if (&button == &triggerClippingChkBx)
        {
            triggerMode = Clipping;
            //DBG("Option 1");
        }
        if (&button == &triggerUpChkBx)
        {
            triggerMode = ThresholdRising;
            //DBG("Option 1");
        }
        else if (&button == &triggerDownChkBx)
        {
            triggerMode = ThresholdFalling;
            //DBG("Option 2");
        }
        else if (&button == &triggerUpDownChkBx)
        {
            triggerMode = ThresholdRisingOrFalling;
            //DBG("Option 3");
        }
     }

    juce::DialogWindow* dialogWindow;

    int triggerMode;
    
    triggerDlgData trigDlgData;
    triggerDlgData *trigDlgDataPtr;
    
    void inittrigDlgData2()
    {
        trigDlgData.enable = true;
        trigDlgData.channel = 1;
        trigDlgData.threshold = 0.5;
        trigDlgData.direction = 1;
        trigDlgData.pretrigger = 50;
    }
    void inittrigDlgData()
    {
        trigDlgData.enable = false;
        trigDlgData.channel = 3;
        trigDlgData.threshold = 0.25;
        trigDlgData.direction = 2;
        trigDlgData.pretrigger = 20;
    }
    /*
    void initTrigDlgData(bool ena, int chan, float thresh, int direct, int pretrig)
    {
        bufferDlgData.enable = ena;
        bufferDlgData.channel = chan;
        triggerMode = direct;
        bufferDlgData.threshold = thresh;
        bufferDlgData.direction = direct;
        bufferDlgData.pretrigger = pretrig;
    }*/
    void inittrigDlgDataAlpha(triggerDlgData* bufferDlgData)
    {
        trigDlgDataPtr = bufferDlgData;
        trigDlgData.enable = bufferDlgData->enable;
        trigDlgData.channel = bufferDlgData->channel;
        triggerMode = bufferDlgData->direction;
        trigDlgData.threshold = bufferDlgData->threshold;
        trigDlgData.direction = bufferDlgData->direction;
        trigDlgData.pretrigger = bufferDlgData->pretrigger;
    }
    void checkBoxChange()
    {
        DBG("checkBoxChange");
    }

    void comboBoxChange()
    {
        DBG("comboBoxChange");
    }

    void OKbuttonClicked()
    {
        trigDlgData.enable = triggerEnableChkBx.getToggleState();
        trigDlgData.channel = triggerChannelCmbBx.getItemId(triggerChannelCmbBx.getSelectedItemIndex());
        trigDlgData.threshold = ThresholdSlider.getValue();
        trigDlgData.direction = triggerMode;
        trigDlgData.pretrigger = preTriggerSlider.getValue();

        trigDlgDataPtr->enable = triggerEnableChkBx.getToggleState();
        trigDlgDataPtr->channel = triggerChannelCmbBx.getItemId(triggerChannelCmbBx.getSelectedItemIndex());
        trigDlgDataPtr->threshold = ThresholdSlider.getValue();        
        trigDlgDataPtr->direction = triggerMode;
        trigDlgDataPtr->pretrigger = preTriggerSlider.getValue();
        
        auto parwin = this->getParentComponent();
        parwin->setVisible(false);
        parwin->exitModalState(0);
    }

    void CancelbuttonClicked()
    {
        auto parwin = this->getParentComponent();
        parwin->setVisible(false);
        parwin->exitModalState(0);
    }
    void sliderValueChanged()
    {
        DBG("sliderValueChanged");
    }
    private:
        //------------------------------------------------------------------------------
        juce::Label triggerEnableLabel;
        juce::ToggleButton triggerEnableChkBx;
        juce::Label triggerChannelLabel;
        juce::ComboBox triggerChannelCmbBx;
        juce::Label ThresholdLabel;
        juce::Slider ThresholdSlider;
        juce::Label DirectionLabel;
        juce::ToggleButton triggerUpChkBx;
        juce::Label trigUpLabel;
        juce::ToggleButton triggerDownChkBx;
        juce::Label trigDownLabel;
        juce::ToggleButton triggerUpDownChkBx;
        juce::Label trigUpDownLabel;
        juce::ToggleButton triggerClippingChkBx;
        juce::Label trigClippingLabel;
        juce::Label preTriggerLabel;
        juce::Slider preTriggerSlider;
        juce::TextButton okButton;
        juce::TextButton cancelButton;
        /*
        juce::Label label;
        juce::TextButton button;
        juce::Slider slider;
        */
};