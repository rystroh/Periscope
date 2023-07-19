#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
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
        if (eScope.isRecording())
            stopRecording();
        else
            startRecording();
    };

    //addAndMakeVisible(eScope.recThumbnail); //needs refactoring
    addAndMakeVisible(eScope); //not working
    //addAndMakeVisible(eScope.recThumbnail); //needs refactoring
    openButton.onClick = [this] { openButtonClicked(); };
    
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    auto& devManager = MainComponent::getAudioDeviceManager();
    devManager.addAudioCallback(eScope.getAudioIODeviceCallBack());
    setSize(870, 600);
    formatManager.registerBasicFormats();
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    auto& devManager = MainComponent::getAudioDeviceManager();
    devManager.removeAudioCallback(eScope.getAudioIODeviceCallBack());
}
//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    auto& devManager = MainComponent::getAudioDeviceManager();
    auto device = devManager.getCurrentAudioDevice();
    //eScope.audioDeviceAboutToStart(device); //needs refactoring
    eScope.setSampleRate(device->getCurrentSampleRate());
}
//-------------------------------------------------------------------------------------
void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
}
//-------------------------------------------------------------------------------------
void MainComponent::releaseResources()
{
}
//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}
//-------------------------------------------------------------------------------------
void MainComponent::resized()
{
    auto area = getLocalBounds();
    recordButton.setBounds(area.removeFromTop(40).removeFromLeft(100).reduced(10));
    openButton.setBounds(recordButton.getX() + recordButton.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    eScope.setBounds(10, 40, getWidth() - 20, area.getHeight() / 4);
}
