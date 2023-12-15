#pragma once

#include <vector>
#include <algorithm>
#include "..\grape\source\grape.h"
    //=====================================================================================
    class RecordingThumbnail : public juce::Component,
        private juce::ChangeListener,
        private juce::ScrollBar::Listener,
        public juce::ChangeBroadcaster
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

        juce::AudioThumbnail& getAudioThumbnail() { return thumbnail; }

        juce::Colour testColour = juce::Colours::antiquewhite;
        juce::Colour wavFormColour = juce::Colour(0xff43d996);
        juce::Colour wavBackgroundColour = juce::Colours::black;
        juce::Colour digitPanelColour = juce::Colour(0xff232323);
        juce::Colour digitColour = juce::Colour(0xff8a8a8a);
        juce::Colour gridColour = juce::Colour(0xff8a8a8a);
        juce::Colour gridHorizontalCenterColour = juce::Colours::red;
        juce::Colour triggerColour = juce::Colours::yellow;
        juce::Colour backGroundColour = juce::Colour(0xff2e2e2e);
        double gridOpacity = 0.5; //grid opacity
        double triggerOpacity = 0.75; //trigger lines opacity

        int yScaleZoneWidth = 50;
        float viewSize = 0.1;// viewing window size
        int chanID = 0; //copy of eScope ID at Thumbnail level so Listener can retrieve info
        std::vector<float> mAudioPoints;
        std::vector<float> mMaxAudioPoints;
        std::vector<float> mMinAudioPoints;        
        //-------------------------------------------------------------------
        //following elements are passed between RecTumbnail and AudioRecorder
        bool bTriggered = false;
        juce::AudioBuffer<float>* eBuffer;
        unsigned long *wfStartAddr ;
        unsigned long *wfTriggAddr ;
        //----------------------------------------------------------------------------------
        bool* getTriggeredPtr(void) { return &bTriggered; }
        //----------------------------------------------------------------------------------
        void setBufferedToImage(juce::AudioBuffer<float>* recBuffer) {  eBuffer = recBuffer; }
        //called by eScope setViewSize
        //----------------------------------------------------------------------------------
        void setBufferStartAddress(unsigned long* addr) { wfStartAddr = addr; }
        //----------------------------------------------------------------------------------
        void setBufferTriggAddress(unsigned long* addr) { wfTriggAddr = addr; }
        //----------------------------------------------------------------------------------
        void setViewSize(float dispTime)// sets viewing window size in secondes in oscillo mode
        {
            viewSize = dispTime;
        }
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
        void setThreshold(double threshold)
        {
#if modify_triggers == 1
            switch ((int)(threshold * 100))
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
        void setRange(juce::Range<double> newRange)
        {
            visibleRange = newRange;
            scrollbar.setCurrentRange(visibleRange);
            repaint();
        }
        //------------------------------------------------------------------------------
        double  getTimeStepSize(int displayWidthPix, double wavDurationToDisplaySec)
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
        void paintGridLin(juce::Graphics& g, const juce::Rectangle<int>& bounds)
        {
            double zoomfactor = 128;

            //auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
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
            g.setColour(gridColour);
            g.setOpacity(gridOpacity);
            for (auto x : xs)
            {
                newX1 = timeToX(x); // get 
                g.drawVerticalLine(newX1, top, bottom);
            }

            int newY2, newY41, newY42, newY81, newY82, newY83, newY84, thumbh;
            thumbh = bounds.getHeight();
            newY2 = bounds.getCentreY();
            g.setColour(gridHorizontalCenterColour);//Draw middle horizontal line
            g.setOpacity(gridOpacity);
            g.drawHorizontalLine(newY2, left, right);

            g.setColour(triggerColour);//Draw middle horizontal line
            g.setOpacity(triggerOpacity);
            newX1 = timeToX(viewSize * 0.5); // get time center
            g.drawVerticalLine(newX1, top, bottom);
            double yfact = ThumbYZoom;
            float thresholdPos = newY2 - (float)thumbh * 0.5 * thresholdTrigger * ThumbYZoom;
            newY2 = (int)thresholdPos;            
            g.drawHorizontalLine(newY2, left, right);             
        }
        //----------------------------------------------------------------------------------
        void paintGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds)
        {
            double zoomfactor = 128;

            //auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
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
            g.setColour(gridColour);
            g.setOpacity(gridOpacity);
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
            g.setColour(gridHorizontalCenterColour);//Draw middle horizontal line
            g.setOpacity(gridOpacity);
            g.drawHorizontalLine(newY2, left, right);
            //DBG("paintGrid:: Y2 = " << newY2); 
            // draw all other horizontal lines
            g.setColour(gridColour);
            g.setOpacity(gridOpacity);
            for (auto y : NiceGainY)
            {
                newY41 = newY2 - y;
                g.drawHorizontalLine(newY41, left, right);
                newY42 = newY2 + y;
                g.drawHorizontalLine(newY42, left, right);
                //DBG("paintGrid:: Y41 = " << newY41<< " Y42 = " << newY42);
            }
        }
        //----------------------------------------------------------------------------------
        void drawBuffer(juce::Graphics& g,
            const juce::Rectangle<int>& bounds,
            double  startTimeSeconds,
            double  endTimeSeconds,
            float  	verticalZoomFactor
        )
        {
            if (startTimeSeconds < 0)
                startTimeSeconds = 0;//catch stupid conditions
                        
            mAudioPoints.clear();

            mMaxAudioPoints.clear();
            mMinAudioPoints.clear();

            double wavPoint,wavMin,wavMax;
            unsigned long* ptr;
            ptr = wfTriggAddr;
            juce::AudioBuffer<float> waveform = *eBuffer;
            auto ptNb = eBuffer->getNumSamples();

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
            float ratio = (float)ptNb / (float)width;

            DBG("ptNb = " << ptNb << " width = " << width << " ratio = " << ratio << " verticalZoomFactor = " << verticalZoomFactor);

            int idx,idxEnd;
            g.setColour(wavFormColour);

            if (ratio >=2)
            {
                for (float sample = (float)startSample; sample < (float)endSample - ratio; sample += ratio)
                {
                    idx = (int)sample;
                    idxEnd = idx + (int)ratio;                 
                    wavMin = eBuffer->getSample(0, idx);
                    wavMax = wavMin;
                    while (idx < idxEnd)
                    {
                        auto val = eBuffer->getSample(0, idx++);
                        if (wavMax < val)
                            wavMax = val;
                        if (wavMin > val)
                            wavMin = val;
                    }
                    mMaxAudioPoints.push_back(wavMax);
                    mMinAudioPoints.push_back(wavMin);
                }
                bool pathStarted = false;
                float pointMinScaled, pointMaxScaled;

                for (int sample = 0; sample < mMinAudioPoints.size(); sample++)
                {
                    pointMinScaled = mMinAudioPoints[sample] * verticalZoomFactor;
                    pointMaxScaled = mMaxAudioPoints[sample] * verticalZoomFactor;
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
            } 
            else // more than 1 pixel per wav sample
            {
                for (float sample = (float)startSample; sample < (float)endSample - ratio; sample += ratio)
                {
                    idx = (int)sample;
                    wavPoint = eBuffer->getSample(0, idx);
                    mAudioPoints.push_back(wavPoint);
                }
                bool pathStarted = false;
                auto pointScaled = mAudioPoints[0] * verticalZoomFactor;
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
            }

            // draw vertical time lines
            g.setColour(testColour);
            g.setOpacity(gridOpacity);
            g.drawLine(left, bottom, right, bottom);
            g.drawLine(left, top, right, top);
            g.drawVerticalLine(left, top, bottom);
            /*
            float nwx1, nwx2, nwy1, nwy2;
            nwx1 = 2 * (right - left) / 3;
            nwy1 = top + 0.25 * (bottom-top);
            nwy2 = top + 0.75 * (bottom - top);
            g.drawVerticalLine((int)nwx1, nwy1, nwy2);*/
            //g.drawLine(left, bottom, left, top);
            g.drawLine(right, bottom, right, top);
         //   g.drawLine(left, bottom, right, top);// diagonals for debug 
         //   g.drawLine(left, top, right, bottom);// diagonals for debug
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
            auto left = textArea.getX();
            auto top = textArea.getY();
            auto right = left + textArea.getWidth();

            std::vector<double> xs = getXs();

            int newX1;
            for (auto x : xs)
            {
                juce::String str;
                newX1 = timeToX(x); // get
                str << x- timeOffset;
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
        void drawYLabels(juce::Graphics& g, const juce::Rectangle<int>& bounds)
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
            juce::String strTxt;
            strTxt << "-144 dB";
            auto txtWidth = g.getCurrentFont().getStringWidth(strTxt);
            rTxt.setSize(txtWidth, fontHeight);

            //std::vector<double> xs = getXs();
            std::vector<int> NiceGainVect;
            std::vector<int> NiceGainY;
            int ret = getNiceGainVect(wavZone.getHeight(), NiceGainVect, NiceGainY);
            int newY2, newY41, newY42;
            int newY1, newGain;

            newY2 = wavZone.getCentreY();

            for (int idx = 0; idx < NiceGainY.size(); idx++)
            {
                juce::String str;
                newY1 = newY2 - NiceGainY[idx] - fontHeight / 2.0;
                newGain = NiceGainVect[idx]; // get
                str << newGain << " dB";
                str.toDecimalStringWithSignificantFigures(newGain, 2);
                juce::Rectangle<int> r;
                auto textWidth = g.getCurrentFont().getStringWidth(str);
                int left, top, right, butt;

                left = maxright - textWidth - 10;

                r.setLeft(left);
                r.setY(newY1);
                r.setSize(textWidth, fontHeight);
                //            r.setY(textArea.getY());
                g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
                newY1 = newY2 - NiceGainY[idx];
                left = textArea.getTopLeft().getX();
                g.drawHorizontalLine(newY1, left - 3, left);
                //           DBG("drawYLabels:: newY1 = " << newY1 << " gain = " << str);
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
            bounds.removeFromTop(4);
            bounds.removeFromBottom(wavRect.getHeight() + 2);
            bounds.setRight(wavRect.getRight());
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

            if (thumbnail.getTotalLength() > 0.0)
            {
                double startTime = 0.0f;
                double  endTime = 1.0f;
                double  endofrecording = 1.0f;
                double currentlength = thumbnail.getTotalLength();
                endofrecording = juce::jmax(10.0, currentlength);
                juce::Range<double> newRange;
                double thumbnailsize;
                int xzoomticknb;

                switch (displayThumbMode)
                {
                case 0: //Full Thumb mode (expand recording data to window when stopping Recording
                    endTime = thumbnail.getTotalLength();
                    scrollbar.setAutoHide(false);
                    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                    newRange.setStart(0.0);
                    newRange.setEnd(endTime);
                    scrollbar.setRangeLimits(newRange);
                    setRange(newRange);
                    wavZone = getWaveZone(thumbArea);
                    paintGrid(g, wavZone);
                    g.setColour(wavFormColour);
                    thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    xzoomticknb = createZoomVector(zoomVector);
                    drawXLabels(g, thumbArea);
                    drawYLabels(g, thumbArea);
                    break;

                case 1: // recording mode (scrolling data)                
                    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                    g.setColour(wavFormColour);
                    thumbnail.drawChannels(g, wavZone.reduced(2), startTime, endofrecording, (float)ThumbYZoom);
                    break;

                case 2: //oscilloscope dancing view                    
                    if (currentlength >= viewSize)
                    {
                        thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                        wavZone = getWaveZone(thumbArea);
                        paintGrid(g, wavZone);
                        g.setColour(wavFormColour);
                        thumbnail.drawChannels(g, wavZone.reduced(2), currentlength - viewSize, currentlength, (float)ThumbYZoom);
                        bTriggered = false;
                    }
                    break;

                case 3: // zooming mode                
                    thumbnailsize = thumbnail.getTotalLength();
                    newRange.setStart(0.0);
                    newRange.setEnd(thumbnailsize);
                    scrollbar.setRangeLimits(newRange);
                    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                    wavZone = getWaveZone(thumbArea);
                    paintGrid(g, wavZone);
                    g.setColour(wavFormColour);
                    thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    drawXLabels(g, thumbArea);
                    drawYLabels(g, thumbArea);
                    break;
                case 4: //oscilloscope with trigger
                    if (bTriggered)
                    {
                        //    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                        bTriggered = false;
                    }
                    thumbnailsize = thumbnail.getTotalLength();
                    newRange.setStart(0.0);
                    newRange.setEnd(thumbnailsize);
                    setRange(newRange);
                    wavZone = getWaveZone(thumbArea);
                    xzoomticknb = createZoomVector(zoomVector);
                    //paintGrid(g, wavZone);
                    paintGridLin(g, wavZone);
                    g.setColour(wavFormColour);
                    ////thumbnail.drawChannels(g, wavZone.reduced(2), currentlength - viewSize, currentlength, ThumbYZoom);
                    //thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    drawBuffer(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    drawXLabelsOffset(g, thumbArea, viewSize * 0.5);
                    //drawYLabels(g, thumbArea);

                    /*
                    juce::AudioBuffer<float>* eBuffer;
                    unsigned long* wfStartAddr;
                    unsigned long* wfTriggAddr;*/
                    break;
                case 5: //oscilloscope with trigger Zoom 
                    if (bTriggered)
                    {
                        //    thumbArea.removeFromBottom(scrollbar.getHeight() + 4);
                        bTriggered = false;
                    }
                    thumbnailsize = thumbnail.getTotalLength();
                    //newRange.setStart(0.0);
                    //newRange.setEnd(thumbnailsize);
                    //setRange(newRange);
                    wavZone = getWaveZone(thumbArea);
                    //paintGrid(g, wavZone);
                    paintGridLin(g, wavZone);
                    g.setColour(wavFormColour);
                    ////thumbnail.drawChannels(g, wavZone.reduced(2), currentlength - viewSize, currentlength, ThumbYZoom);
                    //thumbnail.drawChannels(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    drawBuffer(g, wavZone.reduced(2), visibleRange.getStart(), visibleRange.getEnd(), (float)ThumbYZoom);
                    drawXLabelsOffset(g, thumbArea, viewSize * 0.5);
                    //drawYLabels(g, thumbArea);

                    /*
                    juce::AudioBuffer<float>* eBuffer;
                    unsigned long* wfStartAddr;
                    unsigned long* wfTriggAddr;*/
                    break;
                }
            }
            else
            {
                g.setFont(14.0f);
                //g.drawFittedText ("(No file recorded)", getLocalBounds(), 
                //Justification::centred, 2);
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
            xzoomticknb = createZoomVector(zoomVector);
        }
        //----------------------------------------------------------------------------------
        int createZoomVector(std::vector<double>& Divider)
        {
            //auto vrange = visibleRange.getLength();
            auto totlen = thumbnail.getTotalLength();
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
            double sub1Tab[]{ 24.0, 16.0, 12.0, 8.0, 6.0 , 4.0, 3.0 , 2.0 };

            std::vector<double> Divider2;// , Divider;
            Divider.clear();
            Divider2.clear();
            for (double n : sub1Tab)
            {
                Divider2.push_back(1.0 / n);
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

            return ((int)Divider.size());
        }
        //----------------------------------------------------------------------------------
        void mouseDown(const juce::MouseEvent& event)
        {
            auto Posi3 = getMouseXYRelative(); // Read Mouse click position
            //DBG("Mouse.x = " << Posi3.getX());            
        }
        //----------------------------------------------------------------------------------
        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
        {
            juce::Point< int > Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
            if (thumbnail.getTotalLength() > 0.0)
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
                }
                else if (juce::ModifierKeys::currentModifiers.isShiftDown())//X Move
                {
                    auto newStart = visibleRange.getStart() -
                        wheel.deltaY * (visibleRange.getLength()) / 10.0;
                    newStart = juce::jlimit(0.0, juce::jmax(0.0, thumbnail.getTotalLength() -
                        (visibleRange.getLength())), newStart);
                    setRange({ newStart, newStart + visibleRange.getLength() });
                    repaint();
                    sendChangeMessage();
                }
                else //X Zoom Control
                {
                    auto WheelDelta = wheel.deltaY;
                    auto totlen = thumbnail.getTotalLength();
                    auto vrange = visibleRange.getLength();
                    if (totlen == vrange)
                        XZoomIndex = 0;
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
                    //DBG("XZoomIndex = " << XZoomIndex << " vectorSize = " 
                    //<< zoomVector.size() << " NewZoom " << NewZoomFactor);
                    setDisplayXZone(NewZoomFactor, Posi3);
                    sendChangeMessage();
                }
            }
        }
        //----------------------------------------------------------------------------------
        void setDisplayXZoom(double zoomfactor)
        {
            auto thumbArea = getLocalBounds(); //bounds of display zone
            auto width = getWidth(); // width of Display zone in pixels
            juce::Point< int >MousePosition;
            MousePosition.setX(width / 2);
            MousePosition.setY(0);
            setDisplayXZone(zoomfactor, MousePosition);
        }
        //----------------------------------------------------------------------------------
        void setDisplayXZone(double zoomfactor, juce::Point< int >MousePosition)
        {
            displayFullThumb = false;
            if (displayThumbMode > 3)// if was in triggered mode
                displayThumbMode = 5;
            else
                displayThumbMode = 3; //zoom mode
         //auto Posi3 = getMouseXYRelative(); // Read Hoverin Mouse position
         //   repaint();
            if (thumbnail.getTotalLength() > 0)
            {
                auto wavWindowWidth = wavZone.getWidth();//width in pixels 

                auto totlen = thumbnail.getTotalLength(); //total length of sample in seconds
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
                    displayStartTime = 0; // prevent stupid cases
                if (displayEndTime > totlen)
                    displayEndTime = totlen;// prevent stupid cases
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
        //----------------------------------------------------------------------------------
    private:
        juce::AudioFormatManager formatManager;
        juce::AudioThumbnailCache thumbnailCache{ 10 };
        juce::AudioThumbnail thumbnail{ 1, formatManager, thumbnailCache };

        double sampleRate = 0.0;
        int samplesPerBlockExpected = 0;

        // display things
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
        juce::Rectangle<int> wavZone;
        double thresholdTrigger;

        //----------------------------------------------------------------------------------
        float timeToX(const double time) const
        {
            if (visibleRange.getLength() <= 0)
                return 0;
            //auto width = getWidth();
            auto width = wavZone.getWidth();

            return (float)width * (float)((time - visibleRange.getStart()) /
                visibleRange.getLength());
        }
        //----------------------------------------------------------------------------------
        double xToTime(const float x) const
        {
            //auto width = getWidth();
            auto width = wavZone.getWidth();
            return (x / (float)width) * (visibleRange.getLength()) + visibleRange.getStart();
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
                repaint();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingThumbnail)
    };
    //=====================================================================================

