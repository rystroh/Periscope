/*
  ==============================================================================

    Enums.h
    Created: 4 Sep 2024 4:20:36pm
    Author:  René-Yves

  ==============================================================================
*/

#pragma once
enum triggerModes { Clipping = 1, ThresholdRising,ThresholdFalling,ThresholdRisingOrFalling };
enum zoomModes { Zoom_1_Centered = 1, Zoom_Max_Centered, Zoom_1_Left, Zoom_1_Right, Zoom_Out_Full }; //menu IDs must start @1, not 0
enum horizontalScale { Absolute, RelativeToTrigger };
enum verticalScale { Linear, dB };
enum displayMode { FullThumb, ScrollingView, Oscilloscope, Zooming, Triggerred, TriggeredZoomed };
