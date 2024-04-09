#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
#include "eScope.h"
#include "ChannelControl.h"
#include "Header.h"

#define option 1 //1 2 or 8
#if option == 1
const int eScopeChanNb = 1;
#endif // option = 1

#if option == 2
const int eScopeChanNb = 2;
#endif // option = 1

#if option == 8
const int eScopeChanNb = 8;
#endif // option = 1

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

private:
    friend Header;

    // Audio stuff
    juce::AudioDeviceManager defaultDeviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    bool usingCustomDeviceManager;

    juce::AudioFormatManager formatManager;                    // [3]    
    juce::File lastRecording[eScopeChanNb];

    // GUI stuff
    std::unique_ptr<juce::FileChooser> chooser;

    std::unique_ptr<Header> header;
    //std::unique_ptr<EScope> eScope[eScopeChanNb]; //[ToBeChanged]

    RecordingThumbnail recThumbnail[eScopeChanNb];
    juce::AudioRecorder recorder{ recThumbnail[0].getAudioThumbnail()};

    std::unique_ptr<Rack> display_rack; // this is a horizontal rack for placing a vertical panel switch bar
    std::unique_ptr<Rack> channel_rack; // this is for encapsulating eScope channels
                                        // in a vertical rack inside the parent vertical rack
                                        // to offer a global scrollbar for all channels at once
    std::unique_ptr<Rack> thumbnail_rack[8];
    std::unique_ptr<ChannelControl> channelControl[8];
    std::unique_ptr<Panel> channelDisplay[8];

    std::unique_ptr<grape::DREAMLookAndFeel> laf;

    // Parameter management stuff
    std::unique_ptr<grape::ParameterContainer> pc;
    std::unique_ptr<grape::MappingManager> mm;

    int recmode = 2; // can be 1= track view or 2= oscilloscope
    double oscilloWinSize = 0.05;

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
            {   //[ToBeChanged]
                auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                //eScope[idx]->setSource(new juce::FileInputSource(line));
                //eScope[idx]->setSampleRate(reader->sampleRate);
                //eScope[idx]->setDisplayThumbnailMode(0);// request waveform to fill viewing zone
                //eScope[idx]->setDisplayYZoom(1.0);
                //eScope[idx]->resized();
                //[inTheProcess]
                recThumbnail->setSource(new juce::FileInputSource(line));
                recorder.setSampleRate(reader->sampleRate); //eScope[idx]->setSampleRate(reader->sampleRate);[1]
                recThumbnail[idx].setSampleRate(reader->sampleRate);//eScope[idx]->setSampleRate(reader->sampleRate);[2]

                recThumbnail[idx].setDisplayThumbnailMode(0);
                recThumbnail[idx].repaint();

                recThumbnail[idx].setDisplayYZoom(1.0);

                auto area = getLocalBounds();
                recThumbnail[idx].setBounds(area);
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
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            //[ToBeChanged]
            //eScope[idx]->setSampleRate(smpRate);
            lastRecording[idx] = parentDir.getNonexistentChildFile("eScope Recording", ".wav");
            //eScope[idx]->startRecording(lastRecording[idx]);
            //eScope[idx]->setDisplayThumbnailMode(recmode);
            //[inTheProcess]
            recorder.startRecording(lastRecording[idx]); //eScope[idx]->startRecording(lastRecording[idx]);            
            recThumbnail[idx].setDisplayThumbnailMode(recmode);//eScope[idx]->setDisplayThumbnailMode(recmode); [1]
            recThumbnail[idx].repaint();                       //eScope[idx]->setDisplayThumbnailMode(recmode); [2]

            //recorder.setSampleRate(smpRate);
            //recThumbnail[idx].setSampleRate(smpRate);

        }
        //grape// header->recordButton.setButtonText("Stop");
    }
//-------------------------------------------------------------------------------------
    void stopRecording()
    {
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            //eScope[idx]->recorder.stop();//[ToBeChanged]
            //[inTheProcess]
            recorder.stop();
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
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            //[ToBeChanged]
            lastRecording[idx] = juce::File();
            //eScope[idx]->setDisplayThumbnailMode(0);// request waveform to fill viewing zone
            //eScope[idx]->setDisplayYZoom(1.0);
            //[inTheProcess]
            recThumbnail[idx].setDisplayThumbnailMode(0); //eScope[idx]->setDisplayThumbnailMode(0); [1]
            recThumbnail[idx].repaint();                  //eScope[idx]->setDisplayThumbnailMode(0); [2]
            recThumbnail[idx].setDisplayYZoom(1.0);
        }
        //grape// header->recordButton.setButtonText("Record");
        
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
