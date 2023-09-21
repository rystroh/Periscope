#pragma once

#include <JuceHeader.h>
#include <sstream>
#include <string>
#include "eScope.h"
#include "ListenerComponent.h"
const int eScopeChanNb = 8;
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, public juce::ChangeListener
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
    void changeListenerCallback(juce::ChangeBroadcaster* source);

private:
    //==============================================================================
    // Your private member variables go here...
    juce::TextButton recordButton{ "Record" };
    juce::TextButton openButton{ "Open File" };
    juce::ComboBox menu;
    juce::Slider oscWinSizeSlider;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;                    // [3]    
    
    juce::File lastRecording[eScopeChanNb];
    ListenerComponent listenerComponent;
    juce::EScope eScope[eScopeChanNb];
    int recmode; // can be 1= track view or 2= oscilloscope
    double oscilloWinSize = 0.05;
 //-------------------------------------------------------------------------------------   
    juce::AudioDeviceManager& getAudioDeviceManager() //getting access to the built in AudioDeviceManager
    {
        return deviceManager; 
    }
 //-------------------------------------------------------------------------------------
    void openButtonClicked()
    {/*
        chooser = std::make_unique<juce::FileChooser>("Select a Wave file...",
                                                       juce::File{},"*.wav");*/
        chooser = std::make_unique<juce::FileChooser>("Select a Wave List...",
            juce::File{}, "*.txt");
        auto chooserFlags = juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();

                if (file != juce::File{})
                {
                    //std::ifstream wavlist(file);
                    //fopen(file, "r");
                    juce::FileInputStream inputStream(file);

                    int idx = 0;

                    while (!inputStream.isExhausted()) // [3]
                    {
                        auto line = inputStream.readNextLine();
                        auto* reader = formatManager.createReaderFor(line);

                        if (reader != nullptr)
                        {
                            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                            eScope[idx].setSource(new juce::FileInputSource(line));
                            eScope[idx].setSampleRate(reader->sampleRate);
                            eScope[idx].setDisplayThumbnailMode(0);// request waveform to fill viewing zone
                            eScope[idx].setDisplayYZoom(1.0);
                            eScope[idx].resized();
                            idx++;
                        }
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
        auto& devManager = MainComponent::getAudioDeviceManager();
        auto device = devManager.getCurrentAudioDevice();
        auto smpRate = device->getCurrentSampleRate();
        //eScope.recThumbnail.setSampleRate(eScope.rec.getSampleRate()); //needs refactoring
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx].setSampleRate(smpRate);
            lastRecording[idx] = parentDir.getNonexistentChildFile("eScope Recording", ".wav");
            eScope[idx].startRecording(lastRecording[idx]);
            eScope[idx].setDisplayThumbnailMode(recmode);
        }
        recordButton.setButtonText("Stop");
    }
//-------------------------------------------------------------------------------------
    void stopRecording()
    {
        for (int idx = 0; idx < eScopeChanNb; idx++)
        {
            eScope[idx].rec.stop();
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
            eScope[idx].setDisplayThumbnailMode(0);// request waveform to fill viewing zone
            eScope[idx].setDisplayYZoom(1.0);
        }
        recordButton.setButtonText("Record");
        
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
