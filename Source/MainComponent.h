#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
#include "eScope.h"
#include "ListenerComponent.h"
#include "Header.h"
const int eScopeChanNb = 8;

class Rack;
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public sgul::Rack, public juce::AudioSource
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
    juce::AudioDeviceManager defaultDeviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    bool usingCustomDeviceManager;

    //==============================================================================
    // Your private member variables go here...
    juce::AudioFormatManager formatManager;                    // [3]    
    std::unique_ptr<Header> header;
    std::unique_ptr<EScope> eScope[eScopeChanNb];
    juce::File lastRecording[eScopeChanNb];
 //   juce::EScope eScope[eScopeChanNb];
 
    std::unique_ptr<sgul::DREAMLookAndFeel> laf;

    // Parameter management stuff
    std::unique_ptr<sgul::ParameterContainer> pc;
    std::unique_ptr<sgul::MappingManager> mm;

 
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
                eScope[idx]->setSource(new juce::FileInputSource(line));
                eScope[idx]->setSampleRate(reader->sampleRate);
                eScope[idx]->setDisplayThumbnailMode(0);// request waveform to fill viewing zone
                eScope[idx]->setDisplayYZoom(1.0);
                eScope[idx]->resized();
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
        //eScope.recThumbnail.setSampleRate(eScope.rec.getSampleRate()); //needs refactoring
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx]->setSampleRate(smpRate);
            lastRecording[idx] = parentDir.getNonexistentChildFile("eScope Recording", ".wav");
            eScope[idx]->startRecording(lastRecording[idx]);
            eScope[idx]->setDisplayThumbnailMode(header->getRecMode());
        }
        header->recordButton.setButtonText("Stop");
    }
//-------------------------------------------------------------------------------------
    void stopRecording()
    {
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx]->rec.stop();
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
            lastRecording[idx] = juce::File();
            eScope[idx]->setDisplayThumbnailMode(0);// request waveform to fill viewing zone
            eScope[idx]->setDisplayYZoom(1.0);
        }
        header->recordButton.setButtonText("Record");
        
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
