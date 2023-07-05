#pragma once
namespace juce
{
    //=====================================================================================
    /** A simple class that acts as an AudioIODeviceCallback
                              and writes the incoming audio data to a WAV file.          */
    class AudioRecorder : public AudioIODeviceCallback
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

            const ScopedLock sl(writerLock);

            if (activeWriter.load() != nullptr && numInputChannels >= thumbnail.getNumChannels())
            {
                activeWriter.load()->write(inputChannelData, numSamples);

                // Create an AudioBuffer to wrap our incoming data, note that this does no 
                //allocations or copies, it simply references our input data
                AudioBuffer<float> buffer(const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
                thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
                nextSampleNum += numSamples;
            }

            // We need to clear the output buffers, in case they're full of junk..
            for (int i = 0; i < numOutputChannels; ++i)
                if (outputChannelData[i] != nullptr)
                    FloatVectorOperations::clear(outputChannelData[i], numSamples);
        }
        //----------------------------------------------------------------------------------
    private:
        AudioThumbnail& thumbnail;
        // the thread that will write our audio data to disk
        TimeSliceThread backgroundThread{ "Audio Recorder Thread" };
        // the FIFO used to buffer the incoming data
        std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedWriter;
        double sampleRate = 0.0;
        int chanNb = 1;
        int bitDepth = 24;
        int64 nextSampleNum = 0;
        CriticalSection writerLock;
        std::atomic<AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };
    };
};

