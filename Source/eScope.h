#pragma once

#include <vector>
#include <algorithm>
#include "AudioRecorder.h"
#include "RecordingThumbnail.h"
namespace juce
{
    //=====================================================================================
    
    class  EScope 
    {
    public:
        EScope()
        {

        }
        ~EScope()
        {

        }
        juce::RecordingThumbnail recThumbnail;
        juce::AudioRecorder rec{ recThumbnail.getAudioThumbnail() };
    private:



    };
    //==============================================================================*/
};
