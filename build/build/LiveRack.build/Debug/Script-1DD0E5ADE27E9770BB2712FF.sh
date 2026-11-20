#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide pkginfo App /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide pkginfo App /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide pkginfo App /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide pkginfo App /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo
fi

