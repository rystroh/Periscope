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
        juce::AudioRecorder recorder{ recThumbnail.getAudioThumbnail() };


        void EScope::paint(juce::Graphics& g) override
        {
            recThumbnail.paint(g);
        }

        void EScope::resized()
        {
            auto area = getLocalBounds();
            recThumbnail.setBounds(area); //this triggers a recThumbnail.resized();
        }
  
        AudioIODeviceCallback* getAudioIODeviceCallBack() { return &recorder; }

        void startRecording(const File& file) { recorder.startRecording(file); }
        bool isRecording() { return recorder.isRecording(); }
        void audioDeviceAboutToStart(AudioIODevice* device)//needs refactorisation
        {
            auto smpRate = device->getCurrentSampleRate();
            //rec.audioDeviceAboutToStart(device);
            recorder.setSampleRate(smpRate);
            recThumbnail.setSampleRate(smpRate);
           
        }


        void setDisplayThumbnailMode(int displayMode)
        {
            recThumbnail.setDisplayThumbnailMode(displayMode);
            recThumbnail.repaint();
        }
        bool setSource(InputSource* newSource) { return(recThumbnail.setSource(newSource)); }

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate)
        {
            recorder.prepareToPlay(samplesPerBlockExpected, sampleRate);
            recThumbnail.prepareToPlay(samplesPerBlockExpected, sampleRate);
        }

        void setSampleRate(double smpRate) //called by prepareToPlay
        {
            recorder.setSampleRate(smpRate);
            recThumbnail.setSampleRate(smpRate); 
        }
        void setBufferSize(long bufferSize) //called by prepareToPlay
        {
            //recorder.setBufferSize(smpRate);
            //recThumbnail.setBufferSize(smpRate);
        }
        void setDisplayYZoom(double yZoom) { recThumbnail.setDisplayYZoom(yZoom); }
 

        int getChannelID(void)
        {
            return(recorder.getChannelID());
        }

        void setChannelID(int chanID)
        {
            recorder.setChannelID(chanID);
            recThumbnail.chanID = chanID;
            bool* ptr = recThumbnail.getTriggeredPtr();
            recorder.setTriggerPtr(ptr);
        }

        void setViewSize(float dispTime)// sets viewing window size in secondes in oscillo mode
        {
            recorder.setViewSize(dispTime);
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
        void setThreshold(double threshold)
        {
            recorder.setThreshold(threshold);
            recThumbnail.setThreshold(threshold);
        }
    private:


    };
    //==============================================================================*/
};
