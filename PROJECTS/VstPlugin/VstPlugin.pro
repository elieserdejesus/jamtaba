!include( ../Jamtaba-common.pri ) {
    error( "Couldn't find the Jamtaba-common.pri file!" )
}

#CONFIG += qtwinmigrate-uselib
!include(../qtwinmigrate/src/qtwinmigrate.pri) {
    error( "Couldn't find the qtwinmigrate common.pri file!" )
}

QT       += core gui network widgets

TARGET = "JamtabaVST"
TEMPLATE = lib
CONFIG += shared

win32-msvc* {
    DEFINES += _CRT_SECURE_NO_WARNINGS
    #RC_FILE = vstdll.rc

    #supressing warning about missing .pdb files
    QMAKE_LFLAGS += /ignore:4099
}

INCLUDEPATH += $$SOURCE_PATH/Common
INCLUDEPATH += $$SOURCE_PATH/VstPlugin

INCLUDEPATH += $$ROOT_PATH/libs/includes/ogg
INCLUDEPATH += $$ROOT_PATH/libs/includes/vorbis
INCLUDEPATH += $$ROOT_PATH/libs/includes/minimp3

win32{
    INCLUDEPATH += "$$VST_SDK_PATH/"
    INCLUDEPATH += "$$VST_SDK_PATH/pluginterfaces/vst2.x/"
    INCLUDEPATH += "$$VST_SDK_PATH/public.sdk/source/vst2.x"
}

VPATH += $$SOURCE_PATH/Common
VPATH += $$SOURCE_PATH/VstPlugin

DEPENDPATH +=  $$ROOT_PATH/libs/includes/ogg
DEPENDPATH +=  $$ROOT_PATH/libs/includes/vorbis
DEPENDPATH +=  $$ROOT_PATH/libs/includes/minimp3

HEADERS += Plugin.h
HEADERS += Editor.h
HEADERS += MainControllerVST.h
HEADERS += NinjamControllerVST.h
HEADERS += NinjamRoomWindowVST.h
HEADERS += MainWindowVST.h
HEADERS += KeyboardHook.h
HEADERS += VstPreferencesDialog.h

SOURCES += main.cpp
SOURCES += Plugin.cpp
SOURCES += Editor.cpp
SOURCES += MainControllerVST.cpp
SOURCES += ConfiguratorVST.cpp
SOURCES += NinjamRoomWindowVST.cpp
SOURCES += NinjamControllerVST.cpp
SOURCES += MainWindowVST.cpp
SOURCES += KeyboardHook.cpp
SOURCES += VstPreferencesDialog.cpp
SOURCES += $$VST_SDK_PATH/public.sdk/source/vst2.x/audioeffectx.cpp
SOURCES += $$VST_SDK_PATH/public.sdk/source/vst2.x/audioeffect.cpp
#SOURCES += audio/samplesbufferrecorder.cpp
#SOURCES += audio/core/PluginDescriptor.cpp


win32 {
    message("Windows VST build")

    LIBS +=  -lwinmm -lole32 -lws2_32 -lAdvapi32 -lUser32 #-lPsapi
    #performance monitor lib
    #QMAKE_CXXFLAGS += -DPSAPI_VERSION=1

    !contains(QMAKE_TARGET.arch, x86_64) {
        message("x86 build") ## Windows x86 (32bit) specific build here
        win32-msvc*{
            LIBS_PATH = "static/win32-msvc"
        }
        win32-g++{
            LIBS_PATH = "static/win32-mingw"
        }

    } else {
        message("x86_64 build") ## Windows x64 (64bit) specific build here
        LIBS_PATH = "static/win64-msvc"
    }
    message("Libs path: " $$LIBS_PATH)

    win32-msvc*{ #microsoft compilers
        #+++++++++++++ link windows platform statically +++++++++++++++++++++++++++++++++++
        #release platform libs
        CONFIG(release, debug|release): LIBS += -lQt5PlatformSupport
        CONFIG(release, debug|release): LIBS += -L$(QTDIR)\plugins\platforms\ -lqwindows #link windows platform statically

        #debug platform libs
        CONFIG(debug, debug|release): LIBS += -lQt5PlatformSupportd #link windows platform statically
        CONFIG(debug, debug|release): LIBS += -L$(QTDIR)\plugins\platforms\ -lqwindowsd #link windows platform statically
        #++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


        CONFIG(release, debug|release): LIBS += -L$$PWD/../../libs/$$LIBS_PATH/ -lminimp3 -lvorbisfile -lvorbis -logg
        else:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../libs/$$LIBS_PATH/ -lminimp3d -lvorbisfiled -lvorbisd -loggd

        message("Using MSVC libs")
        CONFIG(release, debug|release) {
            #ltcg - http://blogs.msdn.com/b/vcblog/archive/2009/02/24/quick-tips-on-using-whole-program-optimization.aspx
            QMAKE_CXXFLAGS_RELEASE +=  -GL
            QMAKE_LFLAGS_RELEASE += /LTCG
        }
    }
    win32-g++{ #mingw
        message("Using mingw libs")

        LIBS += -L$$PWD/../../libs/$$LIBS_PATH -lminimp3 -lvorbisfile -lvorbisenc -lvorbis -logg

        #platform libs
        LIBS += -lQt5PlatformSupport
        LIBS += -L$(QTDIR)\plugins\platforms\ -lqwindows #link windows platform statically
    }
}
