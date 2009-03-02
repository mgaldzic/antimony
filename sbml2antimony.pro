######################################################################
# Automatically generated by qmake (2.01a) Wed Feb 25 19:18:50 2009
######################################################################

include(common.pri)

contains(DEFINES,NSBML){
	error("Cannot compile antimony2sbml without LIBSBML, but NSBML is defined")
}

TEMPLATE = app
TARGET = antimony2sbml
CONFIG -= qt 
CONFIG += console
DEPENDPATH += . src
INCLUDEPATH += . src

LIBS += -lantimony -Lbin

# Input
HEADERS += src/antimony_api.h \
           src/enums.h 
		   
SOURCES += src/antimony2sbml.cpp 
