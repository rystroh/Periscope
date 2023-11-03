#pragma once
#include "20Hz_Bleep.h";
namespace juce
{
#define audio_source 1
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
        }
        //----------------------------------------------------------------------------------
        void prepareToPlay(int smpPerBlockExpected, double smpRate)
        {
            sampleRate = smpRate;
            samplesPerBlockExpected = smpPerBlockExpected;

            eScopeBufferSize = (int) (smpRate * 2.0); //2 seconds
            double divider;
            int    remainer;
            divider = (double) eScopeBufferSize / (double) smpPerBlockExpected;
            remainer = (int)eScopeBufferSize % (int)smpPerBlockExpected;
            eScopeBuffer.setSize(1, (int)eScopeBufferSize);
            writePosition = 0;
            wavaddr = 0;
            wavidx = 0;
            wavptr = &Bleep_20Hz[0]; 
        }
        //----------------------------------------------------------------------------------
        void setViewSize(float dispTime)
        {
            thumbnailSize = dispTime * sampleRate;
            maxSmpCount = thumbnailSize;
            halfMaxSmpCount = (int)maxSmpCount / 2;
            float remain = (int)maxSmpCount % 2;
            if (remain > 0)
            {
                DBG("Odd MacSmpCount");
            }
        }
        //----------------------------------------------------------------------------------
        void setSampleRate(double smpRate) 
        {
            sampleRate = smpRate;
        }
        //----------------------------------------------------------------------------------
        double getSampleRate(void)
        {
            return(sampleRate);
        }
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
        void audioDeviceStopped() override
        {
            sampleRate = 0;
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
                wavaddr=0;
                //
                // Create an AudioBuffer to wrap our incoming data, note that this does no 
                //allocations or copies, it simply references our input data
#if audio_source == 1
                if(wavidx<=48000)
                    { 
                    //AudioBuffer<float> buffer(const_cast<float**>(&wavptr), 1, numSamples);// one stream per buffer
                    //AudioBuffer<float> buffer(wavptr, 1, numSamples);// one stream per buffer
                    auto* channelData = wavptr;
                    wavptr += numSamples;
                    wavidx += numSamples;
                    }
              

#else               
                AudioBuffer<float> buffer(const_cast<float**> (&inputChannelData[chanID]), 1, numSamples);// one stream per buffer
                auto* channelData = buffer.getWritePointer(0);
#endif

                
                    
                
                
                if (writePosition + numSamples > eScopeBufferSize)//need to wrap
                {
                    int nbOfSmpPossibleToCopy = eScopeBufferSize - writePosition;
                    int remaning = numSamples - nbOfSmpPossibleToCopy;
                    eScopeBuffer.copyFrom(0, writePosition, channelData, nbOfSmpPossibleToCopy);
                    eScopeBuffer.copyFrom(0, 0, channelData + nbOfSmpPossibleToCopy, numSamples - nbOfSmpPossibleToCopy);
                }
                else
                {
                    eScopeBuffer.copyFrom(0, writePosition, channelData, numSamples);
                }
                if (currentSmpCount == 0)
                {
                    // check trigger condition in block of samples
                    double min = 1, max = -1;
                    double smpValue;
                    bool bTriggered;
                    if (*thumbnailTriggeredPtr == false)
                    {
                        for (int idx = 0; idx < numSamples; idx++)
                        {
                            smpValue = buffer.getSample(0, idx);
                            if (smpValue < min)
                                min = smpValue;
                            if (smpValue > max)
                                max = smpValue;
                            if (smpValue > thresholdTrigger)
                            {
                                *thumbnailTriggeredPtr = true;
                                triggAddress = writePosition + idx;
                                triggAddress %= eScopeBufferSize; //wrap if needed
                                currentSmpCount = idx;
                                idx = numSamples; //exit for 
                            }

                        }
                    }
                }
                else if(currentSmpCount > 0) //if triggered condition has been met and samples have started to be recorded
                {
                    currentSmpCount += numSamples;//keep count of samples recorded
                } 
            writePosition += numSamples;
            writePosition %= eScopeBufferSize;
            //now if we have enough sample, pass them to the Thumbnail for display
            if (currentSmpCount >= halfMaxSmpCount)
            {
                thumbnail.reset(1, sampleRate, 0);
                //copy data that are before the Threshold
                if (triggAddress >= halfMaxSmpCount) //head data not wrapped 
                { 
                    int offsetinescopebuffer = triggAddress - halfMaxSmpCount;
                    int smpCount;
                    if (triggAddress + halfMaxSmpCount <= eScopeBufferSize)//tail data not wrapped
                    {
                        smpCount = triggAddress + halfMaxSmpCount;
                        thumbnail.addBlock(offsetinescopebuffer, eScopeBuffer, triggAddress, maxSmpCount);
                    }
                    else //tail data wrapped 
                    {
                        smpCount = eScopeBufferSize - triggAddress + halfMaxSmpCount;
                        thumbnail.addBlock(offsetinescopebuffer, eScopeBuffer, triggAddress, smpCount);

                        smpCount = maxSmpCount - smpCount;
                        thumbnail.addBlock(offsetinescopebuffer, eScopeBuffer, triggAddress, smpCount);
                    }
                }
                else
                { 
                
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
            thresholdTrigger = threshold;
        }
        //----------------------------------------------------------------------------------
        void setTriggerPtr(bool* ptr)  { thumbnailTriggeredPtr = ptr;}        
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
        bool* thumbnailTriggeredPtr;
        juce::AudioBuffer<float>eScopeBuffer;
        int  eScopeBufferSize;
        int writePosition = 0;
        int readPosition = 0;
        float triggerPlaceRatio = 0.5;
        int currentSmpCount = 0;
        int maxSmpCount = 0;
        int halfMaxSmpCount = 0;

        uint16 wavaddr = 0;
        uint16 wavidx = 0;
        const float *wavptr = nullptr;
    };
};

