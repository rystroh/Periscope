#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
  //  :juce::AudioDeviceManager()
{
    // Make sure you set the size of the component after
    // you add any child components.
    
    
    addAndMakeVisible(recordButton);
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    xZoomSlider.setRange(0.0, 1);
    xZoomSlider.setValue(1.0f);
    xZoomSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    xZoomLabel.setText("X Zoom", juce::dontSendNotification);
    xZoomSlider.addListener(this);

    addAndMakeVisible(xZoomSlider);
    addAndMakeVisible(xZoomLabel);


    levelSlider.setRange(0.0, 4);
    levelSlider.setValue(1.0f);
    levelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    levelLabel.setText("Y Zoom", juce::dontSendNotification);
    levelSlider.addListener(this);

    addAndMakeVisible(levelSlider);
    addAndMakeVisible(levelLabel);
    //transport.addChangeListener(this);


    recordButton.onClick = [this]
    {
        if (recorder.isRecording())
            stopRecording();
        else
            startRecording();
    };

    addAndMakeVisible(recordingThumbnail);


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

    //audioDeviceManager.addAudioCallback(&recorder);
    auto& devManager = MainComponent::getAudioDeviceManager();
    devManager.addAudioCallback(&recorder);
    setSize(800, 600);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    //audioDeviceManager.removeAudioCallback(&recorder);
    auto& devManager = MainComponent::getAudioDeviceManager();
    devManager.removeAudioCallback(&recorder);
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    
    //recorder.audioDeviceAboutToStart(this);
    auto& devManager = MainComponent::getAudioDeviceManager();
    auto device = devManager.getCurrentAudioDevice();
    recorder.audioDeviceAboutToStart(device);

}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    auto area = getLocalBounds();

    //liveAudioScroller.setBounds(area.removeFromTop(80).reduced(8));
    //recordingThumbnail.setBounds(area.removeFromTop(80).reduced(8));
    //recordButton.setBounds(area.removeFromTop(36).removeFromLeft(140).reduced(8));
    recordButton.setBounds(area.removeFromTop(40).removeFromLeft(100).reduced(10));
    //recordingThumbnail.setBounds(area.removeFromTop(80).reduced(8));
    recordingThumbnail.setBounds(10,40,getWidth()-40,120);
    xZoomLabel.setBounds(10, 170, 90, 20);
    xZoomSlider.setBounds(100, 170, getWidth() - 130, 20);

    levelLabel.setBounds(10, 200, 90, 20);
    levelSlider.setBounds(100, 200, getWidth() - 130, 20);

}
