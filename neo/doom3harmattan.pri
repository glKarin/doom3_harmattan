
#CONFIG += debug_and_release

CONFIG += link_pkgconfig
QT -= gui core

CONFIG -= warn_on

QMAKE_CXXFLAGS += -ffast-math -Wno-psabi -fno-unsafe-math-optimizations
#-mfloat-abi=softfp

QMAKE_CXXFLAGS += -fvisibility=hidden -fpermissive -Wno-unknown-pragmas
QMAKE_CXXFLAGS += -O3 \
-march=armv7-a -mcpu=cortex-a8 -mfpu=neon #-mthumb
# QMAKE_CXXFLAGS += -g

#QMAKE_CXXFLAGS += -mfpu=neon

DEFINES += _HARMATTAN
DEFINES += _HARMATTAN_GL
DEFINES += _HARMATTAN_PLUS
DEFINES += _HARMATTAN_OPTMIZE
DEFINES += _HARMATTAN_PKG=\\\"doom3\\\"
DEFINES += _HARMATTAN_VER=\\\"harmattan1\\\"
DEFINES += _HARMATTAN_APPNAME=\\\"DOOM_III_Harmattan\\\"
DEFINES += _HARMATTAN_DEV=\\\"karin\\\"
DEFINES += _HARMATTAN_DEVCODE=\\\"Katyusha\\\"
DEFINES += _HARMATTAN_DESC=\\\"DOOM_III_for_OpenGLES_2.0_on_MeeGo_Harmattan\\\"
DEFINES += _HARMATTAN_RELEASE=\\\"20180712\\\"
DEFINES += _HARMATTAN_TMO=\\\"Karin_Zhao\\\"
DEFINES += _HARMATTAN_PATCH=\\\"1\\\"
DEFINES += _HARMATTAN_RESC=\\\"./resc/\\\"
#DEFINES += _HARMATTAN_RESC=\\\"/usr/lib/doom3/resc/\\\"
DEFINES += _HARMATTAN_SAILFISH

# temp
DEFINES += __ANDROID__
DEFINES += GL_ES_VERSION_2_0

DEFINES += _HARMATTAN_CPU=\\\"arm\\\"
DEFINES += _HARMATTAN_CONSOLE_KEY=\\\'\\`\\\'
DEFINES += _HARMATTAN_GPU_MEM=64

# DEFINES -= QT_NO_DEBUG

#DEFINES += ID_OPENAL=1
