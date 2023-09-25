#include "Header.h"
#include "MainComponent.h"
#include "..\SGUL\Source\SGUL.h"

//==============================================================================
Header::Header(MainComponent* mainComp) : Panel("Header"), main(mainComp)
  //  :juce::AudioDeviceManager()
{  
    addAndMakeVisible(recordButton);
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    addAndMakeVisible(openButton);
    openButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x5c5c5c5c));
    openButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    recordButton.onClick = [this]
    {
        if (main->eScope[0]->isRecording())
            main->stopRecording();
        else
            main->startRecording();
    };
    
    addAndMakeVisible(menu);
    menu.setTextWhenNothingSelected("Display Mode");
    
    menu.addItem("Trace Analyser", 1);
    menu.addItem("Oscilloscope", 2);
    recmode = 2;
    menu.setSelectedId(recmode);
    menu.onChange = [this]()
    {
        auto oscmode = menu.getItemId(menu.getSelectedItemIndex());
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            main->eScope[idx]->setDisplayThumbnailMode(oscmode);
        }
    };

    addAndMakeVisible(oscWinSizeSlider);
    //oscWinSizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    //oscWinSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    oscWinSizeSlider.setRange(0.05, 1.00, 0.05);
    oscWinSizeSlider.onValueChange = [this]()
    {
        oscilloWinSize = oscWinSizeSlider.getValue();
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            main->eScope[idx]->setViewSize(oscilloWinSize);
        }
    };

    save_button.reset(new sgul::PushButton("Save", false, sgul::PushButton::NAME));
    addAndMakeVisible(save_button.get());
    save_button->onClick = [this] {((MainComponent*)getParentComponent())->pc->save(juce::File("c:\\Dream\\Projets\\SAM6000 Tools\\eScope\\test.xml")); };


    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        main->eScope[idx]->recThumbnail.addChangeListener(&listenerComponent);
        addAndMakeVisible(main->eScope[idx].get());
        main->eScope[idx]->setChannelID(idx);
    }

    addAndMakeVisible(listenerComponent);

    openButton.onClick = [this] { openButtonClicked(); };

    setWidth(300, 600, 10000);
    setHeight(50, 50, 50);
}

//-------------------------------------------------------------------------------------
void Header::openButtonClicked()
{/*
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file...",
                                                   juce::File{},"*.wav");*/
    chooser = std::make_unique<juce::FileChooser>("Select a Wave List...",
        juce::File{}, "*.txt");
    auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            if (file != juce::File{})
                main->configureStreaming(file);
         });
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
    listenerComponent.setBounds(openButton.getX() + openButton.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    menu.setBounds(listenerComponent.getX() + listenerComponent.getWidth() + 50, recordButton.getY(), 2*recordButton.getWidth(), recordButton.getHeight());
    oscWinSizeSlider.setBounds(menu.getX() + menu.getWidth() + 10, recordButton.getY(),4 * recordButton.getWidth(), recordButton.getHeight());
    save_button->setBounds(oscWinSizeSlider.getX() + oscWinSizeSlider.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
 }
