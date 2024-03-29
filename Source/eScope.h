#pragma once

#include <vector>
#include <algorithm>
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"

namespace juce
{
    //=====================================================================================
    
    class  EScope : public Component, public juce::ChangeBroadcaster
    {
    public:
        EScope()
        {

            addAndMakeVisible(&recThumbnail,0);

        }
        ~EScope()
        {

        }
        juce::RecordingThumbnail recThumbnail;
        juce::AudioRecorder rec{ recThumbnail.getAudioThumbnail() };


        void EScope::paint(juce::Graphics& g) override
        {
            recThumbnail.paint(g);
        }

        void EScope::resized()
        {
            auto area = getLocalBounds();
            recThumbnail.setBounds(area); //this triggers a recThumbnail.resized();
        }
  
        AudioIODeviceCallback* getAudioIODeviceCallBack() { return &rec; }

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
            recThumbnail.chanID = chanID;
        }
        void setViewSize(float dispTime)// sets viewing window size in secondes in oscillo mode
        {
            recThumbnail.setViewSize(dispTime);
        }
        void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) //override
        {
            sendChangeMessage();
            recThumbnail.mouseWheelMove(event, wheel);
        }
        void mouseDown(const MouseEvent& event)
        {
            recThumbnail.mouseDown(event);
        }
        void setVisibleRange(Range<double> newRange)
        {
            recThumbnail.setRange(newRange);
        }
        void setXZoom(double zoomfactor)
        {
            recThumbnail.setDisplayXZone(zoomfactor);
        }
    private:


    };
    //==============================================================================*/
};
