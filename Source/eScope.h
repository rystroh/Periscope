#pragma once

#include <vector>
#include <algorithm>
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"
#include "..\GRAPE\Source\GRAPE.h"

//namespace juce
//{
    //=====================================================================================
    
    class  EScope : public grape::Panel, public juce::ChangeBroadcaster
    {
    public:
        EScope(const juce::String& id) : Panel(id)
        {
            addAndMakeVisible(&recThumbnail,0);
            setWidth(300, 600, 10000);
            setHeight(50, 100, 500);
        }
        ~EScope() {        }
        RecordingThumbnail recThumbnail;
        juce::AudioRecorder recorder{ recThumbnail.getAudioThumbnail() };

        void EScope::paint(juce::Graphics& g) override { recThumbnail.paint(g); }
        void EScope::resized()
        {
            auto area = getLocalBounds();
            recThumbnail.setBounds(area); //this triggers a recThumbnail.resized();
        }  
        juce::AudioIODeviceCallback* getAudioIODeviceCallBack() { return &recorder; }
        void startRecording(const juce::File& file) { recorder.startRecording(file); }
        bool isRecording() { return recorder.isRecording(); }
        void audioDeviceAboutToStart(juce::AudioIODevice* device)//needs refactorisation
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
        bool setSource(juce::InputSource* newSource) { return(recThumbnail.setSource(newSource)); }
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
        double getDisplayYZoom() { recThumbnail.getDisplayYZoom(); }
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
        //----------------------------------------------------------------------------------
        void setViewSize(float dispTime)// sets viewing window size in secondes in oscillo mode
        {
            recorder.setViewSize(dispTime);            
            recThumbnail.setViewSize(dispTime);
            // update pointers to the buffer and pointers used for display outside of Thumbnail
            juce::AudioBuffer<float>* recBuffer = recorder.getBufferPtr();
            recThumbnail.setBufferedToImage(recBuffer);
            unsigned long* StartAddr = recorder.getStartAddrPtr();
            recThumbnail.setBufferStartAddress(StartAddr);
            unsigned long* TriggAddr = recorder.getTriggAddrPtr();
            recThumbnail.setBufferTriggAddress(TriggAddr);
            bool* BufferReady = recorder.getBufferReadyAddrPtr();
            recThumbnail.setBufferReadyAddress(BufferReady);
        }
        //----------------------------------------------------------------------------------
        void setYScale(int scale) { recThumbnail.setYScale(scale); }
        //----------------------------------------------------------------------------------
        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) //override
        {
            sendChangeMessage();
            recThumbnail.mouseWheelMove(event, wheel);
        }
        void mouseDown(const juce::MouseEvent& event)
        {
            recThumbnail.mouseDown(event);
        }
        void setVisibleRange(juce::Range<double> newRange) { recThumbnail.setRange(newRange); }
        void setXZoom(double zoomfactor) { recThumbnail.setDisplayXZoom(zoomfactor); }
        void setThreshold(double threshold)
        {
            recorder.setThreshold(threshold);
            recThumbnail.setThreshold(threshold);
        }     
        int getZoomGroup() { return(recThumbnail.getZoomGroup()); }
        int getXZoomFlag() { return(recThumbnail.getXZoomFlag()); }
        int getYZoomFlag() { return(recThumbnail.getYZoomFlag()); }
    private:


    };
    //==============================================================================*/
//};
