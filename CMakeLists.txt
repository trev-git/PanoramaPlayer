cmake_minimum_required(VERSION 3.16)

project(PanoramaPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QMAKE_EXECUTABLE /usr/local/Qt6.8.3/bin/qmake)
set(QT_ROOT_DIR /usr/local/Qt6.8.3)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Multimedia SerialPort ShaderTools)

qt_standard_project_setup(REQUIRES 6.4)

qt_add_executable(panoramaplay
    src/main.cpp
    src/VideoRenderer.h src/VideoRenderer.cpp
    src/VideoFrameExt.h src/VideoFrameExt.cpp
)

qt_add_qml_module(panoramaplay
    URI PanoramaPlayer
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        Main.qml
    SOURCES
        src/ConfigReceiver.h src/ConfigReceiver.cpp
        src/PanoramaPlayer.h src/PanoramaPlayer.cpp
        src/PanoramaView.h src/PanoramaView.cpp
        src/SerialSensor.h src/SerialSensor.cpp
        src/SystemProcess.h src/SystemProcess.cpp
        src/Lerp.h src/Lerp.cpp
    RESOURCES
        icons/lens-mask.png
        icons/turn-around.png
)

qt6_add_resources(panoramaplay "shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX /
    FILES
        shaders/color.frag
        shaders/color.vert
        shaders/display.frag
        shaders/display.vert
        shaders/view.frag
        shaders/view.vert
)

target_include_directories(panoramaplay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(panoramaplay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KalmanFilter
)

target_link_libraries(panoramaplay PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::SerialPort
)

include(GNUInstallDirs)
install(TARGETS panoramaplay
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
