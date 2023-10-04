#include "Header.h"
#include "MainComponent.h"
#include "CommandList.h"
#include "..\SGUL\Source\SGUL.h"

//==============================================================================
Header::Header(/*MainComponent* mainComp*/) : Panel("Header") //, main(mainComp)
  //  :juce::AudioDeviceManager()
{  
    addAndMakeVisible(recordButton);
    //sgul// recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    //sgul// recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    recordButton.setCommand(RECORD, true); // sgul

    addAndMakeVisible(openButton);
    //openButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x5c5c5c5c));
    //openButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    openButton.setCommand(OPEN_FILE);
    
    addAndMakeVisible(menu);
    //menu.setTextWhenNothingSelected("Display Mode");    
    menu.addItem("Trace Analyser", 1);
    menu.addItem("Oscilloscope", 2);
    menu.setSelectedId(2);
    menu.setCommand(DISPLAY_MODE); // sgul

    addAndMakeVisible(oscWinSizeSlider);
    //oscWinSizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    //oscWinSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    oscWinSizeSlider.setRange(0.05, 1.00, 0.05);
    oscWinSizeSlider.setCommand(WIN_SIZE);

    addAndMakeVisible(save_button);
    save_button.setCommand(SAVE_SETTINGS);

    addAndMakeVisible(load_button);
    load_button.setCommand(LOAD_SETTINGS);


    setWidth(300, 600, 10000);
    setHeight(50, 50, 50);
}


//==============================================================================
void Header::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}
//-------------------------------------------------------------------------------------
void Header::resized()
{
    auto area = getLocalBounds();    
    recordButton.setBounds(area.removeFromTop(40).removeFromLeft(100).reduced(10));
    openButton.setBounds(recordButton.getX() + recordButton.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    menu.setBounds(openButton.getX() + openButton.getWidth() + 50, recordButton.getY(), 2*recordButton.getWidth(), recordButton.getHeight());
    oscWinSizeSlider.setBounds(menu.getX() + menu.getWidth() + 10, recordButton.getY(),4 * recordButton.getWidth(), recordButton.getHeight());
    load_button.setBounds(oscWinSizeSlider.getX() + oscWinSizeSlider.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    save_button.setBounds(load_button.getX() + load_button.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
 }
