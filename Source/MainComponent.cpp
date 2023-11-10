#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
  //  :juce::AudioDeviceManager()
{  
    addAndMakeVisible(recordButton);
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff5c5c));
    recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    addAndMakeVisible(liveButton);
    liveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x1c1c1c1c));
    liveButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    liveButton.onClick = [this] { liveButtonClicked(); };

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
    addAndMakeVisible(dispBuffSizeLabel);
    dispBuffSizeLabel.setText("Buff Size", juce::dontSendNotification);
    addAndMakeVisible(thresholdLabel);
    thresholdLabel.setText("Threshold", juce::dontSendNotification);

    addAndMakeVisible(oscWinSizeSlider);
    oscWinSizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    oscWinSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    oscWinSizeSlider.setRange(0.05, 2.00, 0.05);
    oscWinSizeSlider.setTitle("Buff Size");
    
    oscWinSizeSlider.onValueChange = [this]()
    {
        oscilloWinSize = oscWinSizeSlider.getValue();
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx].setViewSize(oscilloWinSize);
        }
    };

    addAndMakeVisible(thresholdSlider);
    thresholdSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 32);
    thresholdSlider.setTitle("Threshold");
    thresholdSlider.setRange(0.0, 1.00, 0.01);
    thresholdSlider.onValueChange = [this]()
    {
        double thresholdValue = thresholdSlider.getValue();

            eScope[0].setThreshold(thresholdValue);

    };

    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx].recThumbnail.addChangeListener(this);
        eScope[idx].recorder.addChangeListener(this);
        addAndMakeVisible(eScope[idx]);
        eScope[idx].setChannelID(idx);
    }
    openButton.onClick = [this] { openButtonClicked(); };

#if option == 1
    deviceManager.initialise(eScopeChanNb, 2, nullptr, true);
#endif

#if option == 2    
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
#endif
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
#if option == 1
    setSize(1500, 400);
#endif
#if option == 2
    setSize(1800, 1000);
#endif
    
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
        //eScope[idx].setSampleRate(device->getCurrentSampleRate());
        eScope[idx].prepareToPlay(samplesPerBlockExpected, sampleRate);
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
    juce::Rectangle  area = getLocalBounds();
    //juce::Rectangle uiarea = area.removeFromTop(40).removeFromLeft(100).reduced(10);
    recordButton.setBounds(area.removeFromTop(40).removeFromLeft(100).reduced(10));

    int numSlider = 3;
 
    int sliderWidth = (area.getWidth() - 4 * recordButton.getWidth()) / numSlider;

    liveButton.setBounds(recordButton.getX() + recordButton.getWidth() + 10, recordButton.getY(), recordButton.getWidth(), recordButton.getHeight());
    openButton.setBounds(liveButton.getX() + liveButton.getWidth() + 10, liveButton.getY(), liveButton.getWidth(), liveButton.getHeight());
    menu.setBounds(openButton.getX() + openButton.getWidth() + 10, recordButton.getY(), 2*recordButton.getWidth(), recordButton.getHeight());
    
    oscWinSizeSlider.setBounds(menu.getX() + menu.getWidth() + 10, recordButton.getY(), sliderWidth, recordButton.getHeight());
    dispBuffSizeLabel.setBounds(oscWinSizeSlider.getX()+ oscWinSizeSlider.getWidth()/2- recordButton.getWidth()/2, recordButton.getY()-10, recordButton.getWidth(), recordButton.getHeight());
    
    thresholdSlider.setBounds(oscWinSizeSlider.getX() + oscWinSizeSlider.getWidth() + 10, recordButton.getY(), sliderWidth, recordButton.getHeight());
    thresholdLabel.setBounds(thresholdSlider.getX() + thresholdSlider.getWidth() / 2 - recordButton.getWidth()/2, recordButton.getY() - 10, recordButton.getWidth(), recordButton.getHeight());


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