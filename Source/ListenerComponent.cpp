/*
  ==============================================================================

    ListenerComponent.cpp
    Created: 14 Sep 2023 11:51:25am
    Author:  René-Yves

  ==============================================================================
*/

#include <JuceHeader.h>
#include "ListenerComponent.h"

//==============================================================================
ListenerComponent::ListenerComponent()
{
    receiverButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::darkred);
    addAndMakeVisible(receiverButton);

}

ListenerComponent::~ListenerComponent()
{
}

void ListenerComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

    g.setColour (juce::Colours::grey);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText ("ListenerComponent", getLocalBounds(),
                juce::Justification::centred, true);   // draw some placeholder text
}

void ListenerComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    receiverButton.setBounds(getLocalBounds().reduced(30));
}

void ListenerComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    receiverButton.triggerClick();
}
