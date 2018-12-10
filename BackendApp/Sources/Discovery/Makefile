export TOP_ROOT?=$(shell realpath ../../../)/
export StroikaRoot?=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk
endif

SrcDir				=	$(TOP_ROOT)BackendApp/Sources/Discovery/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/BackendApp/Discovery/

include $(StroikaRoot)/ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk


Includes				+=	-I.

vpath %cpp $(SrcDir)

Objs	=	\
	$(ObjDir)Devices.o\
	$(ObjDir)NetworkInterfaces.o\
	$(ObjDir)Networks.o\

all:$(ObjDir) $(Objs)


