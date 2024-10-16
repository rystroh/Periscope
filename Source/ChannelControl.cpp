#include "ChannelControl.h"
#include "MainComponent.h"
#include "..\GRAPE\Source\GRAPE.h"
#include "TriggerDialog.h"
ChannelControl::ChannelControl(const juce::String& name):Panel(name) 
{
    addAndMakeVisible(oscWinSizeSlider);
    //oscWinSizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    //oscWinSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    oscWinSizeSlider.setRange(0.05, 5.00, 0.05);
#if AUDIO_SOURCE == 1 //debug mode
    oscWinSizeSlider.setValue(0.10f);
#else
    oscWinSizeSlider.setValue(2.5f);
#endif
    //oscWinSizeSlider.setTextBoxStyle(oscWinSizeSlider.getTextBoxPosition(), false, newWidth, oscWinSizeSlider.getTextBoxHeight());

    chkBoxXLink.setColour(juce::Label::textColourId, gridColour);
    //chkBoxXLink.setButtonText("x link");

    chkBoxYLink.setColour(juce::Label::textColourId, gridColour);
    //chkBoxYLink.setButtonText("y link");

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
    //sliderOffset.setSliderStyle(grape::Slider::HORIZONTAL2);
    //sliderOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    //sliderLabel.setColour(juce::Label::textColourId, gridColour);
    //sliderLabel.setText("Offset", juce::dontSendNotification);
    addAndMakeVisible(sliderOffset);
    //addAndMakeVisible(sliderLabel);

    sliderOffset.onValueChange = [this] {
        //currentOffset = sliderOffset.getValue();
        repaint();
    };
    sliderXOffset.setRange(0, 200, 1);
    sliderXOffset.setValue(0);
    //sliderXOffset.setSliderStyle(grape::Slider::HORIZONTAL2);
    //sliderXOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    //sliderXLabel.setColour(juce::Label::textColourId, gridColour);
    //sliderXLabel.setText("Width", juce::dontSendNotification);
    addAndMakeVisible(sliderXOffset);
    //addAndMakeVisible(sliderXLabel);

    sliderXOffset.onValueChange = [this] {
        //spareWidth = sliderXOffset.getValue();
        repaint();
    };

    chkBoxXLink.onClick = [this]()
    {
        auto idx = chkBoxXLink.getToggleState();
        this->pointeur->setXZoomFlag(idx);
    };
    chkBoxYLink.onClick = [this]()
    {
        auto idx = chkBoxYLink.getToggleState();
        this->pointeur->setYZoomFlag(idx);
    };
    cmbBoxGroupe.onChange = [this]()
    {
        auto idx = cmbBoxGroupe.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxGroupe.getItemText(idx);
        auto thatID = cmbBoxGroupe.getItemId(idx);
    };

    addAndMakeVisible(cmbBoxYMode);
    addAndMakeVisible(cmbBoxGroupe);
    addAndMakeVisible(chkBoxXLink);
    addAndMakeVisible(chkBoxYLink);

    cmbBoxGroupe.onChange = [this]()
    {
        auto idx = cmbBoxGroupe.getSelectedItemIndex();
        auto thatTrackGroup = cmbBoxGroupe.getItemText(idx);
        auto thatID = cmbBoxGroupe.getItemId(idx);
        auto chanID = this->getName().getTrailingIntValue();//just to check 
        this->pointeur->setZoomGroup(thatID);
    };

    addCommand(Y_SCALE, "Y scale", "Linear or dB scale");

    setWidth(200, 200, 200);//min prefered max
    setHeight(50, 100, 1000);
};
ChannelControl::~ChannelControl() {};
void ChannelControl::resized()
{
auto area = getLocalBounds();
double width;
int xoffset = 30;
int yoffset = 10;
    //oscWinSizeSlider.setBounds(area.removeFromTop(40).removeFromLeft(80).reduced(10));
    //oscWinSizeSlider.setBounds(area);
    //oscWinSizeSlider.setBounds(area.removeFromTop(40).removeFromLeft(40).reduced(xoffset));

GroupeLabel.setBounds(xoffset, yoffset, 50, 20);
//cmbBoxGroupe.setBounds(80, 20, 64, 24);
cmbBoxGroupe.setBounds(xoffset + GroupeLabel.getWidth(), yoffset, 87, 24);
chkBoxXLink.setBounds(xoffset, yoffset+30, 64, 24);
chkBoxYLink.setBounds(xoffset + 75, yoffset+30, 64, 24);

cmbBoxYMode.setBounds(xoffset + cmbBoxYMode.getLabel()->getWidth(), yoffset+60, 87, 24);

//sliderLabel.setBounds(xoffset, yoffset+120, 50, 20);
sliderOffset.setBounds(xoffset+30, yoffset + 90, 100, 20);// + sliderLabel.getWidth(), yoffset+120, 100, 20);
sliderOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, sliderOffset.getTextBoxHeight());

//sliderXLabel.setBounds(xoffset, yoffset+150, 50, 20);
sliderXOffset.setBounds(xoffset+30, yoffset + 120, 100, 20); //+ sliderXLabel.getWidth(), yoffset+150, 100, 20);
sliderXOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, sliderXOffset.getTextBoxHeight());
}

bool ChannelControl::executeCommand(int id, grape::Control* source)
{
    switch (id)
    {
    case Y_SCALE:
    {
        auto mode = (cmbBoxYMode.getToggleState() ? verticalScale::dB : verticalScale::Linear);
        this->pointeur->setYScale(mode);
        this->pointeur->repaint();
        return true;
    }
    default:
        return false;
    }

};