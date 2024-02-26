export TOP_ROOT:=$(abspath ../../../)/
StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

SrcDir		:=	$(TOP_ROOT)BackendApp/Sources/Discovery/
ObjDir		:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/IntegratedModel/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

vpath %cpp $(SrcDir)

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/)"


SubDirs=\
	Private_		\

Objs	=	\
	$(ObjDir)Mgr${OBJ_SUFFIX}\

all:$(ObjDir) $(Objs)
