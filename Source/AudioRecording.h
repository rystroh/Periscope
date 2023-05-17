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
               
                if (auto writer = wavFormat.createWriterFor (fileStream.get(), sampleRate, chanNb, bitDepth, {}, 0))
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
    bool setSampleDepth(int depth)
    {
        int possibleDepth[] = { 8, 16, 24 };
        int n = sizeof(possibleDepth) / sizeof(*possibleDepth);
        bool exists = std::find(possibleDepth, possibleDepth + n, depth) != possibleDepth + n;
        if (exists) {
            bitDepth = depth;
        }
        return(exists);
    }
    //-------------------------------------------------------------------------------------
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
    int chanNb = 1;
    int bitDepth = 24;
    int64 nextSampleNum = 0;

    CriticalSection writerLock;
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
};

//==============================================================================
class RecordingThumbnail : public Component,
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
        thumbnail.addChangeListener(this);
    }

    ~RecordingThumbnail() override
    {
        scrollbar.removeListener(this);
        thumbnail.removeChangeListener(this);
    }

    AudioThumbnail& getAudioThumbnail() { return thumbnail; }
    bool setSource(InputSource* newSource) { return(thumbnail.setSource(newSource)); }
    //-------------------------------------------------------------------------------------
    void setSampleRate(double smpRate)
    {
        sampleRate = smpRate;
    }
    //-------------------------------------------------------------------------------------
    void setDisplayFullThumbnail(bool displayFull)
    {
        displayFullThumb = displayFull;
        if (displayFull)
        {
            auto thumbnailsize = thumbnail.getTotalLength();
            Range<double> newRange(0.0, thumbnailsize);
            scrollbar.setRangeLimits(newRange);
            setRange(newRange);
        }
        else
            repaint();
    }
    //-------------------------------------------------------------------------------------
    void setDisplayThumbnailMode(int displayMode)
    {
        displayThumbMode = displayMode;
    }
    //-------------------------------------------------------------------------------------
    void setDisplayYZoom(double yZoom) // called by SliderValueChanged in MainComponent.h
    {
        ThumbYZoom = yZoom;
        if (yZoom == 1.0)
            YZoomIndex = 8; // YZoomIndex = 0 <-> gain =+12dB // YZoomIndex= 8 <-> gain = 0dB
        DBG("Y Zoom = " << ThumbYZoom);
        repaint();
    }
    //-------------------------------------------------------------------------------------
 /*   void setZoomFactor(double amount)
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
    }*/
    //-------------------------------------------------------------------------------------
    void setRange(Range<double> newRange)
    {
        visibleRange = newRange;
        scrollbar.setCurrentRange(visibleRange);
        repaint();
    }
    //---------------------------------------------------------------------------------
    double  getTimeStepSize(int displayWidthPix, double wavDurationToDisplaySec)
    {
        std::vector<float>NiceTimeRatios{ 1.0 , 2.0 , 5.0 , 10.0 };
        const double minRectWidth{ 128.0 };
        // first pass determin the magnitude range
       // DBG("getTimeStepSize::displayWidthPix = " << wavDurationToDisplaySec << " displayWidthPix = " << displayWidthPix);
        double NextRatio = wavDurationToDisplaySec * minRectWidth / (double)displayWidthPix;
        int i{ 0 };
        double mag{ 1 };
        double magfloor;
        double magRatio = log10(NextRatio);
        //DBG("getTimeStepSize::NextRatio = " << NextRatio);
        magfloor = floor(magRatio);
        mag = exp(log(10.0) * -1.0 * magfloor);
        NextRatio *= mag;
        if (NextRatio >= 1.0)
        {
            while (NextRatio > NiceTimeRatios[i])
                i++;            
            return(NiceTimeRatios[i - 1] / mag);
        }
        else
            return(1);//should never happen
    }
    //---------------------------------------------------------------------------------
    std::vector<double> getTimeLineX()
    {

    }
    //---------------------------------------------------------------------------------
    std::vector<float>  getTimeLabels()
    {

    }

    //-------------------------------------------------------------------------------------
    void paintGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
    {
        double zoomfactor = 128;
        double opa = 0.5; //grid opacity
        auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
        auto totlen = thumbnail.getTotalLength(); //total length of sample in seconds
        double displayStartTime, displayEndTime, displayWidth;

        auto renderArea = bounds;
        auto top = renderArea.getY();
        auto bottom = renderArea.getBottom();
        auto left = renderArea.getX();
        auto right = renderArea.getRight();
        auto width = renderArea.getWidth(); // width of Display zone in pixels

        double SampleSize = totlen * sampleRate; //size  of sample in points
        double Ratio = SampleSize / (double)width;
        auto visRangeWidth = visibleRange.getLength();
        double curRatio = Ratio / totlen * (double)visibleRange.getLength();

        double newstepSize = getTimeStepSize(width, (double)visRangeWidth);

        if (stepSize != newstepSize)
        {
            stepSize = newstepSize;
            //DBG("paintGrid::stepSize = " << stepSize);
        }
        std::vector<double> xs = getXs(); //create vector with nice positions for vert
      
        int newX1;

        // draw vertical time lines

        g.setColour(juce::Colours::darkgrey);
        g.setOpacity(opa);
        for (auto x : xs)
        {
            newX1 = timeToX(x); // get 
            g.drawVerticalLine(newX1, top, bottom);
        }
        
        // draw horizontal Level lines
        std::vector<int> NiceGainVect;
        std::vector<int> NiceGainY;
        int ret = getNiceGainVect(bounds.getHeight(), NiceGainVect, NiceGainY);

        int newY2, newY41, newY42, newY81, newY82, newY83, newY84, thumbh;
        thumbh = bounds.getHeight();
        newY2 = bounds.getCentreY();
        g.setColour(juce::Colours::red);
        g.setOpacity(opa);
        g.drawHorizontalLine(newY2, left, right);

 

        g.setColour(juce::Colours::darkgrey);
        g.setOpacity(opa);
        
        for (auto y : NiceGainY)
        {
            newY41 = newY2 - y;
            g.drawHorizontalLine(newY41, left, right);
            newY42 = newY2 + y;
            g.drawHorizontalLine(newY42, left, right);
        }
/*
        
        if (newY41 >= top)
            g.drawHorizontalLine(newY41, left, right);
        if (newY42 <= bottom)
            g.drawHorizontalLine(newY42, left, right);

        newY81 = newY2 - yRatio / 4;
        newY82 = newY2 + yRatio / 4;
        newY83 = newY2 - yRatio / 4;
        newY84 = newY2 + yRatio / 4;

        if (newY81 >= top)
            g.drawHorizontalLine(newY81, left, right);
        if (newY84 <= bottom)
            g.drawHorizontalLine(newY84, left, right);*/
    }
    //-------------------------------------------------------------------------------------
    void drawXLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds)
    {
        g.setColour(juce::Colours::grey);
        g.setOpacity(1.0);
        const int fontHeight = 10;
        g.setFont(fontHeight);
        auto textArea = getHTextZone(bounds);// getRenderZone(bounds);
        auto left = textArea.getX();
        auto top = textArea.getY();
        auto right = left + textArea.getWidth();

        std::vector<double> xs = getXs();
        int newX1;
        for (auto x : xs)
        {
            String str;
            newX1 = timeToX(x); // get
            str << x;
            str.toDecimalStringWithSignificantFigures(x, 2);
            Rectangle<int> r;
            auto textWidth = g.getCurrentFont().getStringWidth(str);
            r.setSize(textWidth, fontHeight);
            if (newX1 + (double)textWidth/2.0 > right)
                r.setCentre(static_cast<int>((newX1)- (double)textWidth / 2.0), 0);
            else
                r.setCentre(static_cast<int>(newX1), 0);
           
            r.setY(textArea.getY());
            g.drawFittedText(str, r, juce::Justification::centred, 1);
        }
    }
    //-------------------------------------------------------------------------------------
    double round_fl(double x, int num_decimal_precision_digits)
    {
        double power_of_10 = std::pow(10, num_decimal_precision_digits);
        double xr = std::round(x * power_of_10);
        xr = xr / power_of_10;
        return xr;
    }
    //-------------------------------------------------------------------------------------
    std::vector<double> getXs()
    {
        auto timeZoneStart = visibleRange.getStart();
        auto timeZoneLen = visibleRange.getEnd();
        std::vector<double> xs;
        double x1;
        double digits;
        double mag{ 1 };
        double magfloor;
        int precision;
        double magRatio = log10(stepSize);

        //DBG("getTimeStepSize::NextRatio = " << NextRatio);
        magfloor = floor(magRatio);
        precision = abs(magfloor);
        stepSize = round_fl(stepSize, precision);
        //mag = exp(log(10.0) * -1.0 * magfloor);

        x1 = floor(timeZoneStart / stepSize) * stepSize + stepSize;
        x1 = round_fl(x1, precision);
        while (x1 <= timeZoneLen)
        {
            xs.push_back(x1);
            x1 += stepSize;
            x1 = round_fl(x1, precision);
        }
     return(xs);
    }
//-------------------------------------------------------------------------------------
    std::vector<int> getGains()//called by std::vector<int>getNiceGainVect
// creates vector of gains in dB between +12 and -144 - step = 1dB
    {
        std::vector<int> gaindB;
        for (int x = 12; x > -144; x--)
            gaindB.push_back(x); // xs.push_back(x1);
        return(gaindB);
    }
//-------------------------------------------------------------------------------------
    std::vector<long double> getZoomGainVect()
// creates vector of gains in dB between + 12 and -144 - step = 1dB
    {
        std::vector<long double> gainZdB;
        const long double dBStep = 1.5;
        for (int x = 0; x < 104; x++)
            gainZdB.push_back(pow(10,(long double)(x-8)*(long double)1.5/(long double)20.0));
        return(gainZdB);
    }
//-------------------------------------------------------------------------------------
    long double getZoomMult(int index)
    // return gain factor 
    //    index = 0  -> gain = +12 dB 
    //    index = 8  -> gain = 0 dB
    //    + 1 index  -> gain -1.5dB
    {
        long double gainMult;
        const long double dBStep = 1.5;
        gainMult = pow(10, (long double)(index - 8) * (long double)dBStep / (long double)20.0);
        return(gainMult);
    }
 //-------------------------------------------------------------------------------------
    int getNiceGainVect(int displayHeightPix, std::vector<int>& NiceGainVect, std::vector<int>& NiceGainY)
    {
        const double minRectHeight{ 15.0 }; //nb of pixel min between horizontal lines
        //std::vector<int> gaindB = getGains();
        //std::vector<long double> gainMult = getZoomGainVect();
        //std::vector<int> NiceGainVect;
        //std::vector<int> NiceGainY;
        //double previousYdB = 0;
        //double previousYpix = 0;
        //auto curYZoom = ThumbYZoom;
        int curYZoomIndex  = YZoomIndex;
        const long double dBStep = 1.5;
        double topGdB = 12.0 - curYZoomIndex * dBStep;

        double halfHeightPix = floor((double)displayHeightPix / 2.0);
        double curY = halfHeightPix;
        double ratio;
        double gain;
        double NiceY;

        while (curY > 0)
        {
            ratio = curY / halfHeightPix;
            gain = 20.0 * log10(ratio);
            gain = round(gain);
            NiceGainVect.push_back(gain + topGdB);
            ratio = pow(10.0, gain / 20);
            NiceY = halfHeightPix * ratio;
            NiceY = round(NiceY);
            NiceGainY.push_back((int)NiceY);
            curY -= minRectHeight;
        }
   
            return(0);//should never happen
    }
//-------------------------------------------------------------------------------------
    juce::Rectangle<int> getRenderZone(juce::Rectangle<int> bounds)
    {
        bounds.removeFromTop(12);
        bounds.removeFromBottom(scrollbar.getHeight() + 4);
        return bounds;
    }
    //-------------------------------------------------------------------------------------
    juce::Rectangle<int> getHTextZone(juce::Rectangle<int> bounds)
    {
        auto wavRect = getWaveZone(bounds);
        bounds.removeFromTop(4);
        bounds.removeFromBottom(wavRect.getHeight() + 2);
        return bounds;
    }
    //-------------------------------------------------------------------------------------
    juce::Rectangle<int> getWaveZone(juce::Rectangle<int> bounds)
    {
        bounds = getRenderZone(bounds);
        bounds.removeFromTop(4);
        bounds.removeFromBottom(4);
        return bounds;
    }
//-------------------------------------------------------------------------------------
    void paint(Graphics& g) override
    {
        g.fillAll(Colours::black);        

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
            juce::Rectangle<int> wavZone = getWaveZone(thumbArea);
            juce::Rectangle<int> extZone = getRenderZone(thumbArea);
            //g.setColour(juce::Colours::yellow);
            //g.drawRect(wavZone, 1.0);
            //g.setColour(juce::Colours::red);
            //g.drawRect(extZone, 1.0);
            switch (displayThumbMode)
            {
            case 0: //Full Thumb mode (expand recording data to window when stopping Recording
                endTime = thumbnail.getTotalLength();
                scrollbar.setAutoHide(false);              
                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                //paintGrid(g, thumbArea);
                paintGrid(g, wavZone);
                g.setColour(Colours::aquamarine);
                //thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, endTime, ThumbYZoom);
                thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), ThumbYZoom);

                newRange.setStart(0.0);
                newRange.setEnd(endTime);
                scrollbar.setRangeLimits(newRange);
                setRange(newRange);
                xzoomticknb = createZoomVector(zoomVector);
                drawXLabels(g, thumbArea);
                break;

            case 1: // recording mode (scrolling data)                
                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                //thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, jmin(1.0,endTime), ThumbYZoom);
                g.setColour(Colours::aquamarine);
                //thumbnail.drawChannels(g, thumbArea.reduced(2), startTime, endofrecording, ThumbYZoom);
                thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), ThumbYZoom);
                break;

            case 2: // zooming mode                
                thumbnailsize = thumbnail.getTotalLength();
                newRange.setStart(0.0);
                newRange.setEnd(thumbnailsize);             
                scrollbar.setRangeLimits(newRange);
                thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                //paintGrid(g, thumbArea);
                paintGrid(g, wavZone);
                g.setColour(Colours::aquamarine);
                //thumbnail.drawChannels(g, thumbArea.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), ThumbYZoom);
                thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), ThumbYZoom);
                drawXLabels(g, thumbArea);
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
            if ((juce::ModifierKeys::currentModifiers.isCtrlDown())|| (juce::ModifierKeys::currentModifiers.isAltDown())) //Y Zoom
            {
                auto WheelDelta = wheel.deltaY;
                if (WheelDelta > 0)
                {
                    if (YZoomIndex < 96)
                    {
                        YZoomIndex++;
                        ThumbYZoom = getZoomMult(YZoomIndex);
                        
                        repaint();
                    }
                }
                else
                {
                    if (YZoomIndex > 0)
                    {
                        YZoomIndex--;
                        ThumbYZoom = getZoomMult(YZoomIndex);
                        repaint();
                    }
                }
                DBG("mouseWh:YZoomIndex = " << YZoomIndex << " ThumbYZoom = " << ThumbYZoom);
            }
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
    double AmpdBGainToMultFactor(double AmpGaindB)
// called at init
    {
        double fact;
        fact = pow(10, AmpGaindB / 20.0);
        return fact;

    }
//-------------------------------------------------------------------------------------
private:
    AudioFormatManager formatManager;
    AudioThumbnailCache thumbnailCache  { 10 };
    AudioThumbnail thumbnail            { 1, formatManager, thumbnailCache };

    double sampleRate = 0.0;

    bool displayFullThumb = false;
    int displayThumbMode;

    double ThumbXZoom = 1.0f;
    int XZoomIndex = 0;
    double stepSize = 0; //stores the graduation step size (usually in s)
    std::vector<double> zoomVector;

    juce::ScrollBar scrollbar{ false };
    juce::Range<double> visibleRange;

    double ThumbYZoom = 1.0f;
    int YZoomIndex = 0;
    const double AmpZoomGainStepdB = 1.5; //step in dB of each MouseWheel click
    double AmpZoomGainFactor = AmpdBGainToMultFactor(AmpZoomGainStepdB);

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
