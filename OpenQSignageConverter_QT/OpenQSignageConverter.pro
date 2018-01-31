#-------------------------------------------------
#
# Project created by QtCreator 2018-01-09T14:38:59
#
#-------------------------------------------------

QT       += core gui
QT += widgets serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OpenQSignageConverter
TEMPLATE = app
CONFIG += qt warn_on release

LIBS += -L C:\OpenCV-3.2.0\mingw\install\x86\mingw\lib\libopencv_*.a

INCLUDEPATH += C:\OpenCV-3.2.0\mingw\install\include\
               C:\OpenCV-3.2.0\mingw\install\include\opencv
               C:\OpenCV-3.2.0\mingw\install\include\opencv2


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
