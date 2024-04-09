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
        //[ToBeChanged]
        eScope[idx] = std::make_unique<EScope>("Channel " + juce::String(idx));
        eScope[idx]->setChannelID(idx);
        eScope[idx]->recThumbnail.addChangeListener(this);
        eScope[idx]->setDisplayThumbnailMode(recmode);
        //[inTheProcess]
        recorder.setChannelID(idx);               //eScope[idx]->setChannelID(idx); [1]
        recThumbnail[idx].chanID = idx;                //eScope[idx]->setChannelID(idx); [2]
        bool* ptr = recThumbnail[idx].getTriggeredPtr();  //eScope[idx]->setChannelID(idx);[3]
        recorder.setTriggerPtr(ptr);                 //eScope[idx]->setChannelID(idx);[4]

        recThumbnail[idx].addChangeListener(this); //eScope[idx]->recThumbnail.addChangeListener(this);
        
        recThumbnail[idx].setDisplayThumbnailMode(recmode);//eScope[idx]->setDisplayThumbnailMode(recmode);[1]
        recThumbnail[idx].repaint();//eScope[idx]->setDisplayThumbnailMode(recmode);[2]



        //channel_rack->addPanel(eScope[idx].get(), NULL);// VSCROLLABLE + HSCROLLABLE); // RESIZER);// +SWITCHABLE);
        thumbnail_rack[idx] = std::make_unique<grape::Rack>("Channel " + juce::String(idx),false); // false for horizontal rack, true for vertical
        //channel_rack->addPanel(eScope[idx].get(), VSCROLLABLE + HSCROLLABLE + RESIZER + SWITCHABLE);
        
        channelControl[idx] = std::make_unique<ChannelControl>("Channel Control" + juce::String(idx));
        thumbnail_rack[idx]->addPanel(channelControl[idx].get(), COLLAPSIBLE);
        thumbnail_rack[idx]->addPanel(eScope[idx].get()); //[ToBeChanged]
        thumbnail_rack[idx]->computeSizeFromChildren();
        channel_rack->addPanel(thumbnail_rack[idx].get(), VSCROLLABLE + HSCROLLABLE + RESIZER + SWITCHABLE);
    }
    channel_rack->computeSizeFromChildren(true, true);

    // Populate horizontal display rack
    display_rack->addPanelSwitchBar(channel_rack.get());
    display_rack->addPanel(channel_rack.get(), VSCROLLABLE + HSCROLLABLE);
    display_rack->computeSizeFromChildren(true, true);

    // Populate main rack
    addPanel(header.get(), 0);
    addPanel(display_rack.get(), 0);
    

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

#if option == 8 
    
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
    auto& result = devManager.getAudioDeviceSetup();
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
    addCommand(GO_LIVE,"Go Live");
    addCommand(THRESHOLD_LEVEL,"Threshold Level");
    addCommand(Y_SCALE, "Y Scale");

    // setSize(1800, 1000);
#if option == 1
    setSize(1200, 400);
#endif
#if option == 2
    setSize(1200, 800);
#endif
#if option == 8
    setSize(1200, 1000);
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
        devManager.removeAudioCallback(eScope[idx]->getAudioIODeviceCallBack());//[ToBeChanged]
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
        //eScope[idx].setSampleRate(device->getCurrentSampleRate());
        eScope[idx]->prepareToPlay(samplesPerBlockExpected, sampleRate);//[ToBeChanged]
        //[inTheProcess]
        recorder.prepareToPlay(samplesPerBlockExpected, sampleRate); //eScope[idx]->prepareToPlay [1]
        recThumbnail[idx].prepareToPlay(samplesPerBlockExpected, sampleRate); //eScope[idx]->prepareToPlay [2]
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
    int eScopeID = src->chanID; //[ToBeChanged]
    auto visibRange = src->getVisibleRange();
    auto xZoom = src->getXZoom();
    auto yZoom = src->getDisplayYZoom();
    auto broadcasterZoomGroup = src->getZoomGroup();
    for (int idx = 0; idx < eScopeChanNb; idx++)
    {
        if (idx != eScopeID) //[ToBeChanged]
        {
            int targetZoomGroup= eScope[idx]->getZoomGroup(); //[ToBeChanged]
            int targetXZoomFlag = eScope[idx]->getXZoomFlag();
            int targetYZoomFlag = eScope[idx]->getYZoomFlag();
            //[inTheProcess]
            int targetZoomGroupX = recThumbnail[idx].getZoomGroup();
            int targetXZoomFlagX = recThumbnail[idx].getXZoomFlag();
            int targetYZoomFlagX = recThumbnail[idx].getYZoomFlag();

            if (targetZoomGroup == broadcasterZoomGroup)
            {
                if (targetXZoomFlag != 0)
                {
                    eScope[idx]->setXZoom(xZoom); //[ToBeChanged]
                    eScope[idx]->setVisibleRange(visibRange);
                    //[inTheProcess]
                    recThumbnail[idx].setDisplayXZoom(xZoom); //eScope[idx]->setXZoom(xZoom); [1]
                    recThumbnail[idx].setRange(visibRange); //eScope[idx]->setVisibleRange(visibRange); [1]
                }
                if (targetYZoomFlag != 0)
                {
                    eScope[idx]->setDisplayYZoom(yZoom); //[ToBeChanged]
                    //[inTheProcess]
                    recThumbnail[idx].setDisplayYZoom(yZoom); //eScope[idx]->setDisplayYZoom(yZoom); [1]
                }                
            }
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
            eScope[idx]->setDisplayThumbnailMode(oscmode); //[ToBeChanged]
            //[inTheProcess]
            recThumbnail[idx].setDisplayThumbnailMode(oscmode);
            recThumbnail[idx].repaint();
        }
        return true;
    }
    case WIN_SIZE:
    {
        oscilloWinSize = source->getControlValue();

        // update pointers to the buffer and pointers used for display outside of Thumbnail
        juce::AudioBuffer<float>* recBuffer = recorder.getBufferPtr();        
        unsigned long* StartAddr = recorder.getStartAddrPtr();
        unsigned long* TriggAddr = recorder.getTriggAddrPtr();
        bool* BufferReady = recorder.getBufferReadyAddrPtr();
        recorder.setViewSize(oscilloWinSize);

        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx]->setViewSize(oscilloWinSize); //[ToBeChanged]
            //[inTheProcess]            
            recThumbnail[idx].setViewSize(oscilloWinSize);
            recThumbnail[idx].setBufferedToImage(recBuffer);
            recThumbnail[idx].setBufferStartAddress(StartAddr);
            recThumbnail[idx].setBufferTriggAddress(TriggAddr);
            recThumbnail[idx].setBufferReadyAddress(BufferReady);
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
    case GO_LIVE:
    {
        auto& devManager = MainComponent::getAudioDeviceManager();
        auto device = devManager.getCurrentAudioDevice();
        auto smpRate = device->getCurrentSampleRate();
        int samplesPerBlockExpected = 512;
        samplesPerBlockExpected = device->getCurrentBufferSizeSamples();
        prepareToPlay(samplesPerBlockExpected, smpRate);
        //oscilloWinSize = oscWinSizeSlider.getValue(); //merge issue 23-11-2023
        // oscilloWinSize should be accessible here at top level
        recmode = 4; // force to oscilloscope mode
        recorder.setSampleRate(smpRate); //eScope[idx]->setSampleRate(smpRate); [1]
        recorder.setViewSize(oscilloWinSize);//eScope[idx]->setViewSize(oscilloWinSize); [1]
        juce::AudioBuffer<float>* recBuffer = recorder.getBufferPtr();//eScope[idx]->setViewSize(oscilloWinSize); [3]
        unsigned long* StartAddr = recorder.getStartAddrPtr();//eScope[idx]->setViewSize(oscilloWinSize); [5]
        unsigned long* TriggAddr = recorder.getTriggAddrPtr();//eScope[idx]->setViewSize(oscilloWinSize); [5]
        bool* BufferReady = recorder.getBufferReadyAddrPtr();//eScope[idx]->setViewSize(oscilloWinSize); [5]

        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            //[ToBeChanged]
            eScope[idx]->setSampleRate(smpRate);
            eScope[idx]->setViewSize(oscilloWinSize);
            lastRecording[idx] = juce::File();
            eScope[idx]->startRecording(lastRecording[idx]);
            eScope[idx]->setDisplayThumbnailMode(recmode);
            //[inTheProcess]
            recThumbnail[idx].setSampleRate(smpRate);//eScope[idx]->setSampleRate(smpRate); [2]            
            recThumbnail[idx].setViewSize(oscilloWinSize); //eScope[idx]->setViewSize(oscilloWinSize); [2]
            recThumbnail[idx].setBufferedToImage(recBuffer);//eScope[idx]->setViewSize(oscilloWinSize); [4]
            recThumbnail[idx].setBufferStartAddress(StartAddr);//eScope[idx]->setViewSize(oscilloWinSize); [5]
            recThumbnail[idx].setBufferTriggAddress(TriggAddr);//eScope[idx]->setViewSize(oscilloWinSize); [6]
            recThumbnail[idx].setBufferReadyAddress(BufferReady);//eScope[idx]->setViewSize(oscilloWinSize); [7]
            recorder.startRecording(lastRecording[0]); //eScope[idx]->startRecording(lastRecording[idx]);[1]
            recThumbnail[idx].setDisplayThumbnailMode(recmode); //eScope[idx]->setDisplayThumbnailMode(recmode); [1]
            recThumbnail[idx].repaint(); //eScope[idx]->setDisplayThumbnailMode(recmode); [2]
        }
        return true;
    }
    case THRESHOLD_LEVEL:
    {
        double thresholdValue = source->getControlValue();
        eScope[0]->setThreshold(thresholdValue); //[ToBeChanged]
        //[inTheProcess]
        recorder.setThreshold(thresholdValue); //eScope[0]->setThreshold(thresholdValue); [1]
        recThumbnail[0].setThreshold(thresholdValue); //eScope[0]->setThreshold(thresholdValue); [2]
        return true;
    }
    case Y_SCALE:
    {
        int scale = source->getControlValue();
        eScope[0]->setYScale(scale); //[ToBeChanged]
        //[inTheProcess]
        recThumbnail[0].setYScale(scale); // eScope[0]->setYScale(scale); [1]
        return true;
    }

    default:
        return false;
    }
}
