#include "MainComponent.h"
#include "CommandList.h"
#include "..\GRAPE\Source\GRAPE.h"

//==============================================================================
MainComponent::MainComponent() : Rack("Main", true), deviceManager(defaultDeviceManager),
usingCustomDeviceManager(false)
{  
    // Use Dream Look and Feel 
    laf = std::make_unique <grape::DREAMLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(laf.get());

    // Instantiate Parameter Container & Mapping Manager and link Mapping Manager to controls
    pc = std::make_unique<grape::ParameterContainer>("eScope");
    mm = std::make_unique<grape::MappingManager>(pc.get());
    grape::Control::setMappingManager(mm.get());
    grape::Panel::setMappingManager(mm.get());

    // Instantiate top level rack elements   
    // - Create header instance
    header = std::make_unique<Header>();
    // - Create horizontal display rack instance
    display_rack = std::make_unique<grape::Rack>("Display", "", false);

    // Instantiate display rack elements   
    // - Create channel rack instance
    channel_rack = std::make_unique<grape::Rack>("Channels", true);

    // Create eScope instances and populate channel rack
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        eScope[idx] = std::make_unique<EScope>("Channel" + juce::String(idx));
        eScope[idx]->setChannelID(idx);
        eScope[idx]->recThumbnail.addChangeListener(this);
        eScope[idx]->setDisplayThumbnailMode(recmode);
        channel_rack->addPanel(eScope[idx].get(), RESIZER + SWITCHABLE);
    }
    channel_rack->computeSizeFromChildren(true, true);

    // Populate horizontal display rack
    display_rack->addPanelSwitchBar(channel_rack.get());
    display_rack->addPanel(channel_rack.get(), VSCROLLABLE + HSCROLLABLE);
    display_rack->computeSizeFromChildren(true, true);

    // Populate main rack
    addPanel(header.get(), 0);
    addPanel(display_rack.get(), 0);
    

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

    // Add commands
    addCommand(RECORD, "Record");
    addCommand(OPEN_FILE, "Open file");
    addCommand(DISPLAY_MODE, "Display mode");
    addCommand(WIN_SIZE, "Window size");
    addCommand(LOAD_SETTINGS, "Load settings");
    addCommand(SAVE_SETTINGS, "Save settings");

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

//-------------------------------------------------------------------------------------
void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    RecordingThumbnail* src = ((RecordingThumbnail*)source);
    int eScopeID = src->chanID;
    auto visibRange = src->getVisibleRange();
    auto xZoom = src->getXZoom();
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        if (idx != eScopeID)
        {
            eScope[idx]->setXZoom(xZoom);
            eScope[idx]->setVisibleRange(visibRange);
        }
    }
}

bool MainComponent::executeCommand(int id, grape::Control* source)
{
    switch (id)
    {
    case RECORD:
        if (source->getControlValue())
            startRecording();
        else
            stopRecording();
        return true;
    case OPEN_FILE:
    {
        chooser = std::make_unique<juce::FileChooser>("Select a Wave List...",
            juce::File{}, "*.txt");
        auto chooserFlags = juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();

                if (file != juce::File{})
                    configureStreaming(file);
            });

        return true;
    }
    case DISPLAY_MODE:
    {
        int oscmode = source->getControlValue();
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx]->setDisplayThumbnailMode(oscmode);
        }
        return true;
    }
    case WIN_SIZE:
    {
        oscilloWinSize = source->getControlValue();
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx]->setViewSize(oscilloWinSize);
        }
        return true;
    };
    case LOAD_SETTINGS:
    {
        chooser = std::make_unique<juce::FileChooser>("Load settings",
            juce::File{}, "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();

                if (file != juce::File{})
                    mm->loadParameters(file, this);
            });
        return true;
    }
    case SAVE_SETTINGS:
    {
        chooser = std::make_unique<juce::FileChooser>("Save settings as...",
            juce::File{}, "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();

                if (file != juce::File{})
                    mm->saveParameters(file);
            });
        return true;
    }
    default:
        return false;
    }
}
