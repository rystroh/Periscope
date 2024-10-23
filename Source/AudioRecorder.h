#pragma once
#include "Enums.h"
//#include "20Hz_Bleep.h";
//#include "RampPos48000.h"
//#include "Ramp48000Skipped2.h"
//#include "Ramp48000Skipped3.h"
#include "Ramp120k.h"
//#include "Ramp120kUpDown.h"
//#include "Ramp_bleep120k.h"
#include "..\GRAPE\Source\GRAPE.h"

/*------- DEBUG defines --------*/
#define DEBUG_BUFFER = 1  //debug output functions
#define DEBUG_BUFFER_0 = 1 //level 0 of buffer debug 
//#define DEBUG_BUFFER_1 = 1 //level 1 of buffer debug 
//=====================================================================================
    /** A simple class that acts as an AudioIODeviceCallback
                              and writes the incoming audio data to a WAV file.          */
class AudioRecorder : public juce::AudioIODeviceCallback,
    public juce::ChangeBroadcaster
{
public:
    AudioRecorder(/*juce::AudioThumbnail& thumbnailToUpdate, */grape::Panel* trigger_settings)
    {
        triggerSettings = trigger_settings;
        //: thumbnail(thumbnailToUpdate)
        backgroundThread.startThread();
    }

    ~AudioRecorder() override
    {
        stop();
    }
    //---------------------------------------------------------------------------------
    void AttachThumbnail(juce::AudioThumbnail** ptr, int channelNumber) // thumbnail To Update)
    {
        for (int idx = 0; idx < channelNumber; idx++)
        {
            thmbNail[idx] = *ptr++;
            //addChangeListener(thmbNail[idx](Listener));
            //addChangeListener(juce::ChangeListenerthmbNail[idx]));
        }
    }
    //---------------------------------------------------------------------------------
    void AttachListener(juce::ChangeListener& ptr, int channelNumber) // thumbnail To Update)
    {
        /*  juce::ChangeListener& recThmb;
          for (int idx = 0; idx < channelNumber; idx++)
          {
              recThmb = *ptr++;
              //addChangeListener(thmbNail[idx](Listener));
              addChangeListener(recThmb);
          }*/
    }
    //---------------------------------------------------------------------------------
    void startRecording(const juce::File& file)
    {
        stop();

        if (sampleRate > 0)
        {
            // Create an OutputStream to write to our destination file...
            file.deleteFile();

            if (auto fileStream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream()))
            {
                // Now create a WAV writer object that writes to our output stream...
                juce::WavAudioFormat wavFormat;

                if (auto writer = wavFormat.createWriterFor(fileStream.get(), sampleRate, chanNb, bitDepth, {}, 0))
                {
                    fileStream.release(); // (passes responsibility for deleting the stream 
                                         // to the writer object that is now using it)

                    // Now we'll create one of these helper objects which will act as a FIFO
                    // buffer, and will write the data to disk on our background thread.
                    threadedWriter.reset(new juce::AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768));

                    // Reset our recording thumbnail
                    //thumbnail.reset(writer->getNumChannels(), writer->getSampleRate());
                    thmbNail[0]->reset(writer->getNumChannels(), writer->getSampleRate());
                    nextSampleNum = 0;

                    // And now, swap over our active writer pointer so that the audio 
                    //callback will start using it..
                    const juce::ScopedLock sl(writerLock);
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
            const juce::ScopedLock sl(writerLock);
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
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        sampleRate = device->getCurrentSampleRate();
        samplesPerBlockExpected = device->getCurrentBufferSizeSamples();
    }
    //----------------------------------------------------------------------------------
    void prepareToPlay(int smpPerBlockExpected, double smpRate)
    {
        int eScopeChanNb = ESCOPE_CHAN_NB;
        sampleRate = smpRate;
        samplesPerBlockExpected = smpPerBlockExpected;

        eScopBufferSize = maxSmpCount;// (int)(smpRate * 2.0); //2 seconds

        double divider;
        int    remainer;
        divider = (double)eScopBufferSize / (double)smpPerBlockExpected;
        remainer = (int)eScopBufferSize % (int)smpPerBlockExpected;
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            eScopeBuffer[idx].setSize(1, (int)eScopBufferSize);
            eScopeBuffer[idx].clear();
        }
        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            writePosition[idx] = 0;
            wavaddr[idx] = 0;
            wavidx[idx] = 0;
            previousSampleValue[idx] = 0;
            //wavptr = &Bleep_20Hz[0];
            //wavptr = &RampPos48000[0];
            //wavptr = &Ramp48000Skipped2[0];
            //wavptr = &Ramp48000Skipped3[0];
            wavptr[idx] = &Ramp120k[0];
            //wavptr[idx] = &Ramp120kUpDown[0];
        }
        thumbnailWritten = false;
        bufferWritten = false;
        wfBufferUnderRun = true;
        absTriggerAddress = 0;
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
    juce::AudioBuffer<float>* getBufferPtr(int trackID) { return (&eScopeBuffer[trackID]); }
    //----------------------------------------------------------------------------------
    unsigned long* getStartAddrPtr() { return (&wfStartAddress); }
    //----------------------------------------------------------------------------------
    unsigned long* getTriggAddrPtr() { return (&wfTriggAddress); }
    //----------------------------------------------------------------------------------
    bool* getBufferReadyAddrPtr() {
        bool* BufferReady;
        BufferReady = &wfBufferReady;//for debug only
        return (&wfBufferReady);
    }
    //----------------------------------------------------------------------------------
    bool* getBufferUndeRunAddrPtr() {
        bool* BufferUnderRun;
        BufferUnderRun = &wfBufferUnderRun;//for debug only
        return (&wfBufferUnderRun);
    }
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
    void audioDeviceStopped() override { sampleRate = 0; }
    //----------------------------------------------------------------------------------
    bool checkForLevelTrigger(int nbSamples, unsigned int* trigIndex, juce::AudioBuffer<float>* buffer, int currentChan)
    {
        // check trigger condition in block of samples
        int RecTrigChannel = triggerSettings->getControlValue("Channel");

        bool triggerConditionFound = false;
        if (currentChan != RecTrigChannel) //screen non triggering channels
            return (triggerConditionFound);

        double smpValue;
        int idx = 0;
        long longidx = writePosition[currentChan];
        longidx = wavaddr[currentChan];
        longidx = wavidx[currentChan]; //longidx = wavidx[currentChan] = 0; // Why cancel ???

        double thresholdTrigger = triggerSettings->getControlValue("Threshold");
        int RecTrigMode = triggerSettings->getControlValue("Condition");
        thresholdTrigger = 0.500; // DEBUG
        if (RecTrigMode == 0) // to be removed once GRAPE ComboBox issue solved
            RecTrigMode = 2;   //workaround to current GRAPE ComboBox issue 

        switch (RecTrigMode)
        {
        case Clipping: //0: clipping detection
            while ((idx < nbSamples) && !triggerConditionFound)
            {
                smpValue = buffer->getSample(0, idx);
                if (((smpValue == previousSampleValue[currentChan])) && (abs(smpValue) >= 0.999))
                {
                    *trigIndex = idx;
                    triggerConditionFound = true;
                    absTriggerAddress = wavidx[currentChan] + idx;
                }
                previousSampleValue[currentChan] = smpValue;
                idx++;
            }
            break;
        case ThresholdRising: //1: rising edge condition
            while ((idx < nbSamples) && !triggerConditionFound)
            {
                smpValue = buffer->getSample(0, idx);
                if ((smpValue >= thresholdTrigger) && (previousSampleValue[currentChan] < thresholdTrigger))
                {
                    *trigIndex = idx;
                    triggerConditionFound = true;
                    absTriggerAddress = wavidx[currentChan]- nbSamples + idx;
#ifdef DEBUG_BUFFER_0
                    DBG("Trigger point found at wavidx = " << wavidx[currentChan] << " index = " << idx);
#endif //DEBUG_BUFFER_0
                    return (triggerConditionFound);
                }
                previousSampleValue[currentChan] = smpValue;
                idx++;
            }
            break;
        case ThresholdFalling: //2: falling edge condition
            while ((idx < nbSamples) && !triggerConditionFound)
            {
                smpValue = buffer->getSample(0, idx);
                if ((smpValue <= thresholdTrigger) && (previousSampleValue[currentChan] > thresholdTrigger))
                {
                    *trigIndex = idx;
                    triggerConditionFound = true;
                    absTriggerAddress = wavidx[currentChan] + idx;
                }
                idx++;
                previousSampleValue[currentChan] = smpValue;
            }
            break;
        case ThresholdRisingOrFalling: //3: rizing or falling edge condition 
            while ((idx < nbSamples) && !triggerConditionFound)
            {
                smpValue = buffer->getSample(0, idx);

                if (((smpValue >= thresholdTrigger) && (previousSampleValue[currentChan] < thresholdTrigger)) || ((smpValue <= thresholdTrigger) && (previousSampleValue[currentChan] > thresholdTrigger)))
                {
                    *trigIndex = idx;
                    triggerConditionFound = true;
                    absTriggerAddress = wavidx[currentChan] + idx;
                }
                idx++;
                previousSampleValue[currentChan] = smpValue;
            }
            break;
        default:
            break;

        }
        return (triggerConditionFound);
    }
    //----------------------------------------------------------------------------------
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override
    {
        ignoreUnused(context);
        //sendSynchronousChangeMessage(); https://docs.juce.com/master/classMessageManagerLock.html#details
        const juce::ScopedLock sl(writerLock);
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
        //---------------- One shot Recording -----------------
        if (activeWriter.load() == nullptr && chanID < numInputChannels && eScopBufferSize>0)
        {
            //activeWriter.load()->write(&inputChannelData[chanID], numSamples);
            //
            // Create an AudioBuffer to wrap our incoming data, note that this does no 
            //allocations or copies, it simply references our input data
            //juce::AudioBuffer<float> buffer(const_cast<float**> (&inputChannelData[chanID]), 1, numSamples);// one stream per buffer
            juce::AudioBuffer<float> bufferz[ESCOPE_CHAN_NB];
            long addrOfLastWavValueWritten;
            int RecTrigChannel = triggerSettings->getControlValue("Channel");
            
            //int idx = 0;
            int eScopChanNb = ESCOPE_CHAN_NB;
            for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
            {
                bufferz[idx].setDataToReferTo(const_cast<float**> (&inputChannelData[idx]), 1, numSamples);
                auto* channelData = bufferz[idx].getWritePointer(0);
                int numberOfSample = bufferz[idx].getNumSamples();

#if AUDIO_SOURCE == 1 //overwrite stream with test wav file
                overwriteStreamWithTestWav(idx, channelData, numberOfSample);
#endif
                TestChannelID();
                // write in circulare buffer for later display
#ifdef DEBUG_BUFFER_1
                addrOfLastWavValueWritten = getAddressOfLastNonZeroWavSample(idx);
#endif //DEBUG_BUFFER_1

#ifdef DEBUG
                if (currentPostTriggerSmpCount >= halfMaxSmpCount - 2 * numSamples) //if (wavidx[idx] == 119520)
                    DBG(" Almost the last one...");
#endif // DEBUG
                //is post trigger memory still not full ?
                if (currentPostTriggerSmpCount + numSamples <= halfMaxSmpCount)//recording size limit not reached
                {
                    eScopeBuffer[idx].copyFrom(0, writePosition[idx], channelData, numSamples);
#ifdef DEBUG_BUFFER_1
                    addrOfLastWavValueWritten = getAddressOfLastNonZeroWavSample(idx);
#endif
                }
                else//last block received is too bigcopy: copy only partial block to buffer
                {
                    int nbOfSmpToCopy = halfMaxSmpCount - currentPostTriggerSmpCount;
                    eScopeBuffer[idx].copyFrom(0, writePosition[idx], channelData, nbOfSmpToCopy);
#ifdef DEBUG_BUFFER_1
                    addrOfLastWavValueWritten = getAddressOfLastNonZeroWavSample(idx);
#endif
                }
            }

            // if not arleady triggered, check if any sample in this new block is causing trigger
            if (currentPostTriggerSmpCount == 0)
            {
                unsigned int triggerIndex;

                if ((*thumbnailTriggeredPtr == false) && (thumbnailWritten == false))
                    ////if ((*thumbnailTriggeredPtr == false) && (bufferWritten == false))
                {   // check if this block has data meeting trigger condition
                    bool bTriggered = checkForLevelTrigger(numSamples, &triggerIndex, &bufferz[RecTrigChannel], RecTrigChannel);
                    *thumbnailTriggeredPtr = bTriggered;
                    if (bTriggered)
                    {
                        triggAddress = writePosition[RecTrigChannel] + triggerIndex;// sub size of block
                        triggAddress %= eScopBufferSize; //wrap if needed
                        currentPostTriggerSmpCount = numSamples - triggerIndex;//nb of samples recorded after trigger condition
#ifdef DEBUG_BUFFER_0
                        DBG("triggAddress = " << triggAddress << " currentPostTriggerSmpCount = " << currentPostTriggerSmpCount);
#endif
#ifdef DEBUG_BUFFER_1
                        addrOfLastWavValueWritten = getAddressOfLastWavSampleAboveValue(thresholdTrigger, triggAddress + 48);
#endif
                    }
                }
            }
            else if (currentPostTriggerSmpCount > 0) //if triggered condition has been met and samples have started to be recorded
            {
                    currentPostTriggerSmpCount += numSamples;//keep count of samples recorded
            }
 
            for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
            {
                writePosition[idx] += numSamples;

                if (writePosition[idx] >= eScopBufferSize) //check if buff limit reached
                    wfBufferUnderRun = false;//will grant all data in buffer are valid
                writePosition[idx] %= eScopBufferSize;
            }

                //now if we have enough samples, pass them to the Thumbnail for display
                //thumbnailWritten = WriteThumbnail(); // using numSamples ?
                // 
                //check if we have enough sample if yes, flag display to update
#ifdef DEBUG_BUFFER
            checkLastValuesOfBuffer(wavidx[1]); //debug function
#endif
            bufferWritten = PrepareBufferPointers();
            if (bufferWritten) // to allow tests / break points ONLY !
            {
                wfBufferReady = true;
                sendChangeMessage();
            }
            else
                wfBufferReady = false;
        }
        //-------------------  Mode "Recording continuously" ---------------------------
        if (activeWriter.load() != nullptr && chanID < numInputChannels)
        {
            int eScopChanNb = ESCOPE_CHAN_NB;
            for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
            {
                activeWriter.load()->write(&inputChannelData[idx], numSamples);
                // Create an AudioBuffer to wrap our incoming data, note that this does no 
                //allocations or copies, it simply references our input data
                juce::AudioBuffer<float> buffer(const_cast<float**> (&inputChannelData[idx]), 1, numSamples);// one stream per buffer
                //thumbnail.addBlock(nextSampleNum, buffer, 0, numSamples);
                thmbNail[idx]->addBlock(nextSampleNum, buffer, 0, numSamples);                
            }
            nextSampleNum += numSamples;
        }

        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
    }
    //----------------------------------------------------------------------------------
    bool PrepareBufferPointers(void)
    {
        if ((currentPostTriggerSmpCount >= halfMaxSmpCount) && (bufferWritten == false))
        {
#ifdef DEBUG_BUFFER
            DBG("Preparing buffer");
#endif
#ifdef DEBUG_BUFFER_1
            long addrOfLastWavValueWritten;
            addrOfLastWavValueWritten = getAddressOfLastNonZeroWavSample(0);
#endif
            bufferWritten = true;
            //copy data that are before the Threshold
            if (triggAddress >= halfMaxSmpCount) //we have enough data before trig 
            {
                int offsetInEScopeBuffer = triggAddress - halfMaxSmpCount;
                int smpCount;
                juce::int64 nbOfSmpInThumbnail;
                if (triggAddress + halfMaxSmpCount <= eScopBufferSize)//tail data not wrapped
                {
                    smpCount = triggAddress + halfMaxSmpCount;
                    eScopBufferSize = 0; // reset flag for tests
                    wfStartAddress = smpCount;
                    wfTriggAddress = triggAddress;
                }
                else //tail data wrapped 
                {
                    smpCount = eScopBufferSize - triggAddress + halfMaxSmpCount;
                    unsigned long copyStart = triggAddress - halfMaxSmpCount;
                    smpCount = maxSmpCount - smpCount;
                    eScopBufferSize = 0; // reset flag for tests
                    wfStartAddress = smpCount;
                    wfTriggAddress = triggAddress;
                }
            }
            else //head data wrapped or not enough data recorded before trigger point
            {
                int offsetInEScopeBuffer = 0;
                int smpCount;
                if (triggAddress + halfMaxSmpCount <= eScopBufferSize)//tail data not wrapped
                {
                    smpCount = triggAddress + halfMaxSmpCount;
                    int paddingSmpNb = halfMaxSmpCount - triggAddress;
                    int paddingPtrinBuffer = eScopBufferSize - paddingSmpNb;
                    eScopBufferSize = 0; // reset flag for tests
                    wfStartAddress = paddingPtrinBuffer;
                    wfTriggAddress = triggAddress;
                }
                else //should never happen
                {
                    eScopBufferSize = 0; // reset flag for tests
                    wfStartAddress = 0;
                    wfTriggAddress = triggAddress;
                }
            }
            currentPostTriggerSmpCount = 0;
            //thumbnail.reset(1, sampleRate, 0); //[ToBeChanged]
            thmbNail[0]->reset(1, sampleRate, 0);
            return(true);
        }
        else
            return(false);
    }
    //----------------------------------------------------------------------------------
    void saveBufferAsWav(const juce::AudioBuffer<float>& buffer, const juce::File& fileToSave)
    {
        if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        {
            DBG("Le buffer est vide ou non initialisé");
            return;
        }        
        juce::WavAudioFormat wavFormat; // Crée un WavAudioFormat pour gérer le format WAV
        // Ouvre un flux de sortie vers le fichier
        std::unique_ptr<juce::FileOutputStream> fileStream(fileToSave.createOutputStream());

        int buffSize;
        buffSize = buffer.getNumSamples();

        if (fileStream == nullptr || !fileStream->openedOk())
        {
            DBG("Impossible de créer ou ouvrir le fichier WAV pour l'écriture");
            return;
        }

        if (fileStream != nullptr)
        {
            // Crée un AudioFormatWriter pour écrire les données du buffer dans le fichier WAV
            int bitDepth = 16;
            double sampleRate = 48000.0;
            unsigned int chanNb = 1;

            std::unique_ptr<juce::AudioFormatWriter> writer;
            writer.reset(wavFormat.createWriterFor(fileStream.get(),
                48000.0,
                buffer.getNumChannels(),
                24,
                {},
                0));

            if (writer != nullptr)
            {
                // Libère le flux, car l'AudioFormatWriter va maintenant le gérer
                fileStream.release();
                // Écrit le contenu du buffer dans le fichier WAV
                auto buffSize = buffer.getNumSamples();
                auto chanNb = buffer.getNumChannels();
                float wavData;
                for (int xx = 0; xx < chanNb; xx++)
                {
                    for (int yy = 0; yy < buffSize; yy++)
                    {
                        wavData = buffer.getSample(xx, yy);
                    }
                }
                //check if buffer 
                if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffSize))
                {
                    DBG("Erreur lors de l'écriture du buffer dans le fichier WAV");
                }
                else
                {
                    DBG("Fichier WAV écrit avec succès !");
                }
                writer.reset();
            }
            else
            {
                DBG("Impossible de créer un writer pour le format WAV");
            }

        }
    }
    //----------------------------------------------------------------------------------
    void testSaveWaves()
    {   juce::AudioBuffer<float> testBuffer(1, 48000); // 2 canaux, 1 seconde de données à 44100 Hz
    for (int channel = 0; channel < testBuffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < testBuffer.getNumSamples(); ++sample)
        {
            float value = std::sin(2.0 * juce::MathConstants<float>::pi * sample / testBuffer.getNumSamples());
            testBuffer.setSample(channel, sample, value);
        }
    }
    double magni = testBuffer.getMagnitude(0, testBuffer.getNumSamples());
    double maxim = testBuffer.getSample(0, testBuffer.getNumSamples() / 4.0);
    double minim = testBuffer.getSample(0, testBuffer.getNumSamples() * 3.0 / 4.0);
    // Sauvegarder ce buffer en WAV
    saveBufferAsWav(testBuffer, juce::File("test_wav.wav"));
    }
    //----------------------------------------------------------------------------------
#ifdef DEBUG_BUFFER
    long getAddressOfLastNonZeroWavSample(int idx)// for debug only
    {
        double data;
        long addr;
        addr = eScopeBuffer[idx].getNumSamples() - 1;
        if (addr < 0)
        {
            addr = 0;
        }
        else
        {
            while ((data = eScopeBuffer[idx].getSample(0, addr)) == 0)
            {
                addr--;
                if (addr < 0)
                {
                    DBG("Last non zero value = " << data << " at addr " << addr + 1);
                    return addr;
                }
            }
        }
        DBG("Last non zero value = " << data << " at addr " << addr << " on Chan " << idx);
        return addr;
    }
    //----------------------------------------------------------------------------------
    long getAddressOfLastWavSampleAboveValue(double value, long startAddr)// for debug only
    {
        double data;
        long addr;

        addr = startAddr; // eScopeBuffer[0].getNumSamples() - 1;
        if (addr < 0)
        {
            addr = 0;
        }
        else
        {
            while (((data = eScopeBuffer[0].getSample(0, addr)) > value) && ((data = eScopeBuffer[0].getSample(0, addr)) != 0))
            {
                addr--;
                if (addr < 0)
                {
                    DBG("Last value above " << juce::String(value) << " is " << data << " at addr " << addr + 1);
                    return addr;
                }
            }
        }
        DBG("Last value above " << juce::String(value) << " is " << data << " at addr " << addr << " on Chan " << chanID);
        //DBG("Last non zero value = " << data << " at addr " << addr << " on Chan " << chanID);
        return addr;
    }
    //----------------------------------------------------------------------------------
    void checkLastValuesOfBuffer(long addr) //debug function to check the end of each buffer 
    {
        /* made global so values are shown it the watch anywhere
        double lastThreeBuff0[3];
        double lastThreeBuff1[3];*/
        long address;
        address = addr;
        if (addr < 4)
            return;
        if (addr > eScopBufferSize) //only do modulo if >, otherwise will not see the end
            addr %= eScopBufferSize; //pointer modulo buffersize
        for (long idx = 1; idx < 4; idx++)
        {
            lastThreeBuff0[idx - 1] = eScopeBuffer[0].getSample(0, addr - idx);
            lastThreeBuff1[idx - 1] = eScopeBuffer[1].getSample(0, addr - idx);
        }
    }
#endif
    //----------------------------------------------------------------------------------
    void saveWaves()
    {
        juce::String fileName;
        juce::AudioBuffer<float> eBuffer;        
        long addrOfLastWavValueWritten;
        addrOfLastWavValueWritten = getAddressOfLastNonZeroWavSample(0);

        for (int idx = 0; idx < ESCOPE_CHAN_NB; idx++)
        {
            fileName.clear();
            fileName << "Wave_" << idx << ".wav";
            juce::File fileToSave(fileName);
            eBuffer.setSize(1, eScopeBuffer[idx].getNumSamples());
            eBuffer.copyFrom(0, 0, eScopeBuffer[idx], 0, 0, eScopeBuffer[idx].getNumSamples());
            unsigned long ptNb = eBuffer.getNumSamples();
            saveBufferAsWav(eBuffer, fileToSave);
        }
    }
    //----------------------------------------------------------------------------------
        bool WriteThumbnail(void)
        {
            int idx = 0;
            if ((currentPostTriggerSmpCount >= halfMaxSmpCount) && (thumbnailWritten == false))
            {
                thumbnailWritten = true;
                //thumbnail.reset(1, sampleRate, 0);
                thmbNail[0]->reset(1, sampleRate, 0);
                //copy data that are before the Threshold
                if (triggAddress >= halfMaxSmpCount) //head data not wrapped 
                {
                    int offsetInEScopeBuffer = triggAddress - halfMaxSmpCount;
                    int smpCount;
                    juce::int64 nbOfSmpInThumbnail;
                    if (triggAddress + halfMaxSmpCount <= eScopBufferSize)//tail data not wrapped
                    {
                        smpCount = triggAddress + halfMaxSmpCount;
                        //thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, maxSmpCount);
                        ///thumbnail.addBlock(0, eScopeBuffer, offsetInEScopeBuffer, maxSmpCount);
                        ///nbOfSmpInThumbnail = thumbnail.getNumSamplesFinished();
                        thmbNail[0]->addBlock(0, eScopeBuffer[idx], offsetInEScopeBuffer, maxSmpCount);
                        nbOfSmpInThumbnail = thmbNail[0]->getNumSamplesFinished();
                        //eScopBufferSize = 0; // reset flag for tests
                    }
                    else //tail data wrapped 
                    {
                       
                        smpCount = eScopBufferSize - triggAddress + halfMaxSmpCount;
                        unsigned long copyStart = triggAddress - halfMaxSmpCount;
                        //thumbnail.addBlock(0, eScopeBuffer, copyStart, smpCount);
                        //nbOfSmpInThumbnail = thumbnail.getNumSamplesFinished();
                        thmbNail[0]->addBlock(0, eScopeBuffer[idx], copyStart, smpCount);
                        nbOfSmpInThumbnail = thmbNail[0]->getNumSamplesFinished();

                        juce::int64 nextSmpNb = smpCount;
                        smpCount = maxSmpCount - smpCount;
                        copyStart = 0;
                        //thumbnail.addBlock(nextSmpNb, eScopeBuffer, copyStart, smpCount);
                        thmbNail[0]->addBlock(nextSmpNb, eScopeBuffer[idx], copyStart, smpCount);

                        //nbOfSmpInThumbnail = thumbnail.getNumSamplesFinished();
                        nbOfSmpInThumbnail = thmbNail[0]->getNumSamplesFinished();

                        eScopBufferSize = 0; // reset flag for tests
                        wfStartAddress = smpCount;
                        wfTriggAddress = triggAddress;
                    }
                }
                else //head data wrapped or not enough data recorded before trigger point
                {
                    int offsetInEScopeBuffer = 0;
                    int smpCount;
                    if (triggAddress + halfMaxSmpCount <= eScopBufferSize)//tail data not wrapped
                    {
                        smpCount = triggAddress + halfMaxSmpCount;
                        int paddingSmpNb = halfMaxSmpCount - triggAddress;
                        int paddingPtrinBuffer = eScopBufferSize - paddingSmpNb;
                        //thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, maxSmpCount);
                        ///thumbnail.addBlock(0, eScopeBuffer, paddingPtrinBuffer, paddingSmpNb);
                        ///thumbnail.addBlock(paddingSmpNb, eScopeBuffer, offsetInEScopeBuffer, smpCount);
                        thmbNail[0]->addBlock(0, eScopeBuffer[idx], paddingPtrinBuffer, paddingSmpNb);
                        thmbNail[0]->addBlock(paddingSmpNb, eScopeBuffer[idx], offsetInEScopeBuffer, smpCount);
                        //eScopBufferSize = 0; // reset flag for tests
                        wfStartAddress = paddingPtrinBuffer;
                        wfTriggAddress = triggAddress;
                    }
                    else //tail data wrapped 
                    {
                        smpCount = eScopBufferSize - triggAddress + halfMaxSmpCount;
                        ///thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, smpCount);
                        thmbNail[0]->addBlock(offsetInEScopeBuffer, eScopeBuffer[idx], triggAddress, smpCount);
                        smpCount = maxSmpCount - smpCount;
                        ///thumbnail.addBlock(offsetInEScopeBuffer, eScopeBuffer, triggAddress, smpCount);
                        thmbNail[0]->addBlock(offsetInEScopeBuffer, eScopeBuffer[idx], triggAddress, smpCount);
                    }

                }
                //copy data aftert the Threshold
                if (eScopBufferSize - triggAddress >= halfMaxSmpCount)//data not wrapped ?
                {
                    //thumbnail.addBlock(nextSampleNum, eScopeBuffer, 0, numSamples);
                }
                else
                {

                }
                currentPostTriggerSmpCount = 0;
                return(true);
            }
            else
                return(false);            
        }
        //----------------------------------------------------------------------------------
        int getChannelID(void)  {  return(chanID);  }
        //----------------------------------------------------------------------------------
        void setChannelID(int setChanID) {  chanID = setChanID; }
        //-------------------------------------
        void TestChannelID() // for debugging multiple channel sessions
        {
            int channel;
            switch (chanID)
            {
            case 0:
                channel = 0;
                break;
            case 1:
                channel = 1;
                break;
            case 2:
                channel = 2;
                break;
            case 3:
                channel = 3;
                break;
            case 4:
                channel = 4;
                break;
            case 5:
                channel = 5;
                break;
            case 6:
                channel = 6;
                break;
            case 7:
                channel = 7;
                break;
            default:
                channel = 10;
            }
        }
        //----------------------------------------------------------------------------------
        void setTriggerPtr(bool* ptr)  { thumbnailTriggeredPtr = ptr;}
        //----------------------------------------------------------------------------------
//        void setTriggerMode(int mode) { RecTrigMode = mode; }
        //----------------------------------------------------------------------------------
#if AUDIO_SOURCE == 1 //for debug and tests only
        void overwriteStreamWithTestWav(int index, float* chanDataptr, int numSmp)
        {
            int sample = 0;
            long startAddr = wavidx[index];
            long endAddr;
            //only copy at the beginning of the stream (the size of the array) 
            while((wavidx[index] < BleepSize) && (sample < numSmp)) 
            {
                    if(index==0)
                        *chanDataptr = *wavptr[index];
                    else
                        *chanDataptr = - *wavptr[index];
                    chanDataptr++;
                    wavptr[index]++;
                    sample++;
                    wavidx[index]++; //to check that total count doesn't exceed wav size 
            }
            endAddr = wavidx[index];
#ifdef DEBUG_BUFFER_0
            if (index == 0)
                DBG("overwriteStream from " << startAddr << " to " << endAddr);
#endif // DEBUG_BUFFER_0            
        }
#endif //AUDIO_SOURCE
        //----------------------------------------------------------------------------------

    private:
        grape::Panel* triggerSettings;  // handle to trigger settings
        //AudioThumbnail& thumbnail; //pointer to associated audiothumbnail
        juce::AudioThumbnail* thmbNail[ESCOPE_CHAN_NB]; // pointer to thumbnails associated with the recorder;
        int thumbnailSize = 0;

        // the thread that will write our audio data to disk
        juce::TimeSliceThread backgroundThread{ "Audio Recorder Thread" };
        // the FIFO used to buffer the incoming data
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
        double sampleRate = 0.0;
        int samplesPerBlockExpected = 0;
        juce::int64 triggAddress = 0;

        int chanNb = 1;
        int bitDepth = 24;
        juce::int64 nextSampleNum = 0;
        juce::CriticalSection writerLock;
        std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };

        int chanID = 0; // default channel selected in multi channel audio interface is first one
        bool* thumbnailTriggeredPtr; // <- declare ptr to flag accessible by display part
        juce::AudioBuffer<float>eScopeBuffer[ESCOPE_CHAN_NB];
        juce::int64  eScopBufferSize;
        juce::int64 writePosition[ESCOPE_CHAN_NB];
        juce::int64 readPosition = 0;
        float triggerPlaceRatio = 0.5;
        juce::int64 currentPostTriggerSmpCount = 0;
        juce::int64 maxSmpCount = 0;
        juce::int64 halfMaxSmpCount = 0;
        juce::int64 absTriggerAddress = 0;// DEBUG ONLY:
        long expectedTrigAddress = 7680; // DEBUG ONLY: 7680 for level 0.06 ** 6721 for level 0.0560
        double previousSampleValue[ESCOPE_CHAN_NB]; //stores value(t-1) for trigger detection 
        unsigned long wavaddr[ESCOPE_CHAN_NB];// = 0;
        unsigned long wavidx[ESCOPE_CHAN_NB];// = 0;
        //following are shared by pointers between recorder and view
        unsigned long wfStartAddress = 0;
        unsigned long wfTriggAddress = 0;
        bool wfBufferReady = false;
        bool wfBufferUnderRun = true;
        //end of shared

        const float* wavptr[ESCOPE_CHAN_NB];// DEBUG ONLY to override input signal by data LUT in .h
        juce::uint16 wavSize = 48000;
        bool thumbnailWritten = false;
        bool bufferWritten = false;
#ifdef DEBUG_BUFFER
        /* made global so values are shown it the watch anywhere*/
        double lastThreeBuff0[3];
        double lastThreeBuff1[3];
#endif
    };


