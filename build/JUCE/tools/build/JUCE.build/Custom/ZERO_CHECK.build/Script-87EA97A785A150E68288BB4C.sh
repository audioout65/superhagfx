#!/bin/sh
set -e
if test "$CONFIGURATION" = "Custom"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools
  make -f /Users/lebatarddesmontagnes/Dev/LiveRack/build/JUCE/tools/CMakeScripts/ReRunCMake.make
fi

