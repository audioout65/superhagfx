cmake_minimum_required(VERSION 3.22)
project(LiveRack VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_gui_app(LiveRack
    PRODUCT_NAME "LiveRack"
    COMPANY_NAME "YourName"
)

juce_generate_juce_header(LiveRack)

target_sources(LiveRack PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/NativeWindowCapture.mm
    Source/AudioChannel.h
    Source/Compressor.h
    Source/PluginManager.h
    Source/PluginSlot.h
    Source/SlotButton.h
    Source/RackSlotComponent.h
    Source/NativeWindowCapture.h
    Source/CompressorEditor.h
    Source/ChannelStripComponent.h
    Source/ChannelDetailView.h
    Source/TopBarComponent.h
    Source/MeterAndGainOverlay.h
    Source/InputPanelComponent.h
    Source/LeftSidebarComponent.h
    Source/LiveRackLookAndFeel.h
    Source/MainMenu.h
)

target_compile_definitions(LiveRack PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:LiveRack,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:LiveRack,JUCE_VERSION>"
)

target_link_libraries(LiveRack PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if (APPLE)
    target_link_libraries(LiveRack PRIVATE
        "-framework Cocoa"
        "-framework CoreGraphics"
        "-framework ImageIO"
    )

    # JUCE fournit son propre Info.plist tout fait (les attributs Xcode
    # INFOPLIST_KEY_* sont donc ignores). On insere la cle d'autorisation
    # microphone directement dans le fichier final, apres chaque build.
    add_custom_command(TARGET LiveRack POST_BUILD
        COMMAND bash -c "/usr/libexec/PlistBuddy -c 'Delete :NSMicrophoneUsageDescription' '$<TARGET_BUNDLE_CONTENT_DIR:LiveRack>/Info.plist' 2>/dev/null; /usr/libexec/PlistBuddy -c 'Add :NSMicrophoneUsageDescription string \"LiveRack a besoin des entrees audio pour traiter le son en direct.\"' '$<TARGET_BUNDLE_CONTENT_DIR:LiveRack>/Info.plist'"
        VERBATIM
    )
endif()

juce_add_console_app(PluginScanWorker
    PRODUCT_NAME "PluginScanWorker"
)

juce_generate_juce_header(PluginScanWorker)

target_sources(PluginScanWorker PRIVATE
    Source/PluginScanWorkerMain.cpp
)

target_compile_definitions(PluginScanWorker PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
)

target_link_libraries(PluginScanWorker PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
