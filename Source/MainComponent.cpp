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
    for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
    {
        //channel_rack->addPanel(eScope[idx].get(), NULL);// VSCROLLABLE + HSCROLLABLE); // RESIZER);// +SWITCHABLE);
        thumbnail_rack[idx] = std::make_unique<grape::Rack>("Channel " + juce::String(idx),false); // false for horizontal rack, true for vertical
        //channel_rack->addPanel(eScope[idx].get(), VSCROLLABLE + HSCROLLABLE + RESIZER + SWITCHABLE);
        
        channelControl[idx] = std::make_unique<ChannelControl>("Channel Control" + juce::String(idx));
        thumbnail_rack[idx]->addPanel(channelControl[idx].get(), COLLAPSIBLE);
        //channelControl[idx]->thmbNail = &escopeThumbnail[idx]->getAudioThumbnail();
        escopeThumbnail[idx] = std::make_unique<RecordingThumbnail>("Channel " + juce::String(idx));
        channelControl[idx]->thmbNail = &escopeThumbnail[idx];
        channelControl[idx]->pointeur = escopeThumbnail[idx].get();
        thumbnail_rack[idx]->addPanel(escopeThumbnail[idx].get());

        thumbnail_rack[idx]->computeSizeFromChildren();
        channel_rack->addPanel(thumbnail_rack[idx].get(), VSCROLLABLE + HSCROLLABLE + RESIZER + SWITCHABLE);

        recorder.setChannelID(idx);
        escopeThumbnail[idx]->chanID = idx;
        bool* ptr = escopeThumbnail[idx]->getTriggeredPtr();
        recorder.setTriggerPtr(ptr);
        escopeThumbnail[idx]->addChangeListener(this);
        escopeThumbnail[idx]->setDisplayThumbnailMode(recmode);
        escopeThumbnail[idx]->repaint();
    }
    initPtrToRecThumbnailTable(ESCOPE_CHAN_NB);
    recorder.AttachThumbnail(aptr, ESCOPE_CHAN_NB);

    initPtrToRecoThumbnailTable(ESCOPE_CHAN_NB);
    //recorder.AttachListener(juce::ChangeListener& eptr , ESCOPE_CHAN_NB);

    channel_rack->computeSizeFromChildren(true, true);

    // Populate horizontal display rack
    display_rack->addPanelSwitchBar(channel_rack.get());
    display_rack->addPanel(channel_rack.get(), VSCROLLABLE + HSCROLLABLE);
    display_rack->computeSizeFromChildren(true, true);

    // Populate main rack
    addPanel(header.get(), 0);
    addPanel(display_rack.get(), 0);    

#if DRIVER_MODE == 1
    deviceManager.initialise(ESCOPE_CHAN_NB, 2, nullptr, true);
#endif

#if DRIVER_MODE == 2 

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
    deviceManager.initialise(ESCOPE_CHAN_NB, 2, &xxw, true);
#endif

#if DRIVER_MODE == 8 
    
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
    deviceManager.initialise(ESCOPE_CHAN_NB, 2, &xxw, true);
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
    for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
/* {
        devManager.addAudioCallback(eScope[idx]->getAudioIODeviceCallBack());        
    }*/
    devManager.addAudioCallback(&recorder);
    // Build parameter set
    mm->buildParameterSet(this);
    // Add commands
    addCommand(RECORD, "Record");
    addCommand(OPEN_FILE, "Open file");
    addCommand(DISPLAY_MODE, "Display mode");
    addCommand(WIN_SIZE, "Window size");
    addCommand(LOAD_SETTINGS, "Load settings");
    addCommand(SAVE_SETTINGS, "Save settings");
    addCommand(SAVE_WAV, "Save Wav");
    addCommand(GO_LIVE,"Go Live");
    addCommand(THRESHOLD_LEVEL,"Threshold Level");
    addCommand(Y_SCALE, "Y Scale");
    addCommand(TRIGG, "Trig Settings");
    // setSize(1800, 1000);
#if ESCOPE_CHAN_NB == 1
    setSize(1200, 400);
#endif
#if ESCOPE_CHAN_NB == 2
    setSize(1200, 800);
#endif
#if ESCOPE_CHAN_NB == 8
    setSize(1200, 1000);
#endif
    initThresholdSettings();
    formatManager.registerBasicFormats();
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    auto& devManager = MainComponent::getAudioDeviceManager();
 /*   for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
    {
        devManager.removeAudioCallback(eScope[idx]->getAudioIODeviceCallBack());//[ToBeChanged]        
    }*/
    devManager.removeAudioCallback(&recorder);
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
    for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
    {
        recorder.prepareToPlay(samplesPerBlockExpected, sampleRate);
        escopeThumbnail[idx]->prepareToPlay(samplesPerBlockExpected, sampleRate);
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
    auto xzidx = src->getXZoomIndex();
    auto yzidx = src->getYZoomIndex();
    auto broadcasterZoomGroup = src->getZoomGroup();
    auto repaintAllAsked = src->repaintBroadcasted;
    src->repaintBroadcasted = false;
    for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
    {
        if (idx != eScopeID) //[ToBeChanged]
        {
            if (repaintAllAsked)
            {
                escopeThumbnail[idx]->repaint();
                escopeThumbnail[idx]->repaintBroadcasted = false;
            }
            else
            {
                int targetZoomGroup = escopeThumbnail[idx]->getZoomGroup();
                int targetXZoomFlag = escopeThumbnail[idx]->getXZoomFlag();
                int targetYZoomFlag = escopeThumbnail[idx]->getYZoomFlag();

                if (targetZoomGroup == broadcasterZoomGroup)
                {
                    if (targetXZoomFlag != 0)
                    {
                        escopeThumbnail[idx]->setXZoomIndex(xzidx);
                        escopeThumbnail[idx]->setDisplayXZoom(xZoom);
                        escopeThumbnail[idx]->setRange(visibRange);
                        DBG("---------------------------------------------------------------");
                        DBG("changeListenerCallback:xzidx = " << xzidx << " xZoom = " << xZoom);
                        DBG("---------------------------------------------------------------");
                    }
                    if (targetYZoomFlag != 0)
                    {
                        escopeThumbnail[idx]->setYZoomIndex(yzidx);
                        escopeThumbnail[idx]->setDisplayYZoom(yZoom);
                    }
                }
            }
        }
    }
}
//void onDialogBoxClosed(int result);

void MainComponent::onDialogBoxClosed(int result, triggerDlgData *trigDlgData)
{
    // Handle the dialog box closure here
    juce::Logger::writeToLog("Dialog box closed with result: " + juce::String(result));
    
    //bufferDlgData.enable = true;
    //bool enab = bufferDlgData->enable;
    int chan = trigDlgData->channel-1;
    int direct = trigDlgData->direction;
    double thresh = trigDlgData->threshold;
    header->SetThresholdValue(thresh); //sends ThresholdSlider.setValue(thresh);
    recorder.setThreshold(thresh);
    int pretrigg = trigDlgData->pretrigger;

    storeThresholdSettings(trigDlgData->enable, trigDlgData->channel, trigDlgData->threshold, trigDlgData->direction, trigDlgData->pretrigger);

    recorder.setThreshold(thresh);
    recorder.setTriggerChannel(chan);
    recorder.setTriggerMode(direct);
    bool enab = trigDlgData->enable;

    if (enab)
    {
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            escopeThumbnail[idx]->setXScale(RelativeToTrigger);
            escopeThumbnail[idx]->setThreshold(thresh);
            escopeThumbnail[idx]->setTriggerMode(direct);
            escopeThumbnail[idx]->setPreTrigg(pretrigg);
            if (idx == chan)
                escopeThumbnail[idx]->setTrigEnabled(true);
            else
                escopeThumbnail[idx]->setTrigEnabled(false);
        }
    }
    else
    {
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            escopeThumbnail[idx]->setXScale(Absolute);
        }    
    }
    // Perform other actions as needed
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
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            escopeThumbnail[idx]->setDisplayThumbnailMode(oscmode);
            escopeThumbnail[idx]->repaint();
        }
        return true;
    }
    case WIN_SIZE:
    {
        oscilloWinSize = source->getControlValue();
        // update pointers to the buffer and pointers used for display outside of Thumbnail
        juce::AudioBuffer<float>* recBuffer;// = recorder.getBufferPtr(0);        
        unsigned long* StartAddr = recorder.getStartAddrPtr();
        unsigned long* TriggAddr = recorder.getTriggAddrPtr();
        bool* BufferReady = recorder.getBufferReadyAddrPtr();
        bool* BufferUnderRun = recorder.getBufferUndeRunAddrPtr();
        recorder.setViewSize(oscilloWinSize);

        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            recBuffer = recorder.getBufferPtr(idx);
            escopeThumbnail[idx]->setViewSize(oscilloWinSize);
            escopeThumbnail[idx]->setBufferedToImage(recBuffer);
            escopeThumbnail[idx]->setBufferStartAddress(StartAddr);
            escopeThumbnail[idx]->setBufferTriggAddress(TriggAddr);
            escopeThumbnail[idx]->setBufferReadyAddress(BufferReady);
            escopeThumbnail[idx]->setBufferUnderRunAddress(BufferUnderRun);        
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
    case SAVE_WAV:
    {
        recorder.saveWaves();
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
        recmode = 4; // force to oscilloscope mode
        recorder.setSampleRate(smpRate); 
        recorder.setViewSize(oscilloWinSize);
        juce::AudioBuffer<float>* recBuffer;
        unsigned long* StartAddr = recorder.getStartAddrPtr();
        unsigned long* TriggAddr = recorder.getTriggAddrPtr();
        bool* BufferReady = recorder.getBufferReadyAddrPtr();

        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            lastRecording[idx] = juce::File();
            recBuffer = recorder.getBufferPtr(idx);
            escopeThumbnail[idx]->setSampleRate(smpRate);          
            escopeThumbnail[idx]->setViewSize(oscilloWinSize); 
            escopeThumbnail[idx]->setBufferedToImage(recBuffer);
            escopeThumbnail[idx]->setBufferStartAddress(StartAddr);
            escopeThumbnail[idx]->setBufferTriggAddress(TriggAddr);
            escopeThumbnail[idx]->setBufferReadyAddress(BufferReady);
            recorder.startRecording(lastRecording[0]); 
            escopeThumbnail[idx]->setDisplayThumbnailMode(recmode); 
            escopeThumbnail[idx]->repaint();
        }
        return true;
    }
    case THRESHOLD_LEVEL:
    {
        double thresholdValue = source->getControlValue();
        recorder.setThreshold(thresholdValue); 
        escopeThumbnail[0]->setThreshold(thresholdValue); 
        return true;
    }
    case Y_SCALE:
    {
        int scale = source->getControlValue();
        escopeThumbnail[0]->setYScale(scale);
        return true;
    }
    case TRIGG:
    {        
        /*
        bufferDlgData.enable = true;
        bufferDlgData.channel = 2;
        bufferDlgData.threshold = 0.33f;
        bufferDlgData.direction = 3;
        bufferDlgData.pretrigger = 10;*/

        auto* dialogBox = new MyDialogBoxComponent(&bufferDlgData);
        auto* dialogWindow = new juce::DialogWindow("Dialog Box", juce::Colours::grey, true);

        dialogWindow->setContentOwned(dialogBox, true);
        dialogWindow->setUsingNativeTitleBar(true);
        dialogWindow->setResizable(true, true);
        int width, height;
        width = dialogBox->getWidth();
        height = dialogBox->getHeight();
        dialogWindow->centreWithSize(width, height);
        dialogWindow->setVisible(true);

        // Set up a callback for when the dialog window is closed
        dialogWindow->enterModalState(true, juce::ModalCallbackFunction::create([this](int result)
            {
                bool enab = bufferDlgData.enable;
                onDialogBoxClosed(result, &bufferDlgData);
            }), true);
        return true;
    }
    default:
        return false;
    }
}




