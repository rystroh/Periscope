/*
  ==============================================================================

    PopUpMenu.h
    Created: 5 Sep 2024 11:17:32am
    Author:  René-Yves

  ==============================================================================
*/
#include <JuceHeader.h>
#pragma once

class MyPopupMenu : public juce::Component
{
public:
    MyPopupMenu() {}
    ~MyPopupMenu() override {}

    // This function is called when the user clicks the mouse button.
    void mouseDown(const juce::MouseEvent& event) override
    {
        // Check if it's a right-click (context menu trigger)
        if (event.mods.isRightButtonDown())
        {
            // Create a PopupMenu instance
            juce::PopupMenu menu;

            // Add some items to the menu
            menu.addItem(1, "Option 1");
            menu.addItem(2, "Option 2");
            menu.addItem(3, "Option 3");

            // Show the menu and handle the user's choice
            menu.showMenuAsync(juce::PopupMenu::Options(),
                [this](int selectedId)
                {
                    // Handle the user's selection
                    switch (selectedId)
                    {
                    case 1: juce::Logger::outputDebugString("Option 1 selected"); break;
                    case 2: juce::Logger::outputDebugString("Option 2 selected"); break;
                    case 3: juce::Logger::outputDebugString("Option 3 selected"); break;
                    default: break; // No option was selected
                    }
                });
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::lightgrey);
    }
};