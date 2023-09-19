/*
  ==============================================================================

    ListenerComponent.h
    Created: 14 Sep 2023 11:51:25am
    Author:  René-Yves

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class ListenerComponent  : public juce::Component,public juce::ChangeListener
{
public:
    ListenerComponent();
    ~ListenerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override; 

private:
    juce::TextButton receiverButton{ "Receiver" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListenerComponent)
};
