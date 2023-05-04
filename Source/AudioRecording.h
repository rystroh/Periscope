/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             AudioRecordingDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Records audio to a file.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2022, linux_make, androidstudio, xcode_iphone

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:             Component
 mainClass:        AudioRecordingDemo

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
namespace juce
{


//#include "DemoUtilities.h"
//#include "AudioLiveScrollingDisplay.h"
#include <vector>
#include <algorithm>
//==============================================================================
/** A simple class that acts as an AudioIODeviceCallback and writes the
    incoming audio data to a WAV file.
*/
class AudioRecorder  : public AudioIODeviceCallback
{
public:
    AudioRecorder (AudioThumbnail& thumbnailToUpdate)
        : thumbnail (thumbnailToUpdate)
    {
        backgroundThread.startThread();
    }

    ~AudioRecorder() override
    {
        stop();
    }

    //==================================================================================
    void startRecording (const File& file)
    {
        stop();

        if (sampleRate > 0)
        {
            // Create an OutputStream to write to our destination file...
            file.deleteFile();

            if (auto fileStream = std::unique_ptr<FileOutputStream> (file.createOutputStream()))
            {
                // Now create a WAV writer object that writes to our output stream...
                WavAudioFormat wavFormat;

                if (auto writer = wavFormat.createWriterFor (fileStream.get(), sampleRate, 2, 24, {}, 0))
                {
                    fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

                    // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                    // write the data to disk on our background thread.
                    threadedWriter.reset (new AudioFormatWriter::ThreadedWriter (writer, backgroundThread, 32768));

                    // Reset our recording thumbnail
                    thumbnail.reset (writer->getNumChannels(), writer->getSampleRate());
                    nextSampleNum = 0;

                    // And now, swap over our active writer pointer so that the audio callback will start using it..
                    const ScopedLock sl (writerLock);
                    activeWriter = threadedWriter.get();
                }
            }
        }
    }
 //-------------------------------------------------------------------------------------
    void stop()
    {
        // First, clear this pointer to stop the audio callback from using our writer object..
        {
            const ScopedLock sl (writerLock);
            activeWriter = nullptr;
        }

        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
        // the audio callback while this happens.
        threadedWriter.reset();
    }
 //-------------------------------------------------------------------------------------
    bool isRecording() const
    {
        return activeWriter.load() != nullptr;
    }

//=====================================================================================
    void audioDeviceAboutToStart(AudioIODevice* device) override
    {
        sampleRate = device->getCurrentSampleRate();
    }
//-------------------------------------------------------------------------------------
    double getSampleRate(void)
    {
        return(sampleRate);
    }
//-------------------------------------------------------------------------------------
    void audioDeviceStopped() override
    {
        sampleRate = 0;
    }
//-------------------------------------------------------------------------------------
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples, const AudioIODeviceCallbackContext& context) override
    {
        ignoreUnused (context);

        const ScopedLock sl (writerLock);

        if (activeWriter.load() != nullptr && numInputChannels >= thumbnail.getNumChannels())
        {
            activeWriter.load()->write (inputChannelData, numSamples);

            // Create an AudioBuffer to wrap our incoming data, note that this does no allocations or copies, it simply references our input data
            AudioBuffer<float> buffer (const_cast<float**> (inputChannelData), thumbnail.getNumChannels(), numSamples);
            thumbnail.addBlock (nextSampleNum, buffer, 0, numSamples);
            nextSampleNum += numSamples;
        }

        // We need to clear the output buffers, in case they're full of junk..
        for (int i = 0; i < numOutputChannels; ++i)
            if (outputChannelData[i] != nullptr)
                FloatVectorOperations::clear (outputChannelData[i], numSamples);
    }

private:
    AudioThumbnail& thumbnail;
    TimeSliceThread backgroundThread { "Audio Recorder Thread" }; // the thread that will write our audio data to disk
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    double sampleRate = 0.0;
    int64 nextSampleNum = 0;

    CriticalSection writerLock;
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
};

//==============================================================================
class RecordingThumbnail  : public Component,
                            private ChangeListener,
                            private juce::ScrollBar::Listener
{
public:
    RecordingThumbnail()
    {
        addAndMakeVisible(scrollbar);
        scrollbar.setRangeLimits(visibleRange);
        scrollbar.setAutoHide(true);
        scrollbar.addListener(this);
        formatManager.registerBasicFormats();
        thumbnail.addChangeListener (this);
    }

    ~RecordingThumbnail() override
    {
        scrollbar.removeListener(this);
        thumbnail.removeChangeListener (this);
    }

    AudioThumbnail& getAudioThumbnail()     { return thumbnail; }
    bool setSource(InputSource* newSource) { return(thumbnail.setSource(newSource)); }
    
//-------------------------------------------------------------------------------------
    void setSampleRate(double smpRate)
    {
        sampleRate = smpRate;
    }
//-------------------------------------------------------------------------------------
    void setDisplayFullThumbnail (bool displayFull)
    {
        displayFullThumb = displayFull;
        if (displayFull)
        {
            auto thumbnailsize = thumbnail.getTotalLength();
            Range<double> newRange(0.0, thumbnailsize);
            scrollbar.setRangeLimits(newRange);
            setRange(newRange);
            //repaint();
        }            
        else
            repaint();

    }
//-------------------------------------------------------------------------------------
    void setDisplayThumbnailMode(int displayMode)
    {
        displayThumbMode = displayMode;
        /*
        switch (displayMode)
        {
            case 0: //Full Thumb mode
                repaint();
                break;
            case 1: // recording mode
                repaint();
                break;
            case 2: // zooming mode
                auto thumbnailsize = thumbnail.getTotalLength();
                Range<double> newRange(0.0, thumbnailsize);
                scrollbar.setRangeLimits(newRange);
                setRange(newRange);
                break;
            default:
                repaint();

        }*/
    }
//-------------------------------------------------------------------------------------
 /*   void setDisplayXZoom(double xZoom)
    {
        ThumbXZoom = xZoom;
            auto toto = jlimit(0.0000, 1.0, xZoom); // use jmap ? map2log10 ? use skew ?
        ThumbXZoom = toto;
        displayFullThumb = false;
        displayThumbMode = 2; //zoom mode
     //   repaint();
        if (thumbnail.getTotalLength() > 0)
        {
            auto thumbnailsize = thumbnail.getTotalLength();
            auto width = getWidth();
            auto newScale = jmax(0.001, thumbnail.getTotalLength() * (1.0 - jlimit(0.0, 0.99999999, xZoom)));
            
            auto timeAtCentre = xToTime((float)getWidth() / 2.0f);
            //timeAtCentre returns (x / (float)getWidth()) * (visibleRange.getLength()) + visibleRange.getStart();
            // 
         //   DBG("thumbnailsize = " << thumbnailsize << " width = " << width << " timeAtCentre = " << timeAtCentre << " NewSc = " << newScale);
            //DBG("thumbnailsize = " << thumbnailsize << " vRLength = " << visibleRange.getLength() << " vRStart = " << visibleRange.getStart() << " timeAtCentre = " << timeAtCentre << " NewSc = " << newScale);
            setRange({ timeAtCentre - newScale * 0.5, timeAtCentre + newScale * 0.5 });
        }
        else
            repaint();
    }*/
 //-------------------------------------------------------------------------------------
    void setDisplayYZoom(double yZoom)
    {
        ThumbYZoom = yZoom;
        if (yZoom == 1.0)
            YZoomIndex = 0;
        DBG("Y Zoom = " << ThumbYZoom);
        repaint();
    }
 //-------------------------------------------------------------------------------------
    void setZoomFactor(double amount)
    {
        auto toto = jlimit(0.0001, 0.99, amount);

        if (thumbnail.getTotalLength() > 0)
        {
            auto thumbnailsize = thumbnail.getTotalLength();
            auto width = getWidth();

            auto newScale = jmax(0.001, thumbnail.getTotalLength() * (1.0 - jlimit(0.0, 0.99, amount)));
            auto timeAtCentre = xToTime((float)getWidth() / 2.0f);
            setRange({ timeAtCentre - newScale * 0.5, timeAtCentre + newScale * 0.5 });
        }
    }
//-------------------------------------------------------------------------------------
    void setRange(Range<double> newRange)
    {
        visibleRange = newRange;
        scrollbar.setCurrentRange(visibleRange);
        //updateCursorPosition();
        repaint();
    }
//-------------------------------------------------------------------------------------
    void paintGrid(juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
    {
        int newY2, newY41, newY42, newY81, newY82, newY83, newY84, thumbh;
        thumbh = thumbnailBounds.getHeight();
        newY2 = thumbnailBounds.getCentreY();
        g.setColour(juce::Colours::grey);
        g.drawLine(thumbnailBounds.getX(), newY2, thumbnailBounds.getRight(), newY2);

        newY41 = newY2 - thumbh / 4;
        newY42 = newY2 + thumbh / 4;
        g.setColour(juce::Colours::darkgrey);
        g.drawLine(thumbnailBounds.getX(), newY42, thumbnailBounds.getRight(), newY42);
        g.drawLine(thumbnailBounds.getX(), newY41, thumbnailBounds.getRight(), newY41);
        newY81 = newY41 - thumbh / 8;
        newY82 = newY41 + thumbh / 8;
        newY83 = newY42 - thumbh / 8;
        newY84 = newY42 + thumbh / 8;

        g.drawLine(thumbnailBounds.getX(), newY81, thumbnailBounds.getRight(), newY81);
        g.drawLine(thumbnailBounds.getX(), newY82, thumbnailBounds.getRight(), newY82);
        g.drawLine(thumbnailBounds.getX(), newY83, thumbnailBounds.getRight(), newY83);
        g.drawLine(thumbnailBounds.getX(), newY84, thumbnailBounds.getRight(), newY84);

    }
//-------------------------------------------------------------------------------------
    /*void paintit(Graphics& g) override
    {
        g.fillAll (Colours::black);
        g.setColour (Colours::aquamarine);

        if (thumbnail.getTotalLength() > 0.0)
        {
         
            double startTime = 0.0f;
            double  endTime = 1.0f;

            auto thumbArea = getLocalBounds();
            
            //thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
            //thumbnail.drawChannels(g, thumbArea.reduced(2),
            //    visibleRange.getStart(), visibleRange.getEnd(), ThumbYZoom);

            
            if(displayFullThumb)
            { 
                startTime = 0.0f;
                endTime = thumbnail.getTotalLength();
                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, endTime, ThumbYZoom);
            }
            else
            {
               
                double centerTime = thumbnail.getTotalLength() / 2.0f;
                startTime = centerTime - ThumbXZoom * thumbnail.getTotalLength() / 2.0f;
                endTime = centerTime + ThumbXZoom * thumbnail.getTotalLength() / 2.0f;

                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                thumbnail.drawChannels(g, thumbArea.reduced(2),visibleRange.getStart(), visibleRange.getEnd(), ThumbYZoom);
                //thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, endTime, ThumbYZoom);
            }
            //auto thumbArea = getLocalBounds();
            //paintGrid(g, thumbArea);
            
        }
        else
        {
            g.setFont (14.0f);
            //g.drawFittedText ("(No file recorded)", getLocalBounds(), Justification::centred, 2);
        }
    }*/
//-------------------------------------------------------------------------------------
    void paint(Graphics& g) override
    {
        g.fillAll(Colours::black);
        g.setColour(Colours::aquamarine);

        if (thumbnail.getTotalLength() > 0.0)
        {
            double startTime = 0.0f;
            double  endTime = 1.0f;
            double  endofrecording = 1.0f;
            auto thumbArea = getLocalBounds();
            double currentlength = thumbnail.getTotalLength();
            endofrecording = jmax(10.0, currentlength);
            Range<double> newRange;
            double thumbnailsize;
            int xzoomticknb;

            switch (displayThumbMode)
            {
            case 0: //Full Thumb mode (expand recording data to window when stopping Recording
                endTime = thumbnail.getTotalLength();
                scrollbar.setAutoHide(false);              
                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, endTime, ThumbYZoom);
                newRange.setStart(0.0);
                newRange.setEnd(endTime);
                scrollbar.setRangeLimits(newRange);
                setRange(newRange);
                xzoomticknb = createZoomVector(zoomVector);
                break;
            case 1: // recording mode (scrolling data)                
                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                //thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, jmin(1.0,endTime), ThumbYZoom);
                thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, endofrecording, ThumbYZoom);
                break;

            case 2: // zooming mode                
                thumbnailsize = thumbnail.getTotalLength();
                newRange.setStart(0.0);
                newRange.setEnd(thumbnailsize);
                
                scrollbar.setRangeLimits(newRange);
                //setRange(newRange);
                /*
                double centerTime = thumbnail.getTotalLength() / 2.0f;
                startTime = centerTime - ThumbXZoom * thumbnail.getTotalLength() / 2.0f;
                endTime = centerTime + ThumbXZoom * thumbnail.getTotalLength() / 2.0f;*/
                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                thumbnail.drawChannels(g, thumbArea.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), ThumbYZoom);
                break;
            case 3: //stopping
                thumbnailsize = thumbnail.getTotalLength();
                newRange.setStart(0.0);
                newRange.setEnd(thumbnailsize);

                scrollbar.setRangeLimits(newRange);
                setRange(newRange);
                displayThumbMode = 2; // get ready for zooming
                break;
            }


        }
        else
        {
            g.setFont(14.0f);
            //g.drawFittedText ("(No file recorded)", getLocalBounds(), Justification::centred, 2);
        }
    }
//-------------------------------------------------------------------------------------
    void resized() override
    {
        int xzoomticknb;
        scrollbar.setBounds(getLocalBounds().removeFromBottom(14).reduced(2));
        xzoomticknb = createZoomVector(zoomVector);
    }
//-------------------------------------------------------------------------------------
    int  createZoomVector(std::vector<double>& Divider)
    {
        //auto vrange = visibleRange.getLength();
        auto totlen = thumbnail.getTotalLength();
        auto thumbArea = getLocalBounds();
   
    //    auto SampleRate = 48000;
        double SampleSize = totlen * sampleRate;
        double Ratio = SampleSize / thumbArea.getWidth();
        double div = Ratio;
        int it = 0;
        int iteration = 0;
        double seed = 1.0;
        double sub1Tab[]{ 24.0, 16.0, 12.0, 8.0, 6.0 , 4.0, 3.0 , 2.0 };

        std::vector<double> Divider2;// , Divider;
        Divider.clear();
        Divider2.clear();
        for (double n : sub1Tab)
        {
            Divider2.push_back(1.0/n);
        }
        while (div > 2)
        {
            div = div / 2;
            iteration++;
        }
        Divider2.push_back(seed);
        while (it < iteration)
        {
            seed *= 2;
            if (seed < Ratio)
                Divider2.push_back(seed);
            it++;
        }
        for (auto iter = Divider2.cbegin(); iter != Divider2.cend(); ++iter)
        {
            seed = *iter;
            Divider.push_back(seed);
            seed *= 3.0;
            if (seed < Ratio)
                Divider.push_back(seed);
        }

        std::sort(Divider.begin(), Divider.end());

        if (Ratio > Divider[Divider.size() - 1])
            Divider.push_back(Ratio); //if Ratio is not already there, add it 
        std::sort(Divider.begin(), Divider.end(), std::greater());// greater<double>());

        return (Divider.size());
    }
//-------------------------------------------------------------------------------------
    void mouseDown(const MouseEvent& event)
    {
        auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
        DBG("Mouse.x = " << Posi3.getX());
    }
//-------------------------------------------------------------------------------------
    void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& wheel) override
    {
        auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
        if (thumbnail.getTotalLength() > 0.0)
        {
            if (juce::ModifierKeys::currentModifiers.isCtrlDown()) //Y Zoom
            {
                auto WheelDelta = wheel.deltaY;
                if (WheelDelta > 0)
                {
                    if (YZoomIndex < 48)
                    {
                        YZoomIndex++;
                        ThumbYZoom = ThumbYZoom * 1.4125354;
                        repaint();
                    }
                }
                else
                {
                    if (YZoomIndex > 0)
                    {
                        YZoomIndex--;
                        ThumbYZoom = ThumbYZoom / 1.4125354;
                        repaint();
                    }
                }
              
            }
                
            else if (juce::ModifierKeys::currentModifiers.isAltDown())//X Zoom Control
                repaint();
            else if (juce::ModifierKeys::currentModifiers.isShiftDown())//X Move
            {
                auto newStart = visibleRange.getStart() - wheel.deltaY * (visibleRange.getLength()) / 10.0;
                newStart = jlimit(0.0, jmax(0.0, thumbnail.getTotalLength() - (visibleRange.getLength())), newStart);     
                setRange({ newStart, newStart + visibleRange.getLength() });
                repaint();
            }
            else //X Zoom Control
            {
                auto WheelDelta = wheel.deltaY;
                auto totlen = thumbnail.getTotalLength();
                auto vrange = visibleRange.getLength();
                if(totlen== vrange)
                    XZoomIndex = 0;
                double NewZoomFactor;
                //DBG("XZoomIndex = " << XZoomIndex << " vectorSize = " << zoomVector.size());
                if (WheelDelta > 0)
                {
                    if (XZoomIndex < zoomVector.size()-1)
                    {
                        XZoomIndex++;
                        NewZoomFactor =  zoomVector[XZoomIndex];
                    }
                    else
                        NewZoomFactor = zoomVector[zoomVector.size() - 1];
                }
                else
                {
                    if (XZoomIndex > 0)
                    {
                        XZoomIndex--;
                        NewZoomFactor = zoomVector[XZoomIndex];
                    }
                    else
                        NewZoomFactor = zoomVector[0];
                }
                //DBG("XZoomIndex = " << XZoomIndex << " vectorSize = " << zoomVector.size() << " NewZoom " << NewZoomFactor);
                setDisplayXZone(NewZoomFactor);

            }
        }
    }
    //-------------------------------------------------------------------------------------
    void setDisplayXZone(double zoomfactor)
    {
        displayFullThumb = false;
        displayThumbMode = 2; //zoom mode
        auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
     //   repaint();
        if (thumbnail.getTotalLength() > 0)
        {            
     //       auto SampleRate = 48000;
            auto totlen = thumbnail.getTotalLength(); //total length of sample in seconds
            double displayStartTime, displayEndTime, displayWidth;

            auto thumbArea = getLocalBounds(); //bounds of display zone
            auto width = getWidth(); // width of Display zone in pixels

            double SampleSize = totlen * sampleRate; //size  of sample in points
            double Ratio = SampleSize / thumbArea.getWidth();

            auto timeAtMousePos = xToTime((float)Posi3.x);

            double PosixRatioPix = (double)width / (double) Posi3.x;
            displayWidth = totlen * zoomfactor / Ratio;

            displayStartTime = timeAtMousePos - displayWidth / PosixRatioPix;
            displayEndTime = displayStartTime + displayWidth;
         //   DBG("Mouse.x = " << Posi3.x << " PosixRatio = " << PosixRatioPix << " timeAtMousePos = " << timeAtMousePos << "(s) displayStartTime = " << displayStartTime << "(s) displayEndTime = " << displayEndTime << "(s) zoom ratio = " << zoomfactor);
            setRange({ displayStartTime, displayEndTime });
        }
        else
            repaint();
    }
    //-------------------------------------------------------------------------------------
 

private:
    AudioFormatManager formatManager;
    AudioThumbnailCache thumbnailCache  { 10 };
    AudioThumbnail thumbnail            { 1, formatManager, thumbnailCache };

    bool displayFullThumb = false;
    int displayThumbMode;
    double ThumbYZoom = 1.0f;
    int YZoomIndex = 0;
    double ThumbXZoom = 1.0f;
    int XZoomIndex = 0;
    std::vector<double> zoomVector;

    juce::ScrollBar scrollbar{ false };
    juce::Range<double> visibleRange;
    double sampleRate = 0.0;

//-------------------------------------------------------------------------------------
    float timeToX(const double time) const
    {
        if (visibleRange.getLength() <= 0)
            return 0;

        return (float)getWidth() * (float)((time - visibleRange.getStart()) / visibleRange.getLength());
    }
//-------------------------------------------------------------------------------------
    double xToTime(const float x) const
    {
        return (x / (float)getWidth()) * (visibleRange.getLength()) + visibleRange.getStart();
    }
//-------------------------------------------------------------------------------------
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        if (scrollBarThatHasMoved == &scrollbar)
            //if (!(isFollowingTransport && transportSource.isPlaying()))
            setRange(visibleRange.movedToStartAt(newRangeStart));
    }
//-------------------------------------------------------------------------------------
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &thumbnail)
            repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RecordingThumbnail)
};

//==============================================================================
}
