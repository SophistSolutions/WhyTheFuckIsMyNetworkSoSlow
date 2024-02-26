export TOP_ROOT:=$(abspath ../../../)/
StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

SrcDir		:=	$(TOP_ROOT)BackendApp/Sources/Discovery/
ObjDir		:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/WebServices/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk


CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/)"

vpath %cpp $(SrcDir)

Objs	:=	\
	$(ObjDir)IWSAPI${OBJ_SUFFIX}\
	$(ObjDir)JSONPATCH${OBJ_SUFFIX}\
	$(ObjDir)Model${OBJ_SUFFIX}\
	$(ObjDir)WSImpl${OBJ_SUFFIX}\
	$(ObjDir)WebServer${OBJ_SUFFIX}\

all:$(ObjDir) $(Objs)
