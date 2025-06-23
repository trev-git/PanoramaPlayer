import QtCore
import QtQuick
import QtQuick.Controls
import QtMultimedia
import PanoramaPlayer

PanoramaView {
    id: panoramaView
    debugOpenGL: appDebugOpenGL

    readonly property string runIdleCommand: "backlight"
    readonly property int pitchIdleAngle: 30

    readonly property string cmdFullPath: StandardPaths.findExecutable(runIdleCommand).toString()
    readonly property var appWindow: Window.window

    Component.onCompleted: {
        if (!cmdFullPath && !errorLabel.text)
            errorLabel.text = qsTr("Can't find executable <b>%1</b> in the system PATH").arg(runIdleCommand)
    }

    SystemProcess {
        id: systemProcess
    }

    Shortcut {
        sequence: StandardKey.Cancel
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: "F11"
        context: Qt.ApplicationShortcut
        onActivated: {
            appWindow.visibility = (appWindow.visibility === Window.FullScreen) ?
                        Window.Windowed : Window.FullScreen
        }
    }

    /*ListModel {
        id: metaListModel
        function findName(name) {
            for (var i = 0; i < count; i++) {
                if (get(i).name === name) return i
            }
            return -1
        }
    }*/

    PanoramaPlayer {
        id: panoramaPlayer
        //source: appSourceUrl.toString() ? appSourceUrl : allVideoFiles("/srv/http/player")[0]
        source: configReceiver.videoSource
        audioOutput: AudioOutput { }
        videoOutput: panoramaView
        /*onMetaDataChanged: {
            var list = metaData.keys()
            for (var i = 0; i < list.length; i++) {
                var value = metaData.stringValue(list[i])
                if (!value) continue
                var name = metaData.metaDataKeyToString(list[i])
                var idx = metaListModel.findName(name)
                if (~idx) metaListModel.setProperty(idx, "value", value)
                else metaListModel.append({ "name": name, "value": value })
            }
        }*/
        onMediaStateChanged: {
            if (mediaState === PanoramaPlayer.MediaReady) {
                idleTimer.stop()
                systemProcess.startCommand(runIdleCommand + " 1")
                play()
            } else if (mediaState === PanoramaPlayer.MediaUnknown && runIdleCommand) {
                systemProcess.startCommand(runIdleCommand + " 0")
                stop()
            }
        }
    }

    onPitchAngleChanged: {
        if (pitchAngle < pitchIdleAngle && !panoramaPlayer.isPlaying() &&
                panoramaPlayer.mediaState !== PanoramaPlayer.MediaUnknown) {
            idleTimer.stop()
            systemProcess.startCommand(runIdleCommand + " 1")
            panoramaPlayer.play()
        }
    }

    Timer {
        id: idleTimer
        interval: configReceiver.screenSaver * 60000
        running: !pitchAngle || pitchAngle >= pitchIdleAngle
        onTriggered: {
            panoramaPlayer.stop()
            systemProcess.startCommand(runIdleCommand + " 0")
        }
    }

    SerialSensor {
        id: serialSensor
        active: true
        //portName: "ttyUSB0"
        portName: "ttyAMA0"
        enableCompass: configReceiver.enableCompass
        adjustAngle: configReceiver.adjustAngle
        smoothingFactor: configReceiver.smoothingFactor
        //Component.onCompleted: print(allPortNames())
    }
    pitchAngle: serialSensor.pitchAngle
    yawAngle: serialSensor.yawAngle

    ConfigReceiver {
        id: configReceiver
        active: true
        udpPort: 12345
        watchForVideo:     "/srv/http/current_video.txt"
        watchForRotate:    "/srv/http/rotate.txt"
        watchForCompass:   "/srv/http/compass.txt"
        watchForSaver:     "/srv/http/screensaver.txt"
        watchForSmoothing: "/srv/http/smoothing.txt"
        onCalibrateRequested: {
            serialSensor.startCalibrate();
        }
    }
    rotateDisplay: configReceiver.rotateDisplay
    stereoShift: configReceiver.stereoShift
    fovAngle: configReceiver.fovAngle

    readonly property int rotateImage: rotateDisplay < 0 ? 270 : 90 * rotateDisplay

    Image {
        anchors.centerIn: parent
        source: "qrc:/PanoramaPlayer/icons/lens-mask.png"
        visible: !serialSensor.calibrating
        rotation: panoramaView.rotateImage
    }

    Image {
        anchors.centerIn: parent
        source: "qrc:/PanoramaPlayer/icons/turn-around.png"
        visible: serialSensor.calibrating
        rotation: panoramaView.rotateImage
    }

    Dialog {
        modal: true
        anchors.centerIn: parent //Overlay.overlay
        width: Math.max(320, errorLabel.width + leftPadding + rightPadding)
        title: qsTr("Media player error")
        standardButtons: Dialog.Close
        visible: errorLabel.text
        Label {
            id: errorLabel
            text: (panoramaPlayer.errorText ? panoramaPlayer.errorText + "\n\n" : "") +
                  (serialSensor.errorText ? serialSensor.errorText + "\n\n" : "") +
                  panoramaView.errorText
        }
        onRejected: Qt.quit()
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: Qt.quit()
    }
}
