#include "MainComponent.h"
#include "..\SGUL\Source\SGUL.h"

//==============================================================================
MainComponent::MainComponent() : Rack("Main", true), deviceManager(defaultDeviceManager),
usingCustomDeviceManager(false)
{  
    // Use Dream Look and Feel 
    laf = std::make_unique <sgul::DREAMLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(laf.get());

    // Instantiate Parameter Container & Mapping Manager and link Mapping Manager to controls
    pc = std::make_unique<sgul::ParameterContainer>("TestTree");
    mm = std::make_unique<sgul::MappingManager>(pc.get());
    sgul::Control::setMappingManager(mm.get());

    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx] = std::make_unique<EScope>("Channel" + juce::String(idx));
    }

    header = std::make_unique<Header>(this);
    addPanel(header.get(), 0);
    addPanelSwitchBar();

    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx]->setDisplayThumbnailMode(header->getRecMode());
        addPanel(eScope[idx].get(), RESIZER + DISPLAY_NAME + COLLAPSIBLE);
    }
    
    computeSizeFromChildren(true, true);

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
        devManager.addAudioCallback(eScope[idx]->getAudioIODeviceCallBack());
    }

    // Build parameter set
    mm->buildParameterSet(this);

    // setSize(1800, 1000);
    setSize(900, 500);
    formatManager.registerBasicFormats();
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    auto& devManager = MainComponent::getAudioDeviceManager();
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        devManager.removeAudioCallback(eScope[idx]->getAudioIODeviceCallBack());
    }
    jassert(audioSourcePlayer.getCurrentSource() == nullptr);
}

void MainComponent::setAudioChannels(int numInputChannels, int numOutputChannels, const juce::XmlElement* const xml)
{
    juce::String audioError;

    if (usingCustomDeviceManager && xml == nullptr)
    {
        auto setup = deviceManager.getAudioDeviceSetup();

        if (setup.inputChannels.countNumberOfSetBits() != numInputChannels
            || setup.outputChannels.countNumberOfSetBits() != numOutputChannels)
        {
            setup.inputChannels.clear();
            setup.outputChannels.clear();

            setup.inputChannels.setRange(0, numInputChannels, true);
            setup.outputChannels.setRange(0, numOutputChannels, true);

            audioError = deviceManager.setAudioDeviceSetup(setup, false);
        }
    }
    else
    {
        audioError = deviceManager.initialise(numInputChannels, numOutputChannels, xml, true);
    }

    jassert(audioError.isEmpty());

    deviceManager.addAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(this);
}

void MainComponent::shutdownAudio()
{
    audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);

    // other audio callbacks may still be using the device
    if (!usingCustomDeviceManager)
        deviceManager.closeAudioDevice();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    auto& devManager = MainComponent::getAudioDeviceManager();
    auto device = devManager.getCurrentAudioDevice();
    //eScope.audioDeviceAboutToStart(device); //needs refactoring
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx]->setSampleRate(device->getCurrentSampleRate());
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
//void MainComponent::paint (juce::Graphics& g)
//{
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//}
//-------------------------------------------------------------------------------------
//void MainComponent::resized()
//{
//}
