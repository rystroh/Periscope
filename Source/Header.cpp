#include "Header.h"
#include "MainComponent.h"
#include "..\GRAPE\Source\GRAPE.h"

//==============================================================================
Header::Header(/*MainComponent* mainComp*/) : Panel("Header") //, main(mainComp)
  //  :juce::AudioDeviceManager()
{  
    addAndMakeVisible(recordButton);
    // recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    // recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    addAndMakeVisible(openButton);
    //openButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x5c5c5c5c));
    //openButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    
    addAndMakeVisible(menu);
    //menu.setTextWhenNothingSelected("Display Mode");    
    menu.addItem("Trace Analyser", 1);
    menu.addItem("Oscilloscope", 2);
    menu.setSelectedId(2);

    addAndMakeVisible(oscWinSizeSlider);
    //oscWinSizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    //oscWinSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    oscWinSizeSlider.setRange(0.05, 5.00, 0.05);
    oscWinSizeSlider.setValue(0.10f);

    addAndMakeVisible(load_button);
    addAndMakeVisible(save_button);
    addAndMakeVisible(saveWav_button);

    addAndMakeVisible(LiveButton);

    addAndMakeVisible(yScaleSlider);
    yScaleSlider.setRange(0, 1, 1);
    yScaleSlider.setValue(0);

    setWidth(300, 600, 10000);
    setHeight(40, 40, 40);
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
    double width;
    recordButton.setBounds(area.removeFromTop(40).removeFromLeft(80).reduced(10));
    LiveButton.setBounds(recordButton.getX() + recordButton.getWidth() + 10, recordButton.getY(), recordButton.getWidth()-10, recordButton.getHeight());

    openButton.setBounds(LiveButton.getX() + LiveButton.getWidth() + 10, recordButton.getY(), recordButton.getWidth()+20, recordButton.getHeight());
    width = 2.0 * (float)recordButton.getWidth();
    menu.setBounds(openButton.getX() + openButton.getWidth() + menu.getLabel()->getWidth() + 10, recordButton.getY(),(int)width, recordButton.getHeight());

    oscWinSizeSlider.setBounds(menu.getX() + menu.getWidth() + oscWinSizeSlider.getLabel()->getWidth() + 10, recordButton.getY(), 2 * recordButton.getWidth(), recordButton.getHeight());
    yScaleSlider.setBounds(oscWinSizeSlider.getX() + oscWinSizeSlider.getWidth() + yScaleSlider.getLabel()->getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    
    load_button.setBounds(yScaleSlider.getX() + yScaleSlider.getWidth() + 10, recordButton.getY(), recordButton.getWidth()-10, recordButton.getHeight());
    //load_button.setBounds(ThesholdSlider.getX() + ThesholdSlider.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    save_button.setBounds(load_button.getX() + load_button.getWidth() + 10, recordButton.getY(), recordButton.getWidth()-10, recordButton.getHeight());
    
    saveWav_button.setBounds(save_button.getX() + save_button.getWidth() + 10, recordButton.getY(), recordButton.getWidth() - 10, recordButton.getHeight());
}
