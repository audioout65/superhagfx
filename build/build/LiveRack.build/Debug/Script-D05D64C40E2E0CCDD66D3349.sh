#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Debug/LiveRack.app/Contents
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Release/LiveRack.app/Contents
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/MinSizeRel/LiveRack.app/Contents
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/RelWithDebInfo/LiveRack.app/Contents
fi

