#!/bin/sh

export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/local/Qt6.8.3/plugins/platforms
export QT_QPA_PLATFORM=xcb
#export QT_QPA_EGLFS_INTEGRATION=eglfs_kms
export QT_QPA_EGLFS_KMS_CONFIG=/home/user/eglfs.json
#export QT_QPA_EGLFS_KMS_ATOMIC=1
#export QT_QPA_EGLFS_SWAPINTERVAL=0
export QT_QPA_EGLFS_DEPTH=24
export QT_QPA_EGLFS_FORCE888=1
export QT_QPA_EGLFS_FORCEVSYNC=0

#export QT_QPA_EGLFS_ROTATION=90
#export QT_QPA_EGLFSL_WIDTH=3840
#export QT_QPA_EGLFSL_HEIGHT=2160
#export QT_QPA_EGLFS_PHYSICAL_WIDTH=millimeters?
#export QT_QPA_EGLFS_PHYSICAL_HEIGHT=millimeters?
#export QT_QPA_FONTDIR=/usr/share/fonts/truetype
#export XDG_RUNTIME_DIR=/tmp
#export QT_LOGGING_RULES="qt.qpa.*=true"
#export QT_LOGGING_RULES="qt.qpa.egldeviceintegration=true"
#export QT_LOGGING_RULES="qt.qpa.input=true"
#export QT_LOGGING_RULES="qt.qpa.eglfs.kms=true"
#export QT_QPA_EGLFS_DEBUG=1
#export QT_DEBUG_PLUGINS=1
#export QT_MEDIA_BACKENG=gstreamer
#export QT_LOGGING_RULES="*.ffmpeg.*=true"
#export QT_LOGGING_RULES="*.multimedia.*=true"
#export QT_FFMPEG_DEBUG=1
#export QT_ENABLE_EXPERIMENTAL_CODECS=1
#export QT_FFMPEG_PROTOCOL_WHITELIST=file,crypto,rtp,udp
#export QT_FFMPEG_DECODING_HW_DEVICE_TYPES=drm,opencl
#export QT_DISABLE_HW_TEXTURES_CONVERSION=1
#export QT_FFMPEG_HW_ALLOW_PROFILE_MISMATCH=1

export LD_LIBRARY_PATH=/usr/local/Qt6.8.3/lib
/home/user/repos/PanoramaPlayer/build/panoramaplay -d >&2 DEBUG.txt
