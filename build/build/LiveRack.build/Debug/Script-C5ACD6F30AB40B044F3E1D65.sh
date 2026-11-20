#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide header /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/Debug/Defs.txt /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/JuceHeader.h
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide header /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/Release/Defs.txt /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/JuceHeader.h
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide header /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/MinSizeRel/Defs.txt /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/JuceHeader.h
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide header /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/RelWithDebInfo/Defs.txt /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/JuceHeader.h
fi

