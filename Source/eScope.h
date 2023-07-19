#pragma once

#include <vector>
#include <algorithm>
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"
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

        
        void EScope::paint(juce::Graphics& g) override
        {
            g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
            g.setColour(juce::Colours::grey);
            g.drawRect(getLocalBounds(), 1);
        }

        void EScope::resized()
        {
            recThumbnail.setBounds(10, 10, 100, 30);
        }
        
        void EScope::addAndMakeVisible(Component& child, int zOrder)
        {
            child.setVisible(true);
            addChildComponent(child, zOrder);
        }/*
        void EScope::addAndMakeVisible() 
        {
            addAndMakeVisible();// recThumbnail);
        }
        */

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
        }
        bool setSource(InputSource* newSource) { return(recThumbnail.setSource(newSource)); }        
        void setSampleRate(double smpRate) 
        {
            rec.setSampleRate(smpRate);
            recThumbnail.setSampleRate(smpRate); 
        }
        void setDisplayYZoom(double yZoom) { recThumbnail.setDisplayYZoom(yZoom); }
        void setBounds(int x, int y, int width, int height)
        {
            recThumbnail.setBounds(x, y, width, height);
        }



    private:


    };
    //==============================================================================*/
};
