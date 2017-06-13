#-------------------------------------------------
#
# Project created by QtCreator 2017-03-23T23:20:40
#
#-------------------------------------------------

QT  += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = kk
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11
    INCLUDEPATH += "src"
    LIBS ''= -lfftw3
    LIBS += -L$$PWD/src -lfftw3
SOURCES += src/main.cpp \
    src/recorder.cpp \
    src/mainwindow.cpp \
    src/wavFile.cpp \
    src/user.cpp \
    src/userwindow.cpp \
    src/adduserwindow.cpp \
	  src/calibrator.cpp \
    src/audiomodel.cpp

HEADERS  += \
    src/recorder.h \
    src/mainwindow.h \
    src/wavFile.h \
    src/user.h \
    src/userwindow.h \
    src/adduserwindow.h \
    src/audiomodel.h \
    src/calibrator.h \

win32:HEADERS+=src/fftw3.h
FORMS += \
    src/mainwindow.ui \
    src/userwindow.ui \
    src/adduserwindow.ui
    win32:DEFINES +=fftw3.dll

    DESTDIR = $$(PWD)
    message(The project will be installed in $$DESTDIR)
