#pragma once
//#include "20Hz_Bleep.h";
//#include "RampPos48000.h"
//#include "Ramp48000Skipped2.h"
//#include "Ramp48000Skipped3.h"
#include "Ramp120k.h"
namespace juce
{
#define audio_source 1
//#define modify_triggers 1
//=====================================================================================
    /** A simple class that acts as an AudioIODeviceCallback
                              and writes the incoming audio data to a WAV file.          */
    class AudioRecorder : public AudioIODeviceCallback,
        public juce::ChangeBroadcaster
    {
    public:
        AudioRecorder(AudioThumbnail& thumbnailToUpdate)
            : thumbnail(thumbnailToUpdate)
        {
            backgroundThread.startThread();
        }

        ~AudioRecorder() override
        {
            stop();
        }
        
        //---------------------------------------------------------------------------------
        void startRecording(const File& file)
        {
            stop();

            if (sampleRate > 0)
            {
                // Create an OutputStream to write to our destination file...
                file.deleteFile();

                if (auto fileStream = std::unique_ptr<FileOutputStream>(file.createOutputStream()))
                {
                    // Now create a WAV writer object that writes to our output stream...
                    WavAudioFormat wavFormat;

                    if (auto writer = wavFormat.createWriterFor(fileStream.get(), sampleRate, chanNb, bitDepth, {}, 0))
                    {
                        fileStream.release(); // (passes responsibility for deleting the stream 
                                             // to the writer object that is now using it)

                        // Now we'll create one of these helper objects which will act as a FIFO
                        // buffer, and will write the data to disk on our background thread.
                        threadedWriter.reset(new AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768));

                        // Reset our recording thumbnail
                        thumbnail.reset(writer->getNumChannels(), writer->getSampleRate());
                        nextSampleNum = 0;

                        // And now, swap over our active writer pointer so that the audio 
                        //callback will start using it..
                        const ScopedLock sl(writerLock);
                        activeWriter = threadedWriter.get();
                    }
                }
            }
        }
        //----------------------------------------------------------------------------------
        void stop()
        {
            // 1st, clear this pter to stop the audio callback from using our writer object..
            {
                const ScopedLock sl(writerLock);
                activeWriter = nullptr;
            }
            // Now we can delete the writer object. It's done in this order because the deletion
            // could take a little time while remaining data gets flushed to disk, 
            // so it's best to avoid blocking the audio callback while this happens.
            threadedWriter.reset();
        }
        //----------------------------------------------------------------------------------
        bool isRecording() const
        {  
            return activeWriter.load() != nullptr;
        }
        //----------------------------------------------------------------------------------
        void audioDeviceAboutToStart(AudioIODevice* device) override
        {
            sampleRate = device->getCurrentSampleRate();
            samplesPerBlockExpected = device->getCurrentBufferSizeSamples();
        }
        //----------------------------------------------------------------------------------
        void prepareToPlay(int smpPerBlockExpected, double smpRate)
        {
            sampleRate = smpRate;
            samplesPerBlockExpected = smpPerBlockExpected;

            eScopeBufferSize = maxSmpCount;// (int)(smpRate * 2.0); //2 seconds

            double divider;
            int    remainer;
            divider = (double) eScopeBufferSize / (double) smpPerBlockExpected;
            remainer = (int)eScopeBufferSize % (int)smpPerBlockExpected;
            eScopeBuffer.setSize(1, (int)eScopeBufferSize);
            eScopeBuffer.clear();
            writePosition = 0;
            wavaddr = 0;
            wavidx = 0;    
            //wavptr = &Bleep_20Hz[0];
            //wavptr = &RampPos48000[0];
            //wavptr = &Ramp48000Skipped2[0];
            //wavptr = &Ramp48000Skipped3[0];
            wavptr = &Ramp120k[0];
            thumbnailWritten = false;
        }
        //----------------------------------------------------------------------------------
        void setViewSize(float dispTime)
        {
            thumbnailSize = dispTime * sampleRate;
            double flBlockNb = (double)thumbnailSize / (double)samplesPerBlockExpected + 0.5;
            int smpBlockNb = (int)flBlockNb;
            maxSmpCount = smpBlockNb * samplesPerBlockExpected; //make it multiple of block Size
            halfMaxSmpCount = (int)maxSmpCount / 2;
            thumbnailSize = maxSmpCount;
            
            float remain = (int)maxSmpCount % 2;
            if (remain > 0)
            {
                DBG("Odd MacSmpCount");
            }
        }
        //----------------------------------------------------------------------------------
        juce::AudioBuffer<float>* getBufferPtr() {  return (&eScopeBuffer);  }
        //----------------------------------------------------------------------------------
        unsigned long* getStartAddrPtr() { return (&wfStartAddress); }
        //----------------------------------------------------------------------------------
        unsigned long* getTriggAddrPtr() { return (&wfTriggAddress); }
        //----------------------------------------------------------------------------------
        void setSampleRate(double smpRate) { sampleRate = smpRate; }
        //----------------------------------------------------------------------------------
        double getSampleRate(void) { return(sampleRate); }
        //----------------------------------------------------------------------------------
        bool setSampleDepth(int depth)
        {
            int possibleDepth[] = { 8, 16, 24 };
            int n = sizeof(possibleDepth) / sizeof(*possibleDepth);
            bool exists = std::find(possibleDepth, possibleDepth + n, depth) != possibleDepth
                + n;
            if (exists) {
                bitDepth = depth;
            }
            return(exists);
        }
        //----------------------------------------------------------------------------------
        bool setSampleChanNb(int chNb)
        {
            if ((chNb == 1) || (chNb == 2))
            {
                chanNb = chNb;
                return(true);
            }
            else
                return(false);
        }
        //----------------------------------------------------------------------------------
        void audioDeviceStopped() override  { sampleRate = 0; }
        //----------------------------------------------------------------------------------
        bool checkForLevelTrigger(int nbSamples, unsigned int* trigIndex, AudioBuffer<float> *buffer)
        {
            // check trigger condition in block of samples
            bool triggerConditionFound = false;
            double min = 1, max = -1;
            double smpValue;
            int idx = 0;
            while((idx < nbSamples) && !triggerConditionFound)
            {
                    smpValue = buffer->getSample(0, idx);
                    if (smpValue < min)
                        min = smpValue;
                    if (smpValue > max)
                        max = smpValue;
                    if (smpValue > thresholdTrigger)
                    {
                        *trigIndex = idx;
                        triggerConditionFound = true;                        
                    }
                    idx++;
            }
            return (triggerConditionFound);
        }
        //----------------------------------------------------------------------------------
        void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
            int numInputChannels,
            float* const* outputChannelData,
            int numOutputChannels,
            int numSamples,
            const AudioIODeviceCallbackContext& context) override
        {
            ignoreUnused(context);
            //sendSynchronousChangeMessage(); https://docs.juce.com/master/classMessageManagerLock.html#details
            const ScopedLock sl(writerLock);
/*
            if (activeWriter.load() != nullptr && numInputChannels >= thumbnail.getNumChannels())
            {
                activeWriter.load()->write(inputChannelData, numSamples);
                // Create an AudioBuffer to wrap our incoming data, note that this does no 
                //allocations or copies, it simply references our input data
                AudioBuffer<float> buffer(const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
                thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
                nextSampleNum += numSamples;
            }*/
            if (activeWriter.load() == nullptr && chanID < numInputChannels && eScopeBufferSize>0)
            {
                //activeWriter.load()->write(&inputChannelData[chanID], numSamples);
                //
                // Create an AudioBuffer to wrap our incoming data, note that this does no 
                //allocations or copies, it simply references our input data
                AudioBuffer<float> buffer(const_cast<float**> (&inputChannelData[chanID]), 1, numSamples);// one stream per buffer
                auto* channelData = buffer.getWritePointer(0);

#if audio_source == 1 //overwrite stream with test wav file
                overwriteStreamWithTestWav(channelData, buffer.getNumSamples());
#endif
                // write in circulare buffer for later display
                if (writePosition + numSamples > eScopeBufferSize)//need to wrap condition
                {
                    int nbOfSmpPossibleToCopy = eScopeBufferSize - writePosition;
                    int remaning = numSamples - nbOfSmpPossibleToCopy;
                    eScopeBuffer.copyFrom(0, writePosition, channelData, nbOfSmpPossibleToCopy);
                    eScopeBuffer.copyFrom(0, 0, channelData + nbOfSmpPossibleToCopy, numSamples - nbOfSmpPossibleToCopy);
                }
                else  //no need to wrap : copy to circular buffer
                {
                    eScopeBuffer.copyFrom(0, writePosition, channelData, numSamples);
                }

             // check if any sample in this new block is above threshold -> triggering display
                if (currentSmpCount == 0)
                {
                    unsigned int triggerIndex;
                    if ((*thumbnailTriggeredPtr == false) && (thumbnailWritten == false))
                    {
                        bool bTriggered = checkForLevelTrigger(numSamples, &triggerIndex, &buffer);
                        *thumbnailTriggeredPtr = bTriggered;
                        if (bTriggered)
                        {
                            triggAddress = writePosition + triggerIndex;
                            triggAddress %= eScopeBufferSize; //wrap if needed
                            currentSmpCount = numSamples - triggerIndex;//nb of samples recorded after trigger condition
                        }
                    }
                }
                else if(currentSmpCount > 0) //if triggered condition has been met and samples have started to be recorded
                {
                    currentSmpCount += numSamples;//keep count of samples recorded
                } 
            writePosition += numSamples;
            writePosition %= eScopeBufferSize;

            //now if we have enough samples, pass them to the Thumbnail for display
            if ((currentSmpCount >= halfMaxSmpCount) && (thumbnailWritten == false))
            {
                thumbnailWritten = true;
                thumbnail.reset(1, sampleRate, 0);
                //copy data that are before the Threshold
                if (triggAddress >= halfMaxSmpCount) //head data not wrapped 
                { 
                    int offsetInEScopeBuffer = triggAddress - halfMaxSmpCount;
                    int smpCount;
                    if (triggAddress + halfMaxSmpCount <= eScopeBufferSize)//tail data not wrapped
                    {
                        smpCount = triggAddress + halfMaxSmpCount;
                        //thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, maxSmpCount);
                        thumbnail.addBlock(0, eScopeBuffer, offsetInEScopeBuffer, maxSmpCount);
                        eScopeBufferSize = 0; // reset flag for tests
                    }
                    else //tail data wrapped 
                    {
                        smpCount = eScopeBufferSize - triggAddress + halfMaxSmpCount;
                        thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, smpCount);

                        smpCount = maxSmpCount - smpCount;
                        thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, smpCount);
                    }
                }
                else //head data wrapped or not enough data recorded before trigger point
                {
                    int offsetInEScopeBuffer = 0;
                    int smpCount;
                    if (triggAddress + halfMaxSmpCount <= eScopeBufferSize)//tail data not wrapped
                    {
                        smpCount = triggAddress + halfMaxSmpCount;
                        int paddingSmpNb = halfMaxSmpCount - triggAddress;
                        int paddingPtrinBuffer = eScopeBufferSize - paddingSmpNb;
                        //thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, maxSmpCount);
                        thumbnail.addBlock(0, eScopeBuffer, paddingPtrinBuffer, paddingSmpNb);
                        thumbnail.addBlock(paddingSmpNb, eScopeBuffer, offsetInEScopeBuffer, smpCount);
                        eScopeBufferSize = 0; // reset flag for tests
                    }
                    else //tail data wrapped 
                    {
                        smpCount = eScopeBufferSize - triggAddress + halfMaxSmpCount;
                        thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, smpCount);

                        smpCount = maxSmpCount - smpCount;
                        thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, smpCount);
                    }
                
                }
                //copy data aftert the Threshold
                if(eScopeBufferSize-triggAddress >= halfMaxSmpCount)//data not wrapped ?
                {
                    thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
                }
                else
                {

                }
                currentSmpCount = 0;
            }
 /*         if (*thumbnailTriggeredPtr)
            {
                thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
                nextSampleNum += numSamples;
            }*/

            }

            if (activeWriter.load() != nullptr && chanID < numInputChannels)
            {             
                activeWriter.load()->write(&inputChannelData[chanID], numSamples);
                // Create an AudioBuffer to wrap our incoming data, note that this does no 
                //allocations or copies, it simply references our input data
                AudioBuffer<float> buffer(const_cast<float**> (&inputChannelData[chanID]), 1, numSamples);// one stream per buffer
                thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
                nextSampleNum += numSamples;
            }

            // We need to clear the output buffers, in case they're full of junk..
            for (int i = 0; i < numOutputChannels; ++i)
                if (outputChannelData[i] != nullptr)
                    FloatVectorOperations::clear(outputChannelData[i], numSamples);
        }
        //----------------------------------------------------------------------------------
        int getChannelID(void)
        {
            return(chanID);
        }
        //----------------------------------------------------------------------------------
        void setChannelID(int setChanID)
        {
            chanID = setChanID;
        }
        //----------------------------------------------------------------------------------
        void setThreshold(double threshold)
        {
#if modify_triggers == 1
            switch ((int)(threshold*100))
            {
            case 0:
                thresholdTrigger = 0.010;   // addr =    530
                break;
            case 1:
                thresholdTrigger = 0.011;   // addr =  2 829
                break;
            case 2:
                thresholdTrigger = 0.017;   // addr =  5 303
                break;
            case 3:
                thresholdTrigger = 0.037;   // addr = 10 141
                break;
            case 4:
                thresholdTrigger = 0.800;   // addr = 19 556
                break;
            default:
                thresholdTrigger = threshold;
            }
#else
            thresholdTrigger = threshold;
#endif
        }
        //----------------------------------------------------------------------------------
        void setTriggerPtr(bool* ptr)  { thumbnailTriggeredPtr = ptr;}
        //----------------------------------------------------------------------------------
        //----------------------------------------------------------------------------------
#if audio_source == 1 //for debug and tests only
        void overwriteStreamWithTestWav(float* chanDataptr, int numSmp)
        {
            int sample = 0;
            while((wavidx <= BleepSize) && (sample < numSmp)) //only copy at the beginning of the stream (the size of the array) 
            {
                    *chanDataptr = *wavptr;
                    chanDataptr++;
                    wavptr++;
                    sample++;
                    wavidx++; //to check that total count doesn't exceed wav size 
            }
        }
#endif
        //----------------------------------------------------------------------------------
    private:
        AudioThumbnail& thumbnail;
        int thumbnailSize = 0;

        // the thread that will write our audio data to disk
        TimeSliceThread backgroundThread{ "Audio Recorder Thread" };
        // the FIFO used to buffer the incoming data
        std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedWriter;
        double sampleRate = 0.0;
        int samplesPerBlockExpected = 0;
        int triggAddress = 0;

        int chanNb = 1;
        int bitDepth = 24;
        int64 nextSampleNum = 0;
        CriticalSection writerLock;
        std::atomic<AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };

        int chanID = 0; // default channel selected in multi channel audio interface is first one
        double thresholdTrigger = 1.0;
        bool* thumbnailTriggeredPtr; // <- declare ptr to flag accessible by display part
        juce::AudioBuffer<float>eScopeBuffer;
        int  eScopeBufferSize;
        int writePosition = 0;
        int readPosition = 0;
        float triggerPlaceRatio = 0.5;
        int currentSmpCount = 0;
        int maxSmpCount = 0;
        int halfMaxSmpCount = 0;

        unsigned long wavaddr = 0;
        unsigned long wavidx = 0;

        unsigned long wfStartAddress = 0;
        unsigned long wfTriggAddress = 0;

        const float *wavptr = nullptr;
        uint16 wavSize = 48000;
        bool thumbnailWritten = false;
    };
};

