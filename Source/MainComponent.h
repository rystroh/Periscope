#pragma once

#include <JuceHeader.h>
//#include "AudioRecording.h"
#include "eScope.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent                                                                  
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
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...
    juce::TextButton recordButton{ "Record" };
    juce::TextButton openButton{ "Open File" };
    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;                    // [3]    
  
    juce::File lastRecording;
    juce::EScope eScope;
 //-------------------------------------------------------------------------------------   
    juce::AudioDeviceManager& getAudioDeviceManager() //getting access to the built in AudioDeviceManager
    {
        return deviceManager; 
    }
 //-------------------------------------------------------------------------------------
    void openButtonClicked()
    {
        chooser = std::make_unique<juce::FileChooser>("Select a Wave file...",
                                                       juce::File{},"*.wav");
        auto chooserFlags = juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();

                if (file != juce::File{})
                {
                    auto* reader = formatManager.createReaderFor(file);

                    if (reader != nullptr)
                    {
                        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                        eScope.recThumbnail.setSource(new juce::FileInputSource(file));
                        eScope.recThumbnail.setSampleRate(reader->sampleRate);
                        eScope.recThumbnail.setDisplayThumbnailMode(0);// request waveform to fill viewing zone
                        eScope.recThumbnail.setDisplayYZoom(1.0);                        
                    }
                }
            });
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
        lastRecording = parentDir.getNonexistentChildFile("eScope Recording", ".wav");
        eScope.recThumbnail.setSampleRate(eScope.rec.getSampleRate());
        eScope.rec.startRecording(lastRecording);
        recordButton.setButtonText("Stop");
        //recordingThumbnail.setDisplayThumbnailMode(1); // in record mode scrolling display
        eScope.recThumbnail.setDisplayThumbnailMode(3);
    }
//-------------------------------------------------------------------------------------
    void stopRecording()
    {
        eScope.rec.stop();
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
        lastRecording = juce::File();
        recordButton.setButtonText("Record");

        eScope.recThumbnail.setDisplayThumbnailMode(0);// request waveform to fill viewing zone
        eScope.recThumbnail.setDisplayYZoom(1.0);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
