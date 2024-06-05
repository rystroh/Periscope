#include "ChannelControl.h"
#include "MainComponent.h"
#include "..\GRAPE\Source\GRAPE.h"
ChannelControl::ChannelControl(const juce::String& name):Panel(name) 
{
    addAndMakeVisible(oscWinSizeSlider);
    //oscWinSizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    //oscWinSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    oscWinSizeSlider.setRange(0.05, 5.00, 0.05);
    oscWinSizeSlider.setValue(2.5f);
    //oscWinSizeSlider.setTextBoxStyle(oscWinSizeSlider.getTextBoxPosition(), false, newWidth, oscWinSizeSlider.getTextBoxHeight());

    chkBoxXLink.setColour(juce::Label::textColourId, gridColour);
    chkBoxXLink.setButtonText("x link");

    chkBoxYLink.setColour(juce::Label::textColourId, gridColour);
    chkBoxYLink.setButtonText("y link");

    cmbBoxXMode.setEditableText(false);
    cmbBoxXMode.setJustificationType(juce::Justification::centredLeft);
    cmbBoxXMode.setTextWhenNothingSelected(juce::String());
    cmbBoxXMode.setTextWhenNoChoicesAvailable("(no choices)");
    cmbBoxXMode.addItem("Absolute", 1);
    cmbBoxXMode.addItem("Relative to Trigger", 2);

    cmbBoxYMode.setEditableText(false);
    cmbBoxYMode.setJustificationType(juce::Justification::centredLeft);
    cmbBoxYMode.setTextWhenNothingSelected(juce::String());
    cmbBoxYMode.setTextWhenNoChoicesAvailable("(no choices)");
    cmbBoxYMode.addItem("Linear", 1);
    cmbBoxYMode.addItem("Log dB", 2);

    cmbBoxGroupe.setEditableText(false);
    cmbBoxGroupe.setJustificationType(juce::Justification::centredLeft);
    cmbBoxGroupe.setTextWhenNothingSelected(juce::String());
    cmbBoxGroupe.setTextWhenNoChoicesAvailable("(no choices)");
    cmbBoxGroupe.addItem("None", 16);
    cmbBoxGroupe.addItem("1", 1);
    cmbBoxGroupe.addItem("2", 2);
    cmbBoxGroupe.addItem("3", 3);
    cmbBoxGroupe.addItem("4", 4);

    sliderOffset.setRange(-600, 600, 1);
    sliderOffset.setValue(0);
    sliderOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    sliderLabel.setColour(juce::Label::textColourId, gridColour);
    sliderLabel.setText("Offset", juce::dontSendNotification);
    addAndMakeVisible(sliderOffset);
    addAndMakeVisible(sliderLabel);

    sliderOffset.onValueChange = [this] {
        //currentOffset = sliderOffset.getValue();
        repaint();
    };
    sliderXOffset.setRange(0, 200, 1);
    sliderXOffset.setValue(0);
    sliderXOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    sliderXLabel.setColour(juce::Label::textColourId, gridColour);
    sliderXLabel.setText("Width", juce::dontSendNotification);
    addAndMakeVisible(sliderXOffset);
    addAndMakeVisible(sliderXLabel);

    sliderXOffset.onValueChange = [this] {
        //spareWidth = sliderXOffset.getValue();
        repaint();
    };

    chkBoxXLink.onClick = [this]()
    {
        auto idx = chkBoxXLink.getToggleState();
        this->pointeur->setXZoomFlag(idx);
        //xZoomFlag = idx;
    };
    chkBoxYLink.onClick = [this]()
    {
        auto idx = chkBoxYLink.getToggleState();
        this->pointeur->setYZoomFlag(idx);
        //yZoomFlag = idx;
    };
    cmbBoxXMode.onChange = [this]()
    {
        auto idx = cmbBoxXMode.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxXMode.getItemText(idx);
        auto thatID = cmbBoxXMode.getItemId(idx);
    };
    cmbBoxYMode.onChange = [this]()
    {
        auto idx = cmbBoxYMode.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxYMode.getItemText(idx);
        auto thatID = cmbBoxYMode.getItemId(idx);        
    };
    cmbBoxGroupe.onChange = [this]()
    {
        auto idx = cmbBoxGroupe.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxGroupe.getItemText(idx);
        auto thatID = cmbBoxGroupe.getItemId(idx);
        //zoomGroup = thatID;
    };

    addAndMakeVisible(cmbBoxXMode);
    addAndMakeVisible(cmbBoxYMode);
    addAndMakeVisible(cmbBoxGroupe);
    addAndMakeVisible(chkBoxXLink);
    addAndMakeVisible(chkBoxYLink);


    cmbBoxXMode.onChange = [this]()
    {
        auto idx = cmbBoxXMode.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxXMode.getItemText(idx);
        auto thatID = cmbBoxXMode.getItemId(idx);
    };
    cmbBoxYMode.onChange = [this]()
    {
        auto idx = cmbBoxYMode.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxYMode.getItemText(idx);
        auto thatID = cmbBoxYMode.getItemId(idx);
        
    };
    cmbBoxGroupe.onChange = [this]()
    {
        auto idx = cmbBoxGroupe.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxGroupe.getItemText(idx);
        auto thatID = cmbBoxGroupe.getItemId(idx);
        auto chanID = this->getName().getTrailingIntValue();
        /*
        std::unique_ptr<RecordingThumbnail>* eThbPtr = ((std::unique_ptr<RecordingThumbnail>*)this->thmbNail);
            RecordingThumbnail* escThbPtr = ((RecordingThumbnail*)this->thmbNail);
            auto ptdr = escThbPtr->getZoomGroup();// .zoomGroup = thatID;
        //zoomGroup = thatID;
        auto escopeThumbNailPtr = ((RecordingThumbnail*)this->thmbNail);
        auto toto = this->thmbNail->~unique_ptr;*/
        this->pointeur->setZoomGroup(thatID);
    };


    setWidth(70, 200, 200);//min prefered max
    setHeight(50, 100, 1000);
};
ChannelControl::~ChannelControl() {};
void ChannelControl::resized()
{
    auto area = getLocalBounds();
    double width;
    //oscWinSizeSlider.setBounds(area.removeFromTop(40).removeFromLeft(80).reduced(10));
    //oscWinSizeSlider.setBounds(area);
    //oscWinSizeSlider.setBounds(area.removeFromTop(40).removeFromLeft(40).reduced(5));

    GroupeLabel.setBounds(5, 60, 50, 20);
    //cmbBoxGroupe.setBounds(80, 20, 64, 24);
    cmbBoxGroupe.setBounds(5 + GroupeLabel.getWidth(), 60, 87, 24);
    chkBoxXLink.setBounds(5, 90, 64, 24);
    chkBoxYLink.setBounds(80, 90, 64, 24);

    xGridModeLabel.setBounds(5, 120, 50, 20);
    yGridModeLabel.setBounds(5, 150, 50, 20);

    cmbBoxXMode.setBounds(5 + xGridModeLabel.getWidth(), 120, 87, 24);
    cmbBoxYMode.setBounds(5 + yGridModeLabel.getWidth(), 150, 87, 24);

    sliderLabel.setBounds(5, 180, 50, 20);
    sliderOffset.setBounds(5 + sliderLabel.getWidth(), 180, 100, 20);
    sliderOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, sliderOffset.getTextBoxHeight());

    sliderXLabel.setBounds(5, 210, 50, 20);
    sliderXOffset.setBounds(5 + sliderXLabel.getWidth(), 210, 100, 20);
    sliderXOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, sliderXOffset.getTextBoxHeight());
}
