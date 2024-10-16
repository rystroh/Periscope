#pragma once

#include <vector>
#include <algorithm>
#include "..\grape\source\grape.h"
#include "PopUpMenu.h"
#include "Enums.h"
    //=====================================================================================
    //class RecordingThumbnail : public juce::Component, 
class RecordingThumbnail : public grape::Panel,
    private juce::ChangeListener,
    private juce::ScrollBar::Listener,
    public juce::ChangeBroadcaster
{
public:
     RecordingThumbnail(const juce::String& id, grape::Panel* trigger_settings) : Panel(id)
    {
        triggerSettings = trigger_settings;
        addAndMakeVisible(scrollbar);
        scrollbar.setRangeLimits(visibleRange);
        scrollbar.setAutoHide(true);
        scrollbar.addListener(this);
        formatManager.registerBasicFormats();
        thumbnail.addChangeListener(this);

        /*           juce::String chanTxt;
                   chanTxt << "Channel " << chanID;*/

        setWidth(300, 600, 10000);
        setHeight(50, 100, 500);
    }

    ~RecordingThumbnail() override
    {
        scrollbar.removeListener(this);
        thumbnail.removeChangeListener(this);
    }

    juce::AudioThumbnail& getAudioThumbnail() { return thumbnail; }

    juce::Colour testColour = juce::Colours::antiquewhite;
    juce::Colour wavFormColour = juce::Colour(0xff43d996);
    juce::Colour wavClippingColour = juce::Colour(0xffff0000);
    juce::Colour wavBackgroundColour = juce::Colours::black;
    juce::Colour digitPanelColour = juce::Colour(0xff232323);
    juce::Colour digitColour = juce::Colour(0xff8a8a8a);
    juce::Colour gridColour = juce::Colour(0xff8a8a8a);
    juce::Colour gridHorizontalCenterColour = juce::Colours::red;
    juce::Colour triggerColour = juce::Colours::yellow;
    juce::Colour backGroundColour = juce::Colour(0xff2e2e2e);
    double gridOpacity = 0.5; //grid opacity
    double triggerOpacity = 0.75; //trigger lines opacity

    int spareWidth = 0;//width of zone used to display eScope controls 

    int yScaleZoneWidth = 50;
    float viewSize = 0.1;// viewing window size
    int chanID = -1; //copy of eScope ID at Thumbnail level so Listener can retrieve info
    std::vector<float> mAudioPoints;
    std::vector<float> mMaxAudioPoints;
    std::vector<float> mMinAudioPoints;
    juce::Point< int > Posi3;

    //-------------------------------------------------------------------
    //following elements are passed between RecTumbnail and AudioRecorder
    bool bTriggered = false;
    juce::AudioBuffer<float>* eBuffer;
    unsigned long* wfStartAddr;
    unsigned long* wfTriggAddr;
    bool* bBufferReady;
    bool* bBufferUnderRun;
    //------------------------------
    bool repaintBroadcasted = false;
    //----------------------------------------------------------------------------------
    bool* getTriggeredPtr(void) { return &bTriggered; }
    //----------------------------------------------------------------------------------
    void setBufferedToImage(juce::AudioBuffer<float>* recBuffer) { eBuffer = recBuffer; }
    //called by eScope setViewSize
    //----------------------------------------------------------------------------------
    void setBufferStartAddress(unsigned long* addr) { wfStartAddr = addr; }
    //----------------------------------------------------------------------------------
    void setBufferTriggAddress(unsigned long* addr) { wfTriggAddr = addr; }
    //----------------------------------------------------------------------------------
    void setBufferReadyAddress(bool* addr) { bBufferReady = addr; }
    //----------------------------------------------------------------------------------
    void setBufferUnderRunAddress(bool* addr) { bBufferUnderRun = addr; }
    //----------------------------------------------------------------------------------
    void setViewSize(float dispTime)// sets viewing window size in secondes in oscillo mode
    {
        viewSize = dispTime;
    }
    //----------------------------------------------------------------------------------
    void setYScale(verticalScale scale) { yScale = scale; }
    //----------------------------------------------------------------------------------
    bool setSource(juce::InputSource* newSource) { return(thumbnail.setSource(newSource)); }
    //----------------------------------------------------------------------------------
    void prepareToPlay(int smpPerBlockExpected, double smpRate)
    {
        sampleRate = smpRate;
        samplesPerBlockExpected = smpPerBlockExpected;
    }
    //----------------------------------------------------------------------------------
    void setSampleRate(double smpRate) { sampleRate = smpRate; }

        //----------------------------------------------------------------------------------
        void setDisplayFullThumbnail(bool displayFull)
        {
            displayFullThumb = displayFull;
            if (displayFull)
            {
                auto thumbnailsize = thumbnail.getTotalLength();
                juce::Range<double> newRange(0.0, thumbnailsize);
                scrollbar.setRangeLimits(newRange);
                setRange(newRange);
            }
            else
                repaint();
        }
        //----------------------------------------------------------------------------------
        void setDisplayThumbnailMode(int displayMode)  { displayThumbMode = displayMode; }
        //----------------------------------------------------------------------------------
        void setDisplayYZoom(double yZoom) // called by SliderValueChanged in Mainjuce::Component.h
        {
            ThumbYZoom = yZoom;
            if (yZoom == 1.0)
                YZoomIndex = 8; // YZoomIndex = 0 <-> gain =+12dB 
                                // YZoomIndex = 8 <-> gain = 0dB
            DBG("Y Zoom = " << ThumbYZoom);
            repaint();
        }
        //----------------------------------------------------------------------------------
        double getDisplayYZoom() { return(ThumbYZoom); }
        //----------------------------------------------------------------------------------
        void setRange(juce::Range<double> newRange)
        {
            visibleRange = newRange;
            scrollbar.setCurrentRange(visibleRange);
            repaint();
        }
        //----------------------------------------------------------------------------------
        juce::Range<double> getEScopeVisibleRange(int index,double timeOffset)
        {
            auto thumbnailsize = thumbnail.getTotalLength();
            double currentlength = getSampleSize();
            return(visibleRange);
        }
        //------------------------------------------------------------------------------
        double getTimeStepSize(int displayWidthPix, double wavDurationToDisplaySec)
        {
            std::vector<float>NiceTimeRatios{ 1.0 , 2.0 , 5.0 , 10.0 };
            const double minRectWidth{ 128.0 };
            // first pass determin the magnitude range
           // DBG("getTimeStepSize::displayWidthPix = " << wavDurationToDisplaySec 
            //<< " displayWidthPix = " << displayWidthPix);
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
        //------------------------------------------------------------------------------
        std::vector<double> getTimeLineX()  {        }
        //------------------------------------------------------------------------------
        std::vector<float>  getTimeLabels() {        }
        //----------------------------------------------------------------------------------
        void paintCentralHorizontalRedLine(juce::Graphics& g, const juce::Rectangle<int>& bounds)
        {
            g.setColour(gridHorizontalCenterColour);//Draw middle horizontal line
            g.setOpacity(gridOpacity);
            g.drawHorizontalLine(bounds.getCentreY(), bounds.getX(), bounds.getRight());
        }
        //----------------------------------------------------------------------------------
        void paintTriggerTimeAndLevel(juce::Graphics& g, const juce::Rectangle<int>& bounds)
        {
            int trigTime, trigY, trigX;
            auto top = bounds.getY(); //bounds = render area
            auto bottom = bounds.getBottom();
            auto left = bounds.getX();
            auto right = bounds.getRight();
            auto thumbh = bounds.getHeight();
            auto width = bounds.getWidth(); // width of Display zone in pixels

            auto totlen = getSampleSize();
            double SampleSize = totlen * sampleRate; //size  of sample in points
            double Ratio = SampleSize / (double)width;
            auto visRangeWidth = visibleRange.getLength();
            double curRatio = Ratio / totlen * (double)visibleRange.getLength();
            double tRatio = 0.5;
            float center_t = totlen * tRatio;// timeToX(visRangeWidth * tRatio);  //+ txtZoneOffset; // timeToX(viewSize * 0.5); get time center
            int centerX = timeToX(visRangeWidth * tRatio); // timeToX(viewSize * 0.5); get time center
            double dTrigTime = visRangeWidth * tRatio;
            double xOffset = width / 2.0;
            xOffset = xOffset * Ratio / curRatio;

            double thresholdTrigger = triggerSettings->getControlValue("Threshold");
            int RecTrigMode = triggerSettings->getControlValue("Condition");
            bool trigEnabled = triggerSettings->getControlValue("Enable") && ((int)triggerSettings->getControlValue("Channel") == chanID); // Trigger enabled + matching channel;

            // Draw Trigger Vertical and Horizontal
            if((RecTrigMode == Clipping) && (trigEnabled))
                g.setColour(wavClippingColour);//Trigger on clipping condition is in Red
            else
                g.setColour(triggerColour);//Draw middle horizontal line
            
            g.setOpacity(triggerOpacity);
            trigX = timeToX(viewSize * 0.5);
            trigTime = currentOffset + trigX;//+left; // get time center

            trigX = timeToX(0);
            dTrigTime = trigX + xOffset;
            trigTime = (int)dTrigTime;

            if ((trigTime >= left) && (trigTime <= right))
            {
                g.drawVerticalLine(trigTime, top, bottom);
                if (visibleRange != visibleRangePrevious)
                    DBG("paintTrigTime::viewSize = " << viewSize << " trigX = "<< trigX << " trigTime = " << trigTime);
            }
            float trigLevel = bounds.getCentreY() - (float)thumbh * 0.5 * thresholdTrigger * ThumbYZoom;
            trigY = (int)trigLevel;
            
            if ((trigY >= top) && (trigY <= bottom)&& (trigEnabled))
                if(RecTrigMode != Clipping)
                    g.drawHorizontalLine(trigY, left, right);// only draw if in display area
        }
        //----------------------------------------------------------------------------------
        void paintTimeLines(juce::Graphics& g, const juce::Rectangle<int>& gridBounds, const juce::Rectangle<int>& labelBounds, horizontalScale scale)//(g, wavZone, xScale);
        {
            if(scale == Absolute)
            {
                paintVerticalGrid(g, gridBounds);
                drawXLabels(g, labelBounds);
            }
            else if(scale == RelativeToTrigger)
            {
                paintTimeGridLin(g, gridBounds);
                drawXLabelsOffset(g, labelBounds, viewSize * 0.5);
            }
            else
            {
                DBG("paintTimeLines::Error: xScale = " << scale);
            }

        }
        //----------------------------------------------------------------------------------
        void paintTimeGridLin(juce::Graphics& g, const juce::Rectangle<int>& bounds)
        {
            auto totlen = getSampleSize(); // thumbnail.getTotalLength(); //total length of sample in seconds
            if (totlen > 0)
            {
                double displayStartTime, displayEndTime, displayWidth;

                auto renderArea = bounds;
                auto top = bounds.getY();
                auto bottom = bounds.getBottom();
                auto left = bounds.getX();
                auto right = bounds.getRight();
                auto width = bounds.getWidth(); // width of Display zone in pixels

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
                g.setColour(gridColour);
                g.setOpacity(gridOpacity);
                int newX1, txtZoneOffset;
                txtZoneOffset = renderArea.getTopLeft().getX();
                txtZoneOffset = spareWidth;
                if (visibleRange != visibleRangePrevious)
                    DBG("paintGrid::txtZoneOffset = " << txtZoneOffset);
                double tRatio = 0.5;
                float center_t = totlen * tRatio;// timeToX(visRangeWidth * tRatio);  //+ txtZoneOffset; // timeToX(viewSize * 0.5); get time center
                int centerX = timeToX(visRangeWidth * tRatio); // timeToX(viewSize * 0.5); get time center
                double trigTime = visRangeWidth * tRatio;
                double xOffset = width / 2.0;
                xOffset = xOffset * Ratio / curRatio;
                if (visibleRange != visibleRangePrevious)
                    DBG("paintGrid::xOffset = " << xOffset <<" center_t = " << center_t);
                if (visibleRange != visibleRangePrevious)
                    DBG("paintGrid::visibleRange = " << visibleRange.getStart()<<" End = "<< visibleRange.getEnd());
                std::vector<double> xs;
                std::vector<long> xc;
                double xx = 0;
                if (visibleRange != visibleRangePrevious)
                    DBG("paintGrid: chanID = " << chanID << " XZoomIndex = " << XZoomIndex);
                if (XZoomIndex == 0)
                {
                    //xs = getXsCentered(trigTime); //xs = getXsCentered(viewSize * 0.5); //create vector with nice positions for vert
                    xs = getXsRatio(tRatio);
                    // draw vertical time lines
                    for (auto x : xs)
                    {
                        newX1 = centerX + timeToX(x); // get 
                        g.drawVerticalLine(newX1, top, bottom);
                        if (visibleRange != visibleRangePrevious)
                            DBG("paintGrid0: x = " << x << " xx = " << xx << " newX1 = " << newX1);
                    }
                    xc = getXpRatio(tRatio, width);
                }
                else
                {
                    //xs = getXs(); //create vector with nice positions for vert lines
                    xs = getXsRatio(tRatio);
                    for (auto x : xs)
                    {
                        newX1 = timeToX(x); // get 
                        newX1 += xOffset;
                        g.drawVerticalLine(newX1, top, bottom);
                        if (visibleRange != visibleRangePrevious)
                            DBG("paintGridi: x = " << x << " xx = " << xx << " newX1 = " << newX1);
                    }
                    xc = getXpRatio(tRatio, width);
                }
                paintCentralHorizontalRedLine(g, bounds); // draw Red horizontal venter line            
                paintTriggerTimeAndLevel(g, bounds);  // Draw Trigger Vertical and Horizontal
            }            
        }
        //----------------------------------------------------------------------------------
        void paintVerticalGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
        {
            auto top = bounds.getY();
            auto bottom = bounds.getBottom();
            auto width = bounds.getWidth(); // width of Display zone in pixels
            auto visRangeWidth = visibleRange.getLength();

            double newstepSize = getTimeStepSize(width, (double)visRangeWidth);
            if (stepSize != newstepSize)
            {
                stepSize = newstepSize;
                //DBG("paintGrid::stepSize = " << stepSize);
            }
            std::vector<double> xs = getXs(); //create vector with nice positions for vert lines

            int newX1;
            // draw vertical time lines
            g.setColour(gridColour);
            g.setOpacity(gridOpacity);
            for (auto x : xs)
            {
                newX1 = timeToX(x); // get 
                //g.drawVerticalLine(newX1, top, bottom);
                g.drawVerticalLine(newX1, bounds.getY(), bounds.getBottom());
            }            
        }
        //----------------------------------------------------------------------------------
        void paintHorizontalGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds, int mode)
        {
            // draw horizontal Level lines
            std::vector<int> NiceGainVect;
            std::vector<int> NiceGainY;
            int ret;
            switch (mode)
            {
                case Linear: //0: // linear
                    ret = getNiceGainVectLin(bounds.getHeight(), NiceGainVect, NiceGainY);
                    break;
                case dB: //1: // in dB
                    ret = getNiceGainVect(bounds.getHeight(), NiceGainVect, NiceGainY);
                    break;
                default: // linear
                    break;
            }

            int centralY, newY41, newY42;
            auto left = bounds.getX();
            auto right = bounds.getRight();

            paintCentralHorizontalRedLine(g, bounds); // draw Red horizontal venter line          

            // draw all other horizontal lines
            g.setColour(gridColour);
            g.setOpacity(gridOpacity);
            centralY = bounds.getCentreY();
            //DBG("paintHGrid:: Y2 = " << newY2);
            for (auto y : NiceGainY)
            {
                newY41 = centralY - y;
                g.drawHorizontalLine(newY41, left, right);
                //DBG("paintHGrid:: Y41 = " << newY41);
                newY42 = centralY + y;
                g.drawHorizontalLine(newY42, left, right);
                //DBG("paintGrid:: Y41 = " << newY41<< " Y42 = " << newY42);
            }
        }
        //----------------------------------------------------------------------------------
        void drawBuffer(juce::Graphics& g, const juce::Rectangle<int>& bounds,
            double startTimeSeconds, double endTimeSeconds, float verticalZoomFactor
        )
        {
            g.setColour(wavFormColour);

            if (startTimeSeconds < 0)
                startTimeSeconds = 0;//catch stupid conditions
                        
            mAudioPoints.clear();

            mMaxAudioPoints.clear();
            mMinAudioPoints.clear();

            double wavPoint,wavMin,wavMax;
            // access to pointers set by the AudioRecorder part
            unsigned long* ptrStart;
            ptrStart = wfStartAddr;
            unsigned long* ptrTrig;
            ptrTrig = wfTriggAddr;
            auto ptNb = eBuffer->getNumSamples();

            juce::AudioBuffer<float> waveform(1,ptNb);
            unsigned long idxDest = 0;
            unsigned long wavCount = 0;
            float wavValue;
            for (unsigned long idx = *ptrStart; idx < ptNb; idx++)
            {
                wavValue = eBuffer->getSample(0, idx);
                waveform.setSample(0, idxDest++, wavValue);
                wavCount++;
            }
            for (unsigned long idx = 0; idx < *ptrStart; idx++)
            {
                wavValue = eBuffer->getSample(0, idx);
                waveform.setSample(0, idxDest++, wavValue);
                wavCount++;
            }

            //DBG("drawBuffer Nb of Point Copied = " << wavCount);            

            auto top = bounds.getY();
            auto bottom = bounds.getBottom();
            auto left = bounds.getX();
            auto right = bounds.getRight();
            auto width = bounds.getWidth(); // width of Display zone in pixels

            unsigned long startSample = sampleRate * startTimeSeconds;
            unsigned long endSample = sampleRate * endTimeSeconds;

            if (endSample > ptNb)
                endSample = ptNb; //catch stupid conditions

            ptNb = endSample - startSample;
            double ratio = (double)ptNb / (double)width; //float ratio = (float)ptNb / (float)width;
            double invRatio = (double)width / (double)ptNb;

            unsigned long halfBuffer = eBuffer->getNumSamples() / 2;

            //DBG("ptNb = " << ptNb << " width = " << width << " ratio = " << ratio << " verticalZoomFactor = " << verticalZoomFactor);
            //DBG("drawBuffer: wavCount = " << wavCount << "start = " << startSample << " end = " << endSample << " width = " << width << " ratio = " << ratio << " invratio = " << invRatio);
            int idx,idxEnd;
            int blockID = 0;//for debug
            g.setColour(wavFormColour);

            if (ratio >=1)
            {
                for (float sample = (float)startSample; sample < (float)endSample - ratio; sample += ratio)
                {
                    idx = (int)sample;
                    idxEnd = int(sample + ratio + 0.5);
                    {
                        /*
                        if (visibleRange != visibleRangePrevious)
                            DBG("last block");*/
                    }
                    if (idxEnd > endSample - 1)
                        idxEnd = endSample - 1;
                    //wavMin = eBuffer->getSample(0, idx);
                    wavMin = waveform.getSample(0, idx);
                    if (wavMin > 1)
                        DBG("one point is out = " << wavMin << " " << idx);
                    wavMax = wavMin;
                    while (idx <= idxEnd)
                    {
                        //auto val = eBuffer->getSample(0, idx++);
                        auto val = waveform.getSample(0, idx++);
                        if (val > 1)
                            DBG("one point is out = " << val << " " << idx);
                        if (wavMax < val)
                            wavMax = val;
                        if (wavMin > val)
                            wavMin = val;
                    }
                    mMaxAudioPoints.push_back(wavMax);
                    mMinAudioPoints.push_back(wavMin);
                    blockID++; //for debug
                }
                bool pathStarted = false;
                float pointMinScaled, pointMaxScaled;

                for (int sample = 0; sample < mMinAudioPoints.size(); sample++)
                {
                    pointMinScaled = mMinAudioPoints[sample] * verticalZoomFactor;
                    pointMaxScaled = mMaxAudioPoints[sample] * verticalZoomFactor;
                    if(pointMaxScaled>1)
                        DBG("one point is out = " << pointMinScaled << " " << pointMaxScaled);
                
                    if ((pointMinScaled < -1) && (pointMaxScaled < -1))
                    {
                        DBG("first point is out = " << pointMinScaled << " " << pointMaxScaled);
                    }
                    else if ((pointMinScaled > 1) && (pointMaxScaled > 1))
                    {
                        DBG("first point is out = " << pointMinScaled << " " << pointMaxScaled);
                    }
                    else //first point is in frame                 
                    {
                        if (pointMinScaled < -1)
                            pointMinScaled = -1;
                        if (pointMaxScaled > 1)
                            pointMaxScaled = 1;
                        auto wavbottom = juce::jmap<float>(pointMaxScaled, -1.0f, 1.0f, bottom, top);
                        auto wavtop = juce::jmap<float>(pointMinScaled, -1.0f, 1.0f, bottom, top);
                        if (std::abs(wavtop - wavbottom)< 1.0f)
                            wavbottom = wavtop - 1;//set thickness to 1 at least*/
                        /*
                        if (wavbottom - wavtop < 1)
                            wavbottom = wavtop + 1; //set thickness to 1 at least*/
                        //g.drawVerticalLine(sample, wavtop, wavbottom);
                        g.drawVerticalLine(sample, wavbottom, wavtop);
                        pathStarted = true;
                    }
                }
                

                if ((*ptrTrig < halfBuffer) && *bBufferUnderRun)
                {
                    unsigned long invalidDataAddr = halfBuffer - *ptrTrig;
                    if (invalidDataAddr > startSample)
                    {
                        juce::Rectangle <int> invalidDataRect;
                        float rectLength = float(invalidDataAddr - startSample) / ratio;
                        int redlineY = bounds.getCentreY() / 2;
                        int rectY = redlineY - bounds.getCentreY() / 4;
                        invalidDataRect.setX(bounds.getX());
                        invalidDataRect.setY(bounds.getY());
                        invalidDataRect.setWidth(bounds.getX() + rectLength);
                        invalidDataRect.setHeight(bounds.getHeight());
                        g.setColour(wavFormColour);
                        g.setOpacity(0.75);
                        g.drawRect(invalidDataRect,1.0);
                        g.setOpacity(0.25);
                        g.fillRect(invalidDataRect);
                    }
                    
                }
            } 
            else // more than 1 pixel per wav sample double ratio = (double)ptNb / (double)width;
            {
                ptNb = endSample - startSample;
                ratio = (double)ptNb / (double)width; //float ratio = (float)ptNb / (float)width;
                unsigned int iInvertedRatio;
                iInvertedRatio = (int)(invRatio + 0.5);
                //ratio = (double)1.0 / (double)iInvertedRatio;
                /*
                if (startSample + ptNb * iInvertedRatio > endSample)
                {
                    DBG("shit will hit the fan ! ");
                    //return;
                }*/
            //    DBG("start = " << startSample << " end = " << endSample << " width = " << width << " ratio = " << ratio << " invratio = " << invRatio);

                juce::Rectangle<int> wavPt;
                int pointSize = 4;
                wavPt.setHeight(pointSize);
                wavPt.setWidth(pointSize);

                auto pointScaled = waveform.getSample(0, startSample) * verticalZoomFactor;
                if ((pointScaled < -1) || (pointScaled > 1)) //check if point is still within limits
                    DBG("first point is out = " << pointScaled);
                else
                    wavPt.setCentre(left, pointScaled);

                for (int idx = 1; idx < endSample - startSample; idx++)
                {
                    pointScaled = waveform.getSample(0, startSample+idx) * verticalZoomFactor;
                    //wavPt.setCentre()
                }
                long smpCount = 0;
                double sample; //declared outside of the loop for easiser debugging
                //for (sample = (double)startSample; (int)sample < int((double)endSample-1 - ratio); sample += ratio)
                for (sample = (double)startSample; smpCount< width; sample += ratio)
                {
                    idx = (int)sample;
                    //wavPoint = eBuffer->getSample(0, idx);
                    wavPoint = waveform.getSample(0, idx);
                    mAudioPoints.push_back(wavPoint);
                    smpCount++;
                    if (smpCount> width)
                        DBG("shit will hit the fan again! ");
                }
                if (mAudioPoints.size() > right )
                    DBG("mAudioPoints.size too big " << mAudioPoints.size() << " ratio = " << ratio << " start =  " << startSample << "x limit = " << right);
                bool pathStarted = false;

                pointScaled = mAudioPoints[0] * verticalZoomFactor;
                juce::Path path;
                path.clear();
                if ((pointScaled < -1) || (pointScaled > 1)) //check if point is still within limits
                    DBG("first point is out = " << pointScaled);
                else
                {
                    auto point = juce::jmap<float>(pointScaled, -1.0f, 1.0f, bottom, top);
                    path.startNewSubPath(0, point);
                    pathStarted = true;
                }

                for(int sample = 1;sample< mAudioPoints.size();sample++)
                {
                    pointScaled = mAudioPoints[sample] * verticalZoomFactor;
                    if ((pointScaled < -1) || (pointScaled > 1)) //check if point is still within limits
                        DBG("point with index " << sample << " is out = " << pointScaled);
                    else
                    {
                        auto point = juce::jmap<float>(pointScaled, -1.0f, 1.0f, bottom, top);
                        if (sample > right)
                            DBG("point with index " << sample << " is out of X range = " << right);
                        if (!pathStarted)
                        {
                            path.startNewSubPath(sample, point);
                            pathStarted = true;
                        }
                        else
                            path.lineTo(sample, point);
                    }                
                }
                g.strokePath(path, juce::PathStrokeType(1));
                
                if (*ptrTrig < halfBuffer)
                {
                    unsigned long invalidDataAddr = halfBuffer - *ptrTrig;
                    if (invalidDataAddr > startSample)
                    {
                        juce::Rectangle <int> invalidDataRect;
                        float rectLength = float(invalidDataAddr - startSample) / ratio;
                        int redlineY = bounds.getCentreY() / 2;
                        int rectY = redlineY - bounds.getCentreY() / 4;
                        invalidDataRect.setX(bounds.getX());
                        invalidDataRect.setY(bounds.getY());
                        invalidDataRect.setWidth(bounds.getX() + rectLength);
                        invalidDataRect.setHeight(bounds.getHeight());
                        g.setColour(wavFormColour);
                        g.setOpacity(0.75);
                        g.drawRect(invalidDataRect, 1.0);
                        g.setOpacity(0.25);
                        g.fillRect(invalidDataRect);
                    }
                }
            }

            // draw frame around WavZone
            g.setColour(testColour);
            g.setOpacity(gridOpacity);
            g.drawHorizontalLine(top, left, right);
            g.drawHorizontalLine(bottom, left, right);
            g.drawVerticalLine(left, top, bottom);
            g.drawVerticalLine(right, top, bottom);
            /* debug stuff to make sure we are drawing stuff on the display)
            float nwx1, nwx2, nwy1, nwy2;
            nwx1 = 2 * (right - left) / 3;
            nwy1 = top + 0.25 * (bottom-top);
            nwy2 = top + 0.75 * (bottom - top);
            g.drawVerticalLine((int)nwx1, nwy1, nwy2);
            //g.drawLine(left, bottom, left, top); 
            //g.drawLine(left, bottom, right, top);// diagonals for debug 
            //g.drawLine(left, top, right, bottom);// diagonals for debug
        */
        }
        //----------------------------------------------------------------------------------
        void drawXLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds)
        {
            g.setColour(gridColour);
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
                juce::String str;
                newX1 = timeToX(x); // get
                str << x;
                str.toDecimalStringWithSignificantFigures(x, 2);
                juce::Rectangle<int> r;
                auto textWidth = g.getCurrentFont().getStringWidth(str);
                r.setSize(textWidth, fontHeight);
                if (newX1 + (double)textWidth / 2.0 > right)
                    r.setCentre(static_cast<int>((newX1)-(double)textWidth / 2.0), 0);
                else
                    r.setCentre(static_cast<int>(newX1), 0);

                r.setY(textArea.getY());
                g.drawFittedText(str, r, juce::Justification::centred, 1);
            }
        }
        //----------------------------------------------------------------------------------
        void drawXLabelsOffset(juce::Graphics& g, const juce::Rectangle<int>& bounds, double timeOffset)
        {
            g.setColour(gridColour);
            g.setOpacity(1.0);
            const int fontHeight = 10;
            g.setFont(fontHeight);
            auto textArea = getHTextZone(bounds);// getRenderZone(bounds);
            auto wavZone = getWaveZone(bounds);
            auto left = textArea.getX();
            auto top = textArea.getY();
            auto right = left + textArea.getWidth();
            auto txtWidth = textArea.getWidth(); ; // bounds.getWidth();
            auto xCenter = txtWidth / 2.0;
            auto timeZoneHalf = visibleRange.getLength() / 2;
            auto width = wavZone.getWidth();

            //std::vector<double> xs = getXs();
            double tRatio = 0.5;
            std::vector<double> xs = getXsRatio(tRatio);//getXsCentered(txtWidth * 0.5); //create vector with nice positions for vert
            std::vector<long> xc = getXpRatio(tRatio, width);
            int newX1;
            for (int idx = 0; idx < xs.size(); idx++)//for (auto x : xs)
            {
                juce::String str;
                newX1 = xc[idx];
                str << xs[idx];
                str.toDecimalStringWithSignificantFigures(xs[idx], 2);
                juce::Rectangle<int> r;
                auto textWidth = g.getCurrentFont().getStringWidth(str);
                r.setSize(textWidth, fontHeight);
                if (newX1 + (double)textWidth / 2.0 > right)
                    r.setCentre(static_cast<int>((newX1)-(double)textWidth / 2.0), 0);
                else
                    r.setCentre(static_cast<int>(newX1), 0);

                r.setY(textArea.getY());
                g.drawFittedText(str, r, juce::Justification::centred, 1);
            }
        }
        //----------------------------------------------------------------------------------
        void drawInfo(juce::Graphics& g, const juce::Rectangle<int>& bounds) // for debug only
        {
            g.setColour(gridColour);
            g.setOpacity(1.0);
            const int fontHeight = 10;
            g.setFont(fontHeight);
            juce::Rectangle<int> wavZone = getWaveZone(bounds);
            juce::String str;
            auto totlen = getSampleSize();
            double SampleSize = totlen * sampleRate; //size  of sample in points
            str << "SR = " << sampleRate << " Hz";
            auto txtWidth = g.getCurrentFont().getStringWidth(str);
            juce::Rectangle<int> rTxt;
            int left, top, right, butt;
            auto maxright = wavZone.getRight();
            butt = wavZone.getBottom() - 20;
            left = maxright - txtWidth - 10;
            rTxt.setLeft(left);
            rTxt.setY(butt);
            rTxt.setSize(txtWidth, fontHeight);
            g.drawFittedText(str, rTxt, juce::Justification::centredLeft, 1);

        }
        //----------------------------------------------------------------------------------
        void drawYLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds, int mode)
        {
            g.setColour(gridColour);
            g.setOpacity(1.0);
            const int fontHeight = 10;
            g.setFont(fontHeight);
            auto textArea = getVTextZone(bounds);// getRenderZone(bounds);
                                                 // there is a mistake here TBD
            juce::Rectangle<int> wavZone = getWaveZone(bounds);

            auto left = textArea.getX() + 10;
            auto top = wavZone.getTopRight().getY();
            auto maxright = textArea.getRight();
            auto right = left + textArea.getWidth();

            juce::Rectangle<int> rTxt;
            juce::String strTxt, yUnits;
            strTxt << "-144 dB";
            auto txtWidth = g.getCurrentFont().getStringWidth(strTxt);
            rTxt.setSize(txtWidth, fontHeight);

            //std::vector<double> xs = getXs();
            std::vector<int> NiceGainVect;
            std::vector<int> NiceGainY;
            int ret ; //= getNiceGainVect(wavZone.getHeight(), NiceGainVect, NiceGainY);

            switch (mode)
            {
            case Linear://0: // linear
                ret = getNiceGainVectLin(textArea.getHeight(), NiceGainVect, NiceGainY);
                yUnits << " %";
                break;
            case dB: //1: // in dB
                ret = getNiceGainVect(textArea.getHeight(), NiceGainVect, NiceGainY);
                yUnits << " dB";
                break;
            default: // linear
                ret = getNiceGainVectLin(textArea.getHeight(), NiceGainVect, NiceGainY);
                yUnits << " %";
                break;
            }
            
            int centralY, newY41, newY42;
            int newY1, newGain;

            centralY = wavZone.getCentreY();
            //DBG("drawYLabels:: Y2 = " << newY2);

            for (int idx = 0; idx < NiceGainY.size(); idx++)
            {
                juce::String str, strm;
                newY1 = centralY - NiceGainY[idx] - fontHeight / 2.0;
                newY42 = centralY + NiceGainY[idx] - fontHeight / 2.0;
                newGain = NiceGainVect[idx]; // get value to be displayed
                str << newGain << yUnits;
                if (mode == 0)
                {
                    strm << "-" << newGain << yUnits;
                }
                else
                {
                    strm << newGain << yUnits;
                }
                str.toDecimalStringWithSignificantFigures(newGain, 2);
                strm.toDecimalStringWithSignificantFigures(newGain, 2);

                juce::Rectangle<int> r,rm;
                auto textWidth = g.getCurrentFont().getStringWidth(str);
                auto textWidthm = g.getCurrentFont().getStringWidth(strm);
                int left, top, right, butt;

                left = maxright - textWidth - 10;

                r.setLeft(left);
                r.setY(newY1);
                r.setSize(textWidth, fontHeight);

                rm.setLeft(left);
                rm.setY(newY42);
                rm.setSize(textWidth, fontHeight);
                //            r.setY(textArea.getY());
                g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
                if (NiceGainY[idx] > 0)
                    g.drawFittedText(strm, rm, juce::Justification::centredLeft, 1);

                newY1 = centralY - NiceGainY[idx];
                newY42 = centralY + NiceGainY[idx];
                left = textArea.getTopLeft().getX();
                g.drawHorizontalLine(newY1, left - 3, left);
                if(NiceGainY[idx]>0)
                    g.drawHorizontalLine(newY42, left - 3, left);
            //    DBG("drawYLabels:: Y1 = " << newY1);
            }
        }
        //----------------------------------------------------------------------------------
        double round_fl(double x, int num_decimal_precision_digits)
        {
            double power_of_10 = std::pow(10, num_decimal_precision_digits);
            double xr = std::round(x * power_of_10);
            xr = xr / power_of_10;
            return xr;
        }
        //----------------------------------------------------------------------------------
        std::vector<double> getXs()
        {
            auto timeZoneStart = visibleRange.getStart();
            auto timeZoneLen = visibleRange.getEnd();           

            std::vector<double> xs,xi;
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
        //----------------------------------------------------------------------------------
        std::vector<double> getXsCentered(double center)
        {
            auto timeZoneStart = visibleRange.getStart();
            auto timeZoneLen = visibleRange.getEnd();
            //auto timeZoneHalf = visibleRange.getLength() / 2;

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
            xs.push_back(0.0); //start by pushing center value
            x1 = floor(timeZoneStart / stepSize) * stepSize + stepSize;
            x1 = round_fl(x1, precision);
            while (x1 <= center)//(x1 <= timeZoneHalf)
            {
                xs.push_back(x1);
                xs.push_back(-x1);
                x1 += stepSize;
                x1 = round_fl(x1, precision);
            }
            std::sort(xs.begin(), xs.end());
            //std::sort(xs.begin(), xs.end(), std::smaller());// greater<double>());
            return(xs);
        }
        //----------------------------------------------------------------------------------
        std::vector<double> getXsRatio(double ratio)
        {
            auto timeZoneStart = visibleRange.getStart();
            auto timeZoneEnd = visibleRange.getEnd();
            double currentlength = getSampleSize();
            double midTime = currentlength * ratio;  //(visibleRange.getStart() + visibleRange.getEnd())* ratio;
            double timeZoneBefore = timeZoneStart - midTime;//visibleRange.getLength() * ratio;
            double timeZoneAfter = timeZoneEnd - midTime;//visibleRange.getLength() - timeZoneBefore;

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
            //xs.push_back(0.0); //start by pushing center value
            x1 = 0;
            while (x1 >= timeZoneBefore)//(x1 <= timeZoneHalf)
            {
                xs.push_back(x1);
                x1 -= stepSize;
                x1 = round_fl(x1, precision);
            }
            x1 = 0;
            while (x1 <= timeZoneAfter )//(x1 <= timeZoneHalf)
            {               
                xs.push_back(x1);
                x1 += stepSize;
                x1 = round_fl(x1, precision);
            }
            std::sort(xs.begin(), xs.end());
            //std::sort(xs.begin(), xs.end(), std::smaller());// greater<double>());
            xs.erase(std::unique(xs.begin(), xs.end()), xs.end());//erase doubles 
            //sort(vec.begin(), vec.end());
            //vec.erase(unique(vec.begin(), vec.end()), vec.end());
            return(xs);
        }
        //----------------------------------------------------------------------------------
        std::vector<long> getXpRatio(double ratio, long width) //generate vector of positions in pixels
        {
            //auto width = bounds.getWidth(); // width of Display zone in pixels
            auto totlen = getSampleSize();
            double SampleSize = totlen * sampleRate; //size  of sample in points
            double Ratio = SampleSize / (double)width;
            double curRatio = Ratio / totlen * (double)visibleRange.getLength();
            double tRatio = 0.5;
            double xOffset = width / 2.0;
            xOffset = xOffset * Ratio / curRatio;

            long newX1;
            std::vector<double> xs;
            std::vector<long> xp;
            //DBG("GetXpRatio: ---------- ");
            xs = getXsRatio(tRatio);
            for (auto x : xs)
            {
                newX1 = timeToX(x); // get 
                newX1 += xOffset;
                xp.push_back(newX1);
                //DBG("GetXpRatio: " << newX1);
            }
            return(xp);
        }
        //----------------------------------------------------------------------------------
        std::vector<int> getGains()//called by std::vector<int>getNiceGainVect
    // creates vector of gains in dB between +12 and -144 - step = 1dB
        {
            std::vector<int> gaindB;
            for (int x = 12; x > -144; x--)
                gaindB.push_back(x); // xs.push_back(x1);
            return(gaindB);
        }
        //----------------------------------------------------------------------------------
        std::vector<long double> getZoomGainVect()
            // creates vector of gains in dB between + 12 and -144 - step = 1dB
        {
            std::vector<long double> gainZdB;
            const long double dBStep = 1.5;
            for (int x = 0; x < 104; x++)
                gainZdB.push_back(pow(10, (long double)(x - 8) * (long double)1.5 / (long double)20.0));
            return(gainZdB);
        }
        //----------------------------------------------------------------------------------
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
        //----------------------------------------------------------------------------------
        int getNiceGainVect(int displayHeightPix, std::vector<int>& NiceGainVect,
            std::vector<int>& NiceGainY)
        {
            const double minRectHeight{ 15.0 }; //nb of pixel min between horizontal lines
            //std::vector<int> gaindB = getGains();
            //std::vector<long double> gainMult = getZoomGainVect();
            //std::vector<int> NiceGainVect;
            //std::vector<int> NiceGainY;
            //double previousYdB = 0;
            //double previousYpix = 0;
            //auto curYZoom = ThumbYZoom;
            int curYZoomIndex = YZoomIndex;
            const long double dBStep = 1.5;
            double topGdB = 12.0 - curYZoomIndex * dBStep;

            double halfHeightPix = floor((double)displayHeightPix / 2.0);
            double curY = halfHeightPix;
            double ratio;
            double gain, gain3;
            double NiceY;

            while (curY > 0)
            {
                ratio = curY / halfHeightPix;
                gain = 20.0 * log10(ratio);

                if (round(gain) < -3.0)
                {
                    gain3 = floor(gain / 3.0);
                    gain = gain3 * 3.0;
                }
                else
                {
                    gain = round(gain); //round to the next dB 
                }
                NiceGainVect.push_back(gain + topGdB);
                ratio = pow(10.0, gain / 20);
                NiceY = halfHeightPix * ratio;
                NiceY = round(NiceY);
                NiceGainY.push_back((int)NiceY);
                curY -= minRectHeight;
            }
            return(0);//should never happen
        }
        //----------------------------------------------------------------------------------
        int getNiceGainVectLin(int displayHeightPix, std::vector<int>& NiceGainVect,
            std::vector<int>& NiceGainY)
        {
            const double minRectHeight{ 15.0 }; //nb of pixel min between horizontal lines
            //std::vector<int> gaindB = getGains();
            //std::vector<long double> gainMult = getZoomGainVect();
            //std::vector<int> NiceGainVect;
            //std::vector<int> NiceGainY;
            //double previousYdB = 0;
            //double previousYpix = 0;
            //auto curYZoom = ThumbYZoom;
            int curYZoomIndex = YZoomIndex;
            const long double dBStep = 1.5;
            double topGdB = 12.0 - curYZoomIndex * dBStep;

            double gridRatio, linGain, topLinGain;            
            topLinGain = pow(10.0, topGdB / 20.0);
            //topLinGain = round(topLinGain);
            double halfHeightPix = floor((double)displayHeightPix / 2.0);                       
            double ratio,gain, gain3, NiceY, log2Ratio, linRatio;
            double yStep, perCentStep, yStepNb;

            gridRatio = halfHeightPix / minRectHeight;
            log2Ratio = log2(gridRatio);
            log2Ratio = floor(log2Ratio);
            linRatio = pow(2.0, log2Ratio);
            yStep = halfHeightPix / linRatio;
            perCentStep = 100.0 / linRatio;
            gain = 100.0 * topLinGain;

            double yPercentStep = getAmplitudeStepSize(gain/ gridRatio);
            /*
            while (curY >= 0)
            {
                NiceY = curY;
                NiceY = round(curY);
                NiceGainY.push_back((int)NiceY);
                curY -= yStep;
                NiceGainVect.push_back(gain);
                gain = gain - perCentStep;
            }*/
            int idx = 0;
            float curYPercentAmp = 0.0;
            while (curYPercentAmp <= gain)
            {
                NiceGainVect.push_back(curYPercentAmp);
                curYPercentAmp += yPercentStep;
                idx++;
            }
            if (curYPercentAmp != gain)
            {
                //NiceGainVect.push_back(curYPercentAmp);
                //idx++;
            }
            std::sort(NiceGainVect.begin(), NiceGainVect.end(), std::less());
            double pixelPerStep = halfHeightPix / gain * yPercentStep;
            yStep = halfHeightPix / float(idx-1);
            double curY = 0;
            while (curY <= halfHeightPix)
            {
                NiceY = curY;
                NiceY = round(curY);
                NiceGainY.push_back((int)NiceY);
                curY += pixelPerStep;
            }
            return(0);
        }
        //------------------------------------------------------------------------------
        double getAmplitudeStepSize(double ratio)
        {
            std::vector<float>NicePercentSteps { 1.0 , 2.0 , 2.5 , 5.0 , 10.0 , 12.5,20,25,50,100};
            int i{ 0 };
            if (ratio >= 1.0)
            {
                while (NicePercentSteps[i] < ratio)
                    i++;
                return(NicePercentSteps[i]);
            }
            else
                return(1);//should never happen 
        }
        //----------------------------------------------------------------------------------
        juce::Rectangle<int> getRenderZone(juce::Rectangle<int> bounds)
        {
            //bounds.removeFromTop(12);
            bounds.removeFromBottom(scrollbar.getHeight() + 4);
            return bounds;
        }
        //----------------------------------------------------------------------------------
        juce::Rectangle<int> getHTextZone(juce::Rectangle<int> bounds)
        {
            auto wavRect = getWaveZone(bounds);
            auto left = wavRect.getTopLeft().getX();
            bounds.removeFromTop(4);
            bounds.removeFromBottom(wavRect.getHeight() + 2);
            bounds.setRight(wavRect.getRight());
            bounds.setLeft(left);
            return bounds;
        }
        //----------------------------------------------------------------------------------
        juce::Rectangle<int> getVTextZone(juce::Rectangle<int> bounds)
        {
            juce::Rectangle<int> boundsTxt;
            auto wavRect = getWaveZone(bounds);
            boundsTxt.setLeft(wavRect.getRight() + 4);
            boundsTxt.setTop(wavRect.getTopLeft().getY());
            boundsTxt.setBottom(wavRect.getBottomLeft().getY());
            boundsTxt.setRight(wavRect.getRight() + yScaleZoneWidth);
            return boundsTxt;
        }
        //----------------------------------------------------------------------------------
        juce::Rectangle<int> getWaveZone(juce::Rectangle<int> bounds)
        {
            bounds = getRenderZone(bounds);
            bounds.removeFromTop(16);//make space for time labels
            bounds.removeFromBottom(4);
            bounds.removeFromRight(yScaleZoneWidth);
            bounds.removeFromLeft(spareWidth);//new for panel buttons february 2024
            return bounds;
        }
        //----------------------------------------------------------------------------------
        void paint(juce::Graphics& g) override
        {
            //g.fillAll(Colours::dimgrey);
            
            auto thumbArea = getLocalBounds();
            juce::Rectangle<int> extZone = getRenderZone(thumbArea);
            g.setColour(digitPanelColour);
            g.fillRect(extZone);
            g.setColour(juce::Colours::black);
            g.drawRect(extZone, 1);
            //juce::Rectangle<int> wavZone = getWaveZone(thumbArea);
            wavZone = getWaveZone(thumbArea);
            g.setColour(wavBackgroundColour);
            g.fillRect(wavZone);

            double startTime = 0.0f;
            double  endTime = 1.0f;
            double  endofrecording = 1.0f;
            double currentlength = getSampleSize(); //thumbnail.getTotalLength();
            endofrecording = juce::jmax(10.0, currentlength);
            juce::Range<double> newRange;
            double thumbnailsize;
            int xzoomticknb;
            int eScopeID = chanID; // for debug purposes

            switch (displayThumbMode)
            {
            case FullThumb: //0: //Full Thumb mode (expand recording data to window when stopping Recording
                if (thumbnail.getTotalLength() > 0.0)
                {
                    endTime = thumbnail.getTotalLength();
                    scrollbar.setAutoHide(false);
                    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                    newRange.setStart(0.0);
                    newRange.setEnd(endTime);
                    scrollbar.setRangeLimits(newRange);
                    setRange(newRange);
                    wavZone = getWaveZone(thumbArea);
                    paintVerticalGrid(g, wavZone);
                    paintHorizontalGrid(g, wavZone, yScale);
                    g.setColour(wavFormColour);
                    thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    xzoomticknb = createZoomVector(zoomVector);
                    drawXLabels(g, thumbArea);
                    drawYLabels(g, thumbArea, 1);
                }
                break;

            case ScrollingView://1: // recording mode (scrolling data)
                if (thumbnail.getTotalLength() > 0.0)
                {
                    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                    g.setColour(wavFormColour);
                    thumbnail.drawChannels(g, wavZone.reduced(2), startTime, endofrecording, (float)ThumbYZoom);
                }
                break;

            case Oscilloscope: //2: //oscilloscope dancing view
                if (thumbnail.getTotalLength() > 0.0)
                {
                    if (currentlength >= viewSize)
                    {
                        thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                        wavZone = getWaveZone(thumbArea);
                        paintVerticalGrid(g, wavZone);
                        paintHorizontalGrid(g, wavZone, yScale);
                        g.setColour(wavFormColour);
                        thumbnail.drawChannels(g, wavZone.reduced(2), currentlength - viewSize, currentlength, (float)ThumbYZoom);
                        bTriggered = false;
                    }
                }
                break;

            case Zooming://3: // zooming mode
                if (thumbnail.getTotalLength() > 0.0)
                {
                    horizontalScale xScale = ((bool) triggerSettings->getControlValue("Enable") == true ? RelativeToTrigger : Absolute);
                    thumbnailsize = thumbnail.getTotalLength();
                    newRange.setStart(0.0);
                    newRange.setEnd(thumbnailsize);
                    scrollbar.setRangeLimits(newRange);
                    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                    wavZone = getWaveZone(thumbArea);
                    //paintVerticalGrid(g, wavZone);
                    //drawXLabels(g, thumbArea);
                    paintTimeLines(g, wavZone, thumbArea, xScale);
                    paintHorizontalGrid(g, wavZone, yScale);
                    drawYLabels(g, thumbArea, 1);                    
                    g.setColour(wavFormColour);
                    thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                }
                break;
            case Triggerred://4: //oscilloscope with trigger
                if (*bBufferReady)
                {
                    horizontalScale xScale = ((bool)triggerSettings->getControlValue("Enable") == true ? RelativeToTrigger : Absolute);
                    if (bTriggered)
                    {
                        //    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                        bTriggered = false;
                    }
                    xzoomticknb = createZoomVector(zoomVector);
                    //thumbnailsize = thumbnail.getTotalLength();
                    newRange.setStart(0.0);
                    newRange.setEnd(viewSize); // thumbnailsize);
                    setRange(newRange);
                    wavZone = getWaveZone(thumbArea);
                    //paintVerticalGrid(g, wavZone);
                    //paintTimeGridLin(g, wavZone);
                    //drawXLabelsOffset(g, thumbArea, viewSize * 0.5);
                    paintTimeLines(g, wavZone, thumbArea, xScale);
                    paintHorizontalGrid(g, wavZone, yScale);
                    drawYLabels(g, thumbArea, yScale);
                    drawInfo(g, wavZone); // for debug only
                    //thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    drawBuffer(g, wavZone, visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);                    
                    
                    if (visibleRange != visibleRangePrevious)
                    {
                        visibleRangePrevious = visibleRange;
                        DBG("---------------");
                    }
                }
                break;

            case TriggeredZoomed://5: //oscilloscope with trigger Zoom
                if (*bBufferReady)
                {
                    horizontalScale xScale = ((bool)triggerSettings->getControlValue("Enable") == true ? RelativeToTrigger : Absolute);
                    if (bTriggered)
                    {
                        //    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                        bTriggered = false;
                    }
                    //thumbnailsize = thumbnail.getTotalLength();
                    //newRange.setStart(0.0);
                    //newRange.setEnd(thumbnailsize);
                    //setRange(newRange);
                    wavZone = getWaveZone(thumbArea);
                    //paintVerticalGrid(g, wavZone);
                    paintTimeLines(g, wavZone, thumbArea, xScale);
                    //paintTimeGridLin(g, wavZone);
                    //drawXLabelsOffset(g, thumbArea, viewSize * 0.5);
                    paintHorizontalGrid(g, wavZone, yScale);
                    drawYLabels(g, thumbArea, yScale);
                    //thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    drawBuffer(g, wavZone, visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);                    
                    
                    if (visibleRange != visibleRangePrevious)
                    {
                        visibleRangePrevious = visibleRange;
                        DBG("---------------");
                    }
                }
                break;
            }            
        }      
        //----------------------------------------------------------------------------------
        void resized() override
        {
            int xzoomticknb;
            /*
            auto thumbArea = getLocalBounds();
            wavZone = getWaveZone(thumbArea);

            //scrollbar.setBounds(getLocalBounds().removeFromBottom(14).removeFromRight(50).
            //reduced(2));
            auto bounds = getLocalBounds().removeFromBottom(14).removeFromRight(50).reduced(2);
            bounds = getLocalBounds().removeFromBottom(14).reduced(2);
            bounds.setWidth(wavZone.getWidth());
            //scrollbar.setBounds(getLocalBounds().removeFromBottom(14).reduced(2));
            scrollbar.setBounds(bounds);*/
            //xGridModeSlider.setBounds(5 + xGridModeLabel.getWidth(), 20, 60, 20);
            //yGridModeSlider.setBounds(5 + yGridModeLabel.getWidth(), 50 , 60, 20);

            /*
            eScopeChannelLabel.setBounds(5, 20, 80, 20);
            
            GroupeLabel.setBounds(5, 60, 50, 20);
            //cmbBoxGroupe.setBounds(80, 20, 64, 24);
            cmbBoxGroupe.setBounds(5 + GroupeLabel.getWidth(), 60, 87, 24);
            chkBoxXLink.setBounds(5, 90, 64, 24);
            chkBoxYLink.setBounds(80, 90, 64, 24);
            
            xGridModeLabel.setBounds(5, 120, 50, 20);            
            yGridModeLabel.setBounds(5, 150, 50, 20);           

            cmbBoxXMode.setBounds(5 + xGridModeLabel.getWidth(), 120, 87, 24);
            cmbBoxYMode.setBounds(5 + yGridModeLabel.getWidth(), 150, 87, 24);

            sliderLabel.setBounds(5, 180, 50, 20);
            sliderOffset.setBounds(5 + sliderLabel.getWidth(), 180,100, 20);
            sliderOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, sliderOffset.getTextBoxHeight());

            sliderXLabel.setBounds(5, 210, 50, 20);
            sliderXOffset.setBounds(5 + sliderXLabel.getWidth(), 210, 100, 20);
            sliderXOffset.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, sliderXOffset.getTextBoxHeight());
            */
            xzoomticknb = createZoomVector(zoomVector);
        }
        //----------------------------------------------------------------------------------
        int createZoomVector(std::vector<double>& Divider)
        {
            //auto vrange = visibleRange.getLength();
            auto totlen = getSampleSize(); //thumbnail.getTotalLength();
            if (totlen > 0)
            {
                auto thumbArea = getLocalBounds();
                auto wavWindowWidth = wavZone.getWidth();

                //    auto SampleRate = 48000;
                double SampleSize = totlen * sampleRate;
                //double Ratio = SampleSize / thumbArea.getWidth();
                double Ratio = (double)SampleSize / (double)wavWindowWidth;
                double div = Ratio;
                int it = 0;
                int iteration = 0;
                double seed = 1.0;
                double sub1Tab[]{ 32.0, 24.0, 16.0, 12.0, 8.0, 6.0 , 4.0, 3.0 , 2.0 };

                std::vector<double> Divider2;// , Divider;
                Divider.clear();
                Divider2.clear();
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

                for (double n : sub1Tab)
                {
                    Divider.push_back(1.0 / n);
                }

                std::sort(Divider.begin(), Divider.end(), std::greater());// greater<double>());

                return ((int)Divider.size());
            }
            else
                return(0);
        }
        //----------------------------------------------------------------------------------
        void mouseDown_old(const juce::MouseEvent& event)
        {
            auto Posi3 = getMouseXYRelative(); // Read Mouse click position
            //DBG("Mouse.x = " << Posi3.getX());            
        }
        //----------------------------------------------------------------------------------
        void drawMousePosLabels(juce::Graphics& g)
        {
            auto thumbArea = getLocalBounds();
            juce::Rectangle<int> extZone = getRenderZone(thumbArea);
            
            if((Posi3.getX()>= extZone.getX()) && (Posi3.getX()<= extZone.getX()+ extZone.getWidth()) )
            {
            g.setColour(juce::Colours::whitesmoke);
            g.setOpacity(1.0);
            const int fontHeight = 12;
            g.setFont(fontHeight);       
            juce::String str;            
            str << "x = " << Posi3.getX()<< " y = "<<Posi3.getY();
            //str.toDecimalStringWithSignificantFigures(x, 2);
            juce::Rectangle<int> r;
            auto textWidth = g.getCurrentFont().getStringWidth(str);
            r.setSize(textWidth, fontHeight);
            r.setCentre(static_cast<int>(Posi3.getX()), 0);
            r.setY(Posi3.getY() - 1 * fontHeight);
            g.drawFittedText(str, r, juce::Justification::centred, 1);
            float timex = mouseXToTime(Posi3.getX());
            str.clear();
            str << "t = " << timex <<" s";
            textWidth = g.getCurrentFont().getStringWidth(str);
            r.setY(Posi3.getY() + 1 * fontHeight);
            g.drawFittedText(str, r, juce::Justification::centred, 1);
            }
        }
        //----------------------------------------------------------------------------------
        void mouseMove(const juce::MouseEvent& event)
        {
            Posi3 = getMouseXYRelative(); // Read Mouse  position
            repaint();
        }
        //----------------------------------------------------------------------------------
        void mouseDown(const juce::MouseEvent& event)
        {
            
            Posi3 = getMouseXYRelative(); // Read Mouse click position
             repaint();
            //DBG("Mouse.x = " << Posi3.getX());
             if (event.mods.isRightButtonDown())
             {
                 // Create a PopupMenu instance
                 juce::PopupMenu menu;

                 // Add some items to the menu
                 menu.addItem(Zoom_1_Centered, "Zoom 1:1 Centered");
                 menu.addItem(Zoom_1_Left, "Zoom 1:1 Left");
                 menu.addItem(Zoom_1_Right, "Zoom 1:1 Right");
                 menu.addItem(Zoom_Max_Centered, "Zoom Max Centered");
                 menu.addItem(Zoom_Out_Full, "Zoom Out Full");

                 // Show the menu and handle the user's choice
                 menu.showMenuAsync(juce::PopupMenu::Options(),
                     [this](int selectedId)
                     {
                         // Handle the user's selection
                         int eScopeID = chanID;// for debug only
                         int xZoom;
                         double ZFactor;
                         unsigned long zoomInAddress;
                         xZoom = zoomVector.size();
                         double pretrigmemratio = (double)triggerSettings->getControlValue("Pre-trigger") / (double)100.0;
                         switch (selectedId)
                         {
                         case Zoom_1_Centered://display values centered on trigger point
                             juce::Logger::outputDebugString("Zoom_1_Centered");                             
                             XZoomIndex = xZoom - 10;
                             ZFactor = zoomVector.at(XZoomIndex);
                             zoomInAddress = (unsigned long)((double)eBuffer->getNumSamples() * pretrigmemratio);
                             break;
                         case Zoom_Max_Centered://display values centered on trigger point
                             juce::Logger::outputDebugString("Zoom_Max_Centered");
                             XZoomIndex = xZoom - 1;
                             ZFactor = zoomVector.at(XZoomIndex);
                             zoomInAddress = (unsigned long)((double)eBuffer->getNumSamples() * pretrigmemratio);
                             break;
                         case Zoom_1_Left://display values starting at first samples
                             juce::Logger::outputDebugString("Zoom_1_Left"); 
                             XZoomIndex = xZoom - 10;
                             ZFactor = zoomVector.at(XZoomIndex);                             
                             zoomInAddress = 0;
                             break;
                         case Zoom_1_Right://display values ending at last samples
                             juce::Logger::outputDebugString("Zoom_1_Right");
                             XZoomIndex = xZoom - 10;
                             ZFactor = zoomVector.at(XZoomIndex);
                             zoomInAddress = eBuffer->getNumSamples(); //get max
                             break;
                         case Zoom_Out_Full:
                             juce::Logger::outputDebugString("Zoom_Out_Full");
                             XZoomIndex = 0;
                             ZFactor = zoomVector.at(XZoomIndex);                             
                             zoomInAddress = eBuffer->getNumSamples() / 2;    //center                         
                             break;
                         default: break; // No option was selected
                         }
                         setDisplayZone(ZFactor, zoomInAddress);
                         sendChangeMessage();
                         repaint();
                     });
             }
        }
        //----------------------------------------------------------------------------------
        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
        {
            Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
            
            float sampleTimeInSeconde = getSampleSize(); //
            if ((thumbnail.getTotalLength() > 0.0)|| (*bBufferReady))
            {
                if ((juce::ModifierKeys::currentModifiers.isCtrlDown()) ||
                    (juce::ModifierKeys::currentModifiers.isAltDown())) //Y Zoom
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
                    sendChangeMessage();
                }
                else if (juce::ModifierKeys::currentModifiers.isShiftDown())//X Move
                {
                    auto newStart = visibleRange.getStart() -
                        wheel.deltaY * (visibleRange.getLength()) / 10.0;
                    newStart = juce::jlimit(0.0, juce::jmax(0.0, sampleTimeInSeconde -
                        (visibleRange.getLength())), newStart);
                    setRange({ newStart, newStart + visibleRange.getLength() });
                    repaint();
                    sendChangeMessage();
                }
                else //X Zoom Control
                {
                    auto WheelDelta = wheel.deltaY;
                    float totlen;
                    auto vrange = visibleRange.getLength();

                    if (displayThumbMode == 3)
                    {
                        totlen = sampleTimeInSeconde;
                        if (totlen == vrange)
                            XZoomIndex = 0;
                    }
                    else
                        totlen = viewSize;
                    

                    double NewZoomFactor;
                    //DBG("XZoomIndex = " << XZoomIndex << " vectorSize = " 
                    //<< zoomVector.size());
                    if (WheelDelta > 0)
                    {
                        if (XZoomIndex < zoomVector.size() - 1)
                        {
                            XZoomIndex++;
                            NewZoomFactor = zoomVector[XZoomIndex];
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
                    setDisplayXZone(NewZoomFactor, Posi3);
                    DBG("mouseWh:XZoomIndex = " << XZoomIndex << " vectorSize = "
                        << zoomVector.size() << " NewZoom " << NewZoomFactor << " Range = ["
                        << visibleRange.getStart() << " , " << visibleRange.getEnd()
                        << "] Pos = [" << Posi3.x << " , " << Posi3.y << " ]");
                    sendChangeMessage();
                }
            }
        }
        //----------------------------------------------------------------------------------
        void setDisplayXZoom(double zoomfactor)
        {
            int eScopeID = chanID;
            auto thumbArea = getLocalBounds(); //bounds of display zone
            auto width = getWidth(); // width of Display zone in pixels
            juce::Point< int >MousePosition;
            MousePosition.setX(width / 2);
            MousePosition.setY(0);
            setDisplayXZone(zoomfactor, MousePosition);
        }
        //----------------------------------------------------------------------------------
        double getSampleSize(void)
        {
            double sampleTimeInSecond = 0;
            if (displayThumbMode > 3)// if was in triggered mode
            {
                if(sampleRate < 1 )
                    return(sampleTimeInSecond);
                else
                {
                    sampleTimeInSecond = (double)eBuffer->getNumSamples() / (double)sampleRate;
                    return(sampleTimeInSecond);
                }
            }
            else
            {
                return( thumbnail.getTotalLength());
            }
        }
        //----------------------------------------------------------------------------------
        void setDisplayZone(double zoomfactor, unsigned long Address)
        {
            juce::Point< int >MousePosition;
            int eScopeID = chanID;
            displayFullThumb = false;
            unsigned long bufferSizePoints = eBuffer->getNumSamples();
            double  waveLength = getSampleSize();
            if (waveLength > 0)
            {
                float invZoom;
                if (zoomfactor < 1)
                    invZoom = 1.0 / zoomfactor;
                auto wavWindowWidth = wavZone.getWidth();//width in pixels 
                MousePosition.setX(wavWindowWidth / 2.0);

                auto totlen = waveLength; //thumbnail.getTotalLength(); //total length of sample in seconds
                double displayStartTime, displayEndTime, displayWidth;

                auto thumbArea = getLocalBounds(); //bounds of display zone
                auto width = getWidth(); // width of Display zone in pixels

                double SampleSize = totlen * sampleRate; //size  of sample in points
                double Ratio = SampleSize / thumbArea.getWidth();
                Ratio = (double)SampleSize / (double)wavWindowWidth;

                auto timeAtMousePos = xToTime((float)MousePosition.x);
                timeAtMousePos = waveLength / (double)2.0;

                double timeAtZoomAddress = (double)waveLength *(double)Address / (double)bufferSizePoints;

                //double PosixRatioPix = (double)width / (double) Posi3.x;
                double PosixRatioPix = (double)wavWindowWidth / (double)MousePosition.x;

                displayWidth = totlen * zoomfactor / Ratio;

                displayStartTime = timeAtZoomAddress - displayWidth / PosixRatioPix;
                displayEndTime = displayStartTime + displayWidth;
                //   DBG("Mouse.x = " << Posi3.x << " PosixRatio = " << PosixRatioPix << " timeAtMousePos = " << timeAtMousePos << "(s) displayStartTime = " << displayStartTime << "(s) displayEndTime = " << displayEndTime << "(s) zoom ratio = " << zoomfactor);
                if (displayStartTime < 0)
                {
                    DBG("setDisplayXZone:displayStartTime = " << displayStartTime);
                    displayStartTime = 0; // prevent stupid cases
                    displayEndTime = displayStartTime + displayWidth;

                }
                if (displayEndTime > totlen)
                {
                    DBG("setDisplayXZone:displayEndTime = " << displayEndTime);
                    displayEndTime = totlen;// prevent stupid cases
                    displayStartTime = displayEndTime - displayWidth;
                }
                setRange({ displayStartTime, displayEndTime });
                displayThumbMode = 5;
            }
            else
                repaint();

        }
        //----------------------------------------------------------------------------------
        void setDisplayXZone(double zoomfactor, juce::Point< int >MousePosition)
        {
            int eScopeID = chanID;
            displayFullThumb = false;
            float waveLength = getSampleSize();

            if (displayThumbMode > 3)// if was in triggered mode
            {
                displayThumbMode = 5;
            }                
            else
            {
                displayThumbMode = 3; //zoom mode
            }
                
         //auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
         //   repaint();
            if (waveLength > 0)
            {
                float invZoom;
                if (zoomfactor < 1)
                    invZoom = 1.0 / zoomfactor;
                auto wavWindowWidth = wavZone.getWidth();//width in pixels 

                auto totlen = waveLength; //thumbnail.getTotalLength(); //total length of sample in seconds
                double displayStartTime, displayEndTime, displayWidth;

                auto thumbArea = getLocalBounds(); //bounds of display zone
                auto width = getWidth(); // width of Display zone in pixels

                double SampleSize = totlen * sampleRate; //size  of sample in points
                double Ratio = SampleSize / thumbArea.getWidth();
                Ratio = (double)SampleSize / (double)wavWindowWidth;

                auto timeAtMousePos = xToTime((float)MousePosition.x);

                //double PosixRatioPix = (double)width / (double) Posi3.x;
                double PosixRatioPix = (double)wavWindowWidth / (double)MousePosition.x;

                displayWidth = totlen * zoomfactor / Ratio;

                displayStartTime = timeAtMousePos - displayWidth / PosixRatioPix;
                displayEndTime = displayStartTime + displayWidth;
                //   DBG("Mouse.x = " << Posi3.x << " PosixRatio = " << PosixRatioPix << " timeAtMousePos = " << timeAtMousePos << "(s) displayStartTime = " << displayStartTime << "(s) displayEndTime = " << displayEndTime << "(s) zoom ratio = " << zoomfactor);
                if (displayStartTime < 0)
                {
                    DBG("setDisplayXZone:displayStartTime = " << displayStartTime);
                    displayStartTime = 0; // prevent stupid cases
                    displayEndTime = displayStartTime + displayWidth;
                    
                }
                if (displayEndTime > totlen)
                {
                    DBG("setDisplayXZone:displayEndTime = " << displayEndTime);
                    displayEndTime = totlen;// prevent stupid cases
                    displayStartTime = displayEndTime - displayWidth;
                }
                setRange({ displayStartTime, displayEndTime });
            }
            else
                repaint();
        }
        //----------------------------------------------------------------------------------
        double AmpdBGainToMultFactor(double AmpGaindB)
            // called at init
        {
            double fact;
            fact = pow(10, AmpGaindB / 20.0);
            return fact;

        }
        //----------------------------------------------------------------------------------
        juce::Range<double> getVisibleRange() { return(visibleRange); }
        //----------------------------------------------------------------------------------
        double getXZoom() { return(ThumbXZoom); }
        int getZoomGroup() { return(zoomGroup); }
        bool getXZoomFlag() { return(xZoomFlag); }
        bool getYZoomFlag() { return(yZoomFlag); }
        int getXZoomIndex() { return (XZoomIndex); } 
        int getYZoomIndex() { return (YZoomIndex); }

        void setXZoom(double zoom) { ThumbXZoom = zoom; }
        void setZoomGroup(int group) { zoomGroup = group; }
        void setXZoomFlag(bool flag) { xZoomFlag = flag; }
        void setYZoomFlag(bool flag) { yZoomFlag = flag; }
        void setXZoomIndex(int zidx) { XZoomIndex = zidx; }
        void setYZoomIndex(int zidx) { YZoomIndex = zidx; }

        //----------------------------------------------------------------------------------
    private:
        grape::Panel* triggerSettings;  // handle to trigger settings

        juce::AudioFormatManager formatManager;
        juce::AudioThumbnailCache thumbnailCache{ 10 };
        juce::AudioThumbnail thumbnail{ 1, formatManager, thumbnailCache };

        double sampleRate = 0.0;
        int samplesPerBlockExpected = 0;

        // display things
        bool displayFullThumb = false;
        int displayThumbMode;

        double ThumbXZoom = 1.0f;

        int zoomGroup = 0;
        bool xZoomFlag = false ;
        bool yZoomFlag = false;

        int XZoomIndex = 0;
        double stepSize = 0; //stores the graduation step size (usually in s)
        std::vector<double> zoomVector;

        juce::ScrollBar scrollbar{ false };
        juce::Range<double> visibleRange;
        juce::Range<double> visibleRangePrevious;
        
        int currentOffset = 0;

        //int xScale = 0; //0 = linear // 1 = dB
        //horizontalScale xScale = Absolute; //0 = linear // 1 = dB
        //int yScale = 0; //0 = linear // 1 = dB
        verticalScale yScale = Linear; //0 = linear // 1 = dB
        
        double ThumbYZoom = 1.0f;
        int YZoomIndex = 8;
        const double AmpZoomGainStepdB = 1.5; //step in dB of each MouseWheel click
        double AmpZoomGainFactor = AmpdBGainToMultFactor(AmpZoomGainStepdB);
        juce::Rectangle<int> wavZone;
        
                
        //----------------------------------------------------------------------------------
        double timeToX(const double time) const
        {
            double preciseTime;
            if (visibleRange.getLength() <= 0)
                return 0;
            //auto width = getWidth();
            double width = wavZone.getWidth();

            preciseTime = (double)width * ((double)time - (double)visibleRange.getStart()) /
                (double)visibleRange.getLength();
            return (preciseTime);
        }
        //----------------------------------------------------------------------------------
        double xToTime(const float x) const
        {
            //auto width = getWidth();
            auto width = wavZone.getWidth();
            return (x / (float)width) * (visibleRange.getLength()) + visibleRange.getStart();
        }
        //----------------------------------------------------------------------------------
        double mouseXToTime(const float x) const //identical to xToTime but separated for debug purposes
        {
            //auto width = getWidth();
            auto width = wavZone.getWidth();
            return ((x - spareWidth) / (float)width) * (visibleRange.getLength()) + visibleRange.getStart();
        }
        //----------------------------------------------------------------------------------
        void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
            override
        {
            if (scrollBarThatHasMoved == &scrollbar)
                //if (!(isFollowingTransport && transportSource.isPlaying()))
                setRange(visibleRange.movedToStartAt(newRangeStart));
        }
        //----------------------------------------------------------------------------------
        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            if (source == &thumbnail)
            {
                repaint(); // message broadcasted by the recorder
                repaintBroadcasted = true;
                sendChangeMessage(); //broacast to other RecordingThumbnails
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingThumbnail)
    };
    //=====================================================================================

