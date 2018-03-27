
QT       += core gui network

TARGET = decklinkcapture
TEMPLATE = app

# For debugging, comment out
CONFIG-=warn_on
CONFIG+=release



SOURCES += decklinkcapture.cpp include/DeckLinkAPIDispatch.cpp \
	../../src/fsClient.cpp \
	../../src/suil.cpp \
	../../src/sageMessage.cpp \
	../../src/misc.cpp

HEADERS += decklinkcapture.h \
	include/DeckLinkAPI.h \
	../../sam/src/client/sam_client.h


# For Decklink SDK
INCLUDEPATH += include
# For SAGE
INCLUDEPATH += ../../include
# For SAM
INCLUDEPATH += ../../sam/src
INCLUDEPATH += ../../sam/src/client
# For QUANTA
INCLUDEPATH += ../../QUANTA/include

unix:*-g++*: QMAKE_CXXFLAGS += -fpermissive

macx {
    LIBS += 
    CONFIG -= app_bundle
}
linux-g++ {
    LIBS +=
}
linux-g++-64 {
    LIBS +=
}
# For SAGE
LIBS += -L../../lib -lquanta -lsail -lpthread -lm -ldl
# For SAM
LIBS += -L../../sam/lib -ljack -lsac 


# Enable  deinterlacing (comment the config line if you don't have ffmpeg)
#CONFIG += deinterlacing
deinterlacing {
	message(deinterlacing enabled)
	# FFMPEG (deinterlacing)
	DEFINES += __STDC_CONSTANT_MACROS DEINTERLACING
	LIBS += -lavcodec -lavutil -lswscale
}

target.path=../../bin
INSTALLS += target

message(Qt version: $$[QT_VERSION])
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(Qt resources can be found in the following locations:)
message(Header files: $$[QT_INSTALL_HEADERS])
message(Libraries: $$[QT_INSTALL_LIBS])

