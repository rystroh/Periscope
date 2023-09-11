#pragma once

#include <vector>
#include <algorithm>
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"
#define option 1
namespace juce
{
    //=====================================================================================
    
    class  EScope : public Component
    {
    public:
        EScope()
        {
            //addAndMakeVisible(&recThumbnail,0);
        }
        ~EScope()
        {

        }
        juce::RecordingThumbnail recThumbnail;
        juce::AudioRecorder rec{ recThumbnail.getAudioThumbnail() };

#if option == 2   
        void EScope::paint(juce::Graphics& g) override
        {
            /*
            g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
            g.setColour(juce::Colours::grey);
            g.drawRect(getLocalBounds(), 1);
            */
            recThumbnail.paint(g);
        }

        void EScope::resized()
        {
            auto area = getLocalBounds();
            recThumbnail.setBounds(area); //this triggers a recThumbnail.resized();
        }
#endif        
#if option == 1
        void setBounds(int x, int y, int width, int height)
        {
            recThumbnail.setBounds(x, y, width, height);
        }
#endif  
        AudioIODeviceCallback* getAudioIODeviceCallBack() { return &rec; }
        /*
        void EScope::setVisible(bool  	shouldBeVisible)
        {
            recThumbnail.setVisible(shouldBeVisible);
        }*/

        void startRecording(const File& file) { rec.startRecording(file); }
        bool isRecording() { return rec.isRecording(); }
        void audioDeviceAboutToStart(AudioIODevice* device)//needs refactorisation
        {
            auto smpRate = device->getCurrentSampleRate();
            //rec.audioDeviceAboutToStart(device);
            rec.setSampleRate(smpRate);
            recThumbnail.setSampleRate(smpRate);
           
        }


        void setDisplayThumbnailMode(int displayMode)
        {
            recThumbnail.setDisplayThumbnailMode(displayMode);
            recThumbnail.repaint();
        }
        bool setSource(InputSource* newSource) { return(recThumbnail.setSource(newSource)); }        
        void setSampleRate(double smpRate) 
        {
            rec.setSampleRate(smpRate);
            recThumbnail.setSampleRate(smpRate); 
        }
        void setDisplayYZoom(double yZoom) { recThumbnail.setDisplayYZoom(yZoom); }
 

        int getChannelID(void)
        {
            return(rec.getChannelID());
        }

        void setChannelID(int chanID)
        {
            rec.setChannelID(chanID);
        }
       
    private:


    };
    //==============================================================================*/
};
