TOP_ROOT=$(shell realpath ../../../)/
StroikaPlatformTargetBuildDir=$(TOP_ROOT)Builds/$(CONFIGURATION)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

-include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk

SrcDir				=	$(TOP_ROOT)BackendApp/Sources/Discovery/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/BackendApp/Discovery/

include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

Includes				+=	-I.

vpath %cpp $(SrcDir)

Objs	=	\
	$(ObjDir)IWSAPI.o\
	$(ObjDir)Model.o\
	$(ObjDir)WSImpl.o\
	$(ObjDir)WebServer.o\

all:$(ObjDir) $(Objs)
