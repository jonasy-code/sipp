# qmake project for the interactive animation demo.  See README.
#
#   qmake && make        (qmake from Homebrew: /opt/homebrew/bin/qmake)

QT += core gui widgets
CONFIG += c++17 release warn_on
CONFIG -= app_bundle debug_and_release

# libsipp.a is built for the host macOS version; build for the same one
# (Qt itself defaults to an older minimum), and accept the SDK Qt was
# not tested with.
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = $$system(sw_vers -productVersion)
    CONFIG += sdk_no_version_check
}
TARGET = qtanim
TEMPLATE = app

INCLUDEPATH += ../../libsipp ../animation

SOURCES += main.cpp renderthread.cpp animview.cpp ../animation/scene.c
HEADERS += renderthread.h animview.h ../animation/scene.h

# The scene is C99, like the rest of SIPP.
QMAKE_CFLAGS += -std=gnu99 -O3

LIBS += ../../libsipp/libsipp.a -lm -lpthread
PRE_TARGETDEPS += ../../libsipp/libsipp.a
