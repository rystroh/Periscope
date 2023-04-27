#pragma once

#include <JuceHeader.h>
#include "AudioRecording.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,                       
                       public juce::Slider::Listener
                       
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
    

    //juce::AudioDeviceManager audioDeviceManager; //external audioDeviceManager not needed
    juce::RecordingThumbnail recordingThumbnail;
    juce::AudioRecorder recorder{ recordingThumbnail.getAudioThumbnail() };
    juce::File lastRecording;
    juce::Slider levelSlider;
    juce::Label levelLabel;

    juce::Slider xZoomSlider;
    juce::Label xZoomLabel;
    
    juce::AudioDeviceManager& getAudioDeviceManager() //getting access to the built in AudioDeviceManager
    {
        return deviceManager; 
    }
//-------------------------------------------------------------------------------------
    void sliderValueChanged(juce::Slider* slider) override
    {
        double value;
        if (slider == &levelSlider)
        {
            value = levelSlider.getValue();
            recordingThumbnail.setDisplayYZoom(value);
            //recordingThumbnail.getAudioThumbnail().drawChannels(g, area, start, end, vzoom);
        }
        if (slider == &xZoomSlider)
        {
            value = xZoomSlider.getValue();
            recordingThumbnail.setDisplayXZoom(value);
            //recordingThumbnail.getAudioThumbnail().drawChannels(g, area, start, end, vzoom);
        }
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
                        //transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                        //playButton.setEnabled(true);
                        recordingThumbnail.setSource(new juce::FileInputSource(file));
                        recordingThumbnail.setDisplayThumbnailMode(0);// request waveform to fill viewing zone
                        levelSlider.setValue(1.0); //Reset Volume Slider to 1 upon file change
                        //readerSource.reset(newSource.release());
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

        recorder.startRecording(lastRecording);

        recordButton.setButtonText("Stop");
        xZoomSlider.setValue(0.0f);
        recordingThumbnail.setDisplayXZoom(0); //beware this function resets displayFullThumb
        //recordingThumbnail.setDisplayFullThumbnail(true);
        recordingThumbnail.setDisplayThumbnailMode(1); // in record mode scrolling display
    }
//-------------------------------------------------------------------------------------
    void stopRecording()
    {
        recorder.stop();

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
        //recordingThumbnail.setDisplayFullThumbnail(true);
        recordingThumbnail.setDisplayThumbnailMode(0);// request waveform to fill viewing zone
        levelSlider.setValue(1.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
