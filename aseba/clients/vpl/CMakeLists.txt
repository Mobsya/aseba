set(CMAKE_AUTOMOC on)

add_executable(thymiovpl WIN32
    thymiovpl.cpp
    ThymioVisualProgramming.cpp
    ResizingView.cpp
    Buttons.cpp
    Block.cpp
    EventBlocks.cpp
    StateBlocks.cpp
    ActionBlocks.cpp
    EventActionsSet.cpp
    Scene.cpp
    Compiler.cpp
    CompilerVisitors.cpp
    Style.cpp
    Utils.cpp
    LogSignalMapper.cpp
    ThymioVisualProgramming.h
    ResizingView.h
    Buttons.h
    Block.h
    EventBlocks.h
    ActionBlocks.h
    EventActionsSet.h
    Scene.h
    UsageLogger.h
    LogSignalMapper.h
    ThymioVPLStandalone.h
    ThymioVPLStandalone.cpp
)

target_link_libraries(thymiovpl
    asebaqtcommon
    qt-thymio-dm-client-lib
    asebaqtabout
    asebacommon
    Qt5::Help
    Qt5::Svg
    Qt5::Network
    Qt5::Xml
    Qt5::Widgets
)