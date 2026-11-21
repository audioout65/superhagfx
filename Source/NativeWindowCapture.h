#pragma once
#include <string>

// Capture le contenu reel d'une fenetre native macOS en PNG sur disque, via
// les API systeme de capture d'ecran. Volontairement independant de JUCE
// (types simples uniquement) pour eviter tout conflit de noms entre les
// en-tetes JUCE et Cocoa/Carbon (Point, Component, etc.) quand ce fichier
// est inclus depuis du code C++ qui inclut deja JuceHeader.h.
bool captureNativeWindowToFile (void* nsViewPtr, const std::string& outputPath);
