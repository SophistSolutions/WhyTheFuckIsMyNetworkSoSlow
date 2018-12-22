export TOP_ROOT?=$(shell realpath ../../../)/
export StroikaRoot?=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk
endif

SrcDir				=	$(TOP_ROOT)BackendApp/Sources/Common/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/Common/

include $(StroikaRoot)/ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk


Includes				+=	-I.

vpath %cpp $(SrcDir)

Objs	=	\
	$(ObjDir)GEOLocAndISPLookup.o\
	$(ObjDir)GEOLocation.o\
	$(ObjDir)InternetServiceProvider.o\

all:$(ObjDir) $(Objs)


