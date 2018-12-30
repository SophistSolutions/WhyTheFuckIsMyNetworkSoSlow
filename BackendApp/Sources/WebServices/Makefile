export TOP_ROOT?=$(shell realpath ../../../)/
export StroikaRoot?=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk
endif

SrcDir				=	$(TOP_ROOT)BackendApp/Sources/Discovery/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/Discovery/

include $(StroikaRoot)/ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk


INCLUDES_PATH_COMPILER_DIRECTIVES				+=	-I$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/

vpath %cpp $(SrcDir)

Objs	=	\
	$(ObjDir)IWSAPI.o\
	$(ObjDir)Model.o\
	$(ObjDir)WSImpl.o\
	$(ObjDir)WebServer.o\

all:$(ObjDir) $(Objs)
