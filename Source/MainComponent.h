#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
//#include "eScope.h"
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"
#include "ChannelControl.h"
#include "Header.h"
#include "TriggerDialog.h"
#include "PopUpMenu.h"

// Now defined in Projucer Project
//#define option 2 //1 2 or 8
/*
#if option == 1
const int eScopeChanNb = 1;
#endif // option = 1

#if option == 2
const int eScopeChanNb = 2;
#endif // option = 1

#if option == 8
const int eScopeChanNb = 8;
#endif // option = 1*/

const int eScopeChanNb = ESCOPE_CHAN_NB;

//enum horizontalScale { Absolute, RelativeToTrigger };
//enum verticalScale { Linear, dB };

class Rack;
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public grape::Rack, public juce::AudioSource, public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    //==============================================================================
//    void paint (juce::Graphics& g) override;
//    void resized() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source);

    // Command management stuff
    bool executeCommand(int id, grape::Control* source) override;
    
    /** A subclass should call this from their constructor, to set up the audio. */
    void setAudioChannels(int numInputChannels, int numOutputChannels, const juce::XmlElement* const storedSettings = nullptr);



    /** Shuts down the audio device and clears the audio source.

        This method should be called in the destructor of the derived class
        otherwise an assertion will be triggered.
    */
    void shutdownAudio();


    juce::AudioDeviceManager& deviceManager;
    triggerDlgData bufferDlgData;
    //void initThresholdSettings(void);
    //void storeThresholdSettings(bool triggerEnabled, int triggerChannel, float triggerThreshold, int triggerDirection, int preTriggerPercent);
    void initThresholdSettings(void)
    {
#if AUDIO_SOURCE == 1 //debug mode
        bufferDlgData.enable = true;
        bufferDlgData.channel = 1;
        bufferDlgData.threshold = 0.500;
        bufferDlgData.direction = ThresholdRising;
        bufferDlgData.pretrigger = 50;
#else
        bufferDlgData.enable = true;
        bufferDlgData.channel = 1;
        bufferDlgData.threshold = 0.50f;
        bufferDlgData.direction = ThresholdRising;
        bufferDlgData.pretrigger = 50;
#endif
    }
    void storeThresholdSettings(bool triggerEnabled, int triggerChannel, float triggerThreshold, int triggerDirection, int preTriggerPercent)
    {
        bufferDlgData.enable = triggerEnabled;
        bufferDlgData.channel = triggerChannel;
        bufferDlgData.threshold = triggerThreshold;
        bufferDlgData.direction = triggerDirection;
        bufferDlgData.pretrigger = preTriggerPercent;
    }
    void MainComponent::onDialogBoxClosed(int result, triggerDlgData* trigDlgData);

private:
    friend Header;    

    // Audio stuff
    juce::AudioDeviceManager defaultDeviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    bool usingCustomDeviceManager;

    juce::AudioFormatManager formatManager;                    // [3]    
    juce::File lastRecording[ESCOPE_CHAN_NB];

    // GUI stuff
    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<Header> header;

    std::unique_ptr<RecordingThumbnail> escopeThumbnail[ESCOPE_CHAN_NB];
    juce::AudioThumbnail* ptrToRecThumbnail[ESCOPE_CHAN_NB];
    juce::AudioThumbnail** aptr;

    std::unique_ptr<RecordingThumbnail>* ptrToRecoThumbnail[ESCOPE_CHAN_NB];
    std::unique_ptr<RecordingThumbnail>** eptr;

    AudioRecorder recorder{ escopeThumbnail[1]->getAudioThumbnail() };//{ placeHolderThumbnail };

    std::unique_ptr<Rack> display_rack; // this is a horizontal rack for placing a vertical panel switch bar
    std::unique_ptr<Rack> channel_rack; // this is for encapsulating eScope channels
                                        // in a vertical rack inside the parent vertical rack
                                        // to offer a global scrollbar for all channels at once
    std::unique_ptr<Rack> thumbnail_rack[ESCOPE_CHAN_NB];
    std::unique_ptr<ChannelControl> channelControl[ESCOPE_CHAN_NB];

    std::unique_ptr<grape::DREAMLookAndFeel> laf;

    // Parameter management stuff
    std::unique_ptr<grape::ParameterContainer> pc;
    std::unique_ptr<grape::MappingManager> mm;

    int recmode = 2; // can be 1= track view or 2= oscilloscope
    double oscilloWinSize = 0.05;
    //-------------------------------------------------------------------------------------
    // initialize table of pointers to the escope Thumbnails
    void initPtrToRecThumbnailTable(int channelNumber)
    {
        for (int idx = 0; idx < channelNumber; idx++)
        {
            ptrToRecThumbnail[idx] = &escopeThumbnail[idx]->getAudioThumbnail();
        }
        aptr = ptrToRecThumbnail;
    }
    //-------------------------------------------------------------------------------------
    // initialize table of pointers to the escope Thumbnails
    void initPtrToRecoThumbnailTable(int channelNumber)
    {
        for (int idx = 0; idx < channelNumber; idx++)
        {
            ptrToRecoThumbnail[idx] = &escopeThumbnail[idx];
        }
        eptr = ptrToRecoThumbnail;
    }
    //-------------------------------------------------------------------------------------   
    juce::AudioDeviceManager& getAudioDeviceManager() //getting access to the built in AudioDeviceManager
    {
        return deviceManager; 
    }
 //-------------------------------------------------------------------------------------
    void configureStreaming(juce::File file)
    {
        juce::FileInputStream inputStream(file);

        int idx = 0;

        while (!inputStream.isExhausted()) // [3]
        {
            auto line = inputStream.readNextLine();
            auto* reader = formatManager.createReaderFor(line);

            if (reader != nullptr)
            {
                auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);

                escopeThumbnail[0]->setSource(new juce::FileInputSource(line));
                recorder.setSampleRate(reader->sampleRate);
                escopeThumbnail[idx]->setSampleRate(reader->sampleRate);
                escopeThumbnail[idx]->setDisplayThumbnailMode(0);
                escopeThumbnail[idx]->repaint();
                escopeThumbnail[idx]->setDisplayYZoom(1.0);

                auto area = getLocalBounds();
                escopeThumbnail[idx]->setBounds(area);
                idx++;
            }
        }
    }
//-------------------------------------------------------------------------------------
    void startRecording()
    {
        if (!juce::RuntimePermissions::isGranted(juce::RuntimePermissions::writeExternalStorage))
        {
            SafePointer<MainComponent> safeThis(this);

            juce::RuntimePermissions::request(juce::RuntimePermissions::writeExternalStorage,
                [safeThis](bool granted) mutable
                {
                    if (granted)
                        safeThis->startRecording();
                });
            return;
        }
#if (JUCE_ANDROID || JUCE_IOS)
        auto parentDir = File::getSpecialLocation(File::tempDirectory);
#else
        auto parentDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
#endif
        auto& devManager = MainComponent::getAudioDeviceManager();
        auto device = devManager.getCurrentAudioDevice();
        auto smpRate = device->getCurrentSampleRate();
        
        //recmode = menu.getSelectedItemIndex();      //removed during merge operation 23-11-2023  
        //recmode = 1; //trackView //removed during merge operation 23-11-2023
        //eScope.recThumbnail.setSampleRate(eScope.rec.getSampleRate()); //needs refactoring
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            //[ToBeChanged]
            //eScope[idx]->setSampleRate(smpRate);
            lastRecording[idx] = parentDir.getNonexistentChildFile("eScope Recording", ".wav");
            //eScope[idx]->startRecording(lastRecording[idx]);
            //eScope[idx]->setDisplayThumbnailMode(recmode);
            //[inTheProcess]
            recorder.startRecording(lastRecording[idx]); //eScope[idx]->startRecording(lastRecording[idx]);            
            escopeThumbnail[idx]->setDisplayThumbnailMode(recmode);//eScope[idx]->setDisplayThumbnailMode(recmode); [1]
            escopeThumbnail[idx]->repaint();                       //eScope[idx]->setDisplayThumbnailMode(recmode); [2]

            //recorder.setSampleRate(smpRate);
            //escopeThumbnail[idx]->setSampleRate(smpRate);

        }
        //grape// header->recordButton.setButtonText("Stop");
    }
//-------------------------------------------------------------------------------------
    void stopRecording()
    {
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            //eScope[idx]->recorder.stop();//[ToBeChanged]
            //[inTheProcess]
            recorder.stop(); // to be checked !!
        }
#if JUCE_CONTENT_SHARING
        SafePointer<AudioRecordingDemo> safeThis(this);
        File fileToShare = lastRecording;

        ContentSharer::getInstance()->shareFiles(Array<URL>({ URL(fileToShare) }),
            [safeThis, fileToShare](bool success, const String& error)
            {
                if (fileToShare.existsAsFile())
                    fileToShare.deleteFile();

                if (!success && error.isNotEmpty())
                    NativeMessageBox::showAsync(MessageBoxOptions()
                        .withIconType(MessageBoxIconType::WarningIcon)
                        .withTitle("Sharing Error")
                        .withMessage(error),
                        nullptr);
            });
#endif
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            lastRecording[idx] = juce::File();
            escopeThumbnail[idx]->setDisplayThumbnailMode(0); //eScope[idx]->setDisplayThumbnailMode(0); [1]
            escopeThumbnail[idx]->repaint();                  //eScope[idx]->setDisplayThumbnailMode(0); [2]
            escopeThumbnail[idx]->setDisplayYZoom(1.0);
        }
        //grape// header->recordButton.setButtonText("Record");
        
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
