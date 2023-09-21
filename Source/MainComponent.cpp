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
        if (eScope[0].isRecording())
            stopRecording();
        else
            startRecording();
    };    
    
    addAndMakeVisible(menu);
    menu.setTextWhenNothingSelected("Display Mode");
    
    menu.addItem("Trace Analyser", 1);
    menu.addItem("Oscilloscope", 2);
    recmode = 2;
    menu.setSelectedId(recmode);
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx].setDisplayThumbnailMode(recmode);
    }
    menu.onChange = [this]()
    {
        auto oscmode = menu.getItemId(menu.getSelectedItemIndex());
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx].setDisplayThumbnailMode(oscmode);
        }
    };

    addAndMakeVisible(oscWinSizeSlider);
    oscWinSizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    oscWinSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    oscWinSizeSlider.setRange(0.05, 1.00, 0.05);
    oscWinSizeSlider.onValueChange = [this]()
    {
        oscilloWinSize = oscWinSizeSlider.getValue();
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx].setViewSize(oscilloWinSize);
        }
    };

    

    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx].recThumbnail.addChangeListener(this);
        addAndMakeVisible(eScope[idx]);
        eScope[idx].setChannelID(idx);
    }

    addAndMakeVisible(listenerComponent);

    openButton.onClick = [this] { openButtonClicked(); };
    juce::XmlElement xxw("DEVICESETUP");
    xxw.setAttribute("deviceType", "ASIO");
    xxw.setAttribute("audioOutputDeviceName", "ASIO Fireface USB");
    xxw.setAttribute("audioInputDeviceName", "ASIO Fireface USB");
    xxw.setAttribute("audioDeviceRate", "48000.0");
    xxw.setAttribute("audioDeviceBufferSize", "480.0");
    /*
    xxw.setAttribute("audioDeviceRate", "44100.0");
    xxw.setAttribute("audioDeviceBufferSize", "512.0");*/

    xxw.setAttribute("audioDeviceInChans", "11111111");    

    deviceManager.initialise(eScopeChanNb, 2, &xxw, true);
    /*
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
    }*/
    auto& devManager = MainComponent::getAudioDeviceManager();
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        devManager.addAudioCallback(eScope[idx].getAudioIODeviceCallBack());
    }
    setSize(1800, 1000);
    formatManager.registerBasicFormats();
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    auto& devManager = MainComponent::getAudioDeviceManager();
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        devManager.removeAudioCallback(eScope[idx].getAudioIODeviceCallBack());
    }
}
//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    auto& devManager = MainComponent::getAudioDeviceManager();
    auto device = devManager.getCurrentAudioDevice();
    //eScope.audioDeviceAboutToStart(device); //needs refactoring
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx].setSampleRate(device->getCurrentSampleRate());
    }
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
    listenerComponent.setBounds(openButton.getX() + openButton.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    menu.setBounds(listenerComponent.getX() + listenerComponent.getWidth() + 10, recordButton.getY(), 2*recordButton.getWidth(), recordButton.getHeight());
    oscWinSizeSlider.setBounds(menu.getX() + menu.getWidth() + 10, recordButton.getY(),4 * recordButton.getWidth(), recordButton.getHeight());
    
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx].setBounds(10, 40 + idx*area.getHeight() / eScopeChanNb, getWidth() - 20, area.getHeight() / eScopeChanNb);
    }
}

void stopRecording()
{
    
}

//-------------------------------------------------------------------------------------
void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    juce::RecordingThumbnail* src = ((juce::RecordingThumbnail*)source);
    int eScopeID = src->chanID;
    auto visibRange = src->getVisibleRange();
    auto xZoom = src->getXZoom();
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        if (idx != eScopeID)
        {
            eScope[idx].setXZoom(xZoom);
            eScope[idx].setVisibleRange(visibRange);
        }
    }
}