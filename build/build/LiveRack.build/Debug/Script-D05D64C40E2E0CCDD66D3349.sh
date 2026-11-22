#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Debug/LiveRack.app/Contents
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  bash -c "/usr/libexec/PlistBuddy -c 'Delete :NSMicrophoneUsageDescription' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Debug/LiveRack.app/Contents/Info.plist' 2>/dev/null; /usr/libexec/PlistBuddy -c 'Add :NSMicrophoneUsageDescription string \"LiveRack a besoin des entrees audio pour traiter le son en direct.\"' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Debug/LiveRack.app/Contents/Info.plist'"
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Release/LiveRack.app/Contents
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  bash -c "/usr/libexec/PlistBuddy -c 'Delete :NSMicrophoneUsageDescription' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Release/LiveRack.app/Contents/Info.plist' 2>/dev/null; /usr/libexec/PlistBuddy -c 'Add :NSMicrophoneUsageDescription string \"LiveRack a besoin des entrees audio pour traiter le son en direct.\"' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/Release/LiveRack.app/Contents/Info.plist'"
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/MinSizeRel/LiveRack.app/Contents
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  bash -c "/usr/libexec/PlistBuddy -c 'Delete :NSMicrophoneUsageDescription' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/MinSizeRel/LiveRack.app/Contents/Info.plist' 2>/dev/null; /usr/libexec/PlistBuddy -c 'Add :NSMicrophoneUsageDescription string \"LiveRack a besoin des entrees audio pour traiter le son en direct.\"' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/MinSizeRel/LiveRack.app/Contents/Info.plist'"
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  /Applications/CMake.app/Contents/bin/cmake -E copy /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/JuceLibraryCode/LiveRack/PkgInfo /Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/RelWithDebInfo/LiveRack.app/Contents
  cd /Users/lebatarddesmontagnes/Dev/LiveRack/build
  bash -c "/usr/libexec/PlistBuddy -c 'Delete :NSMicrophoneUsageDescription' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/RelWithDebInfo/LiveRack.app/Contents/Info.plist' 2>/dev/null; /usr/libexec/PlistBuddy -c 'Add :NSMicrophoneUsageDescription string \"LiveRack a besoin des entrees audio pour traiter le son en direct.\"' '/Users/lebatarddesmontagnes/Dev/LiveRack/build/LiveRack_artefacts/RelWithDebInfo/LiveRack.app/Contents/Info.plist'"
fi

