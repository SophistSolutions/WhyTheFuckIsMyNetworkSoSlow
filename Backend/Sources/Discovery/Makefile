export TOP_ROOT:=$(abspath ../../../)/
StroikaRoot :=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir		:=	$(TOP_ROOT)BackendApp/Sources/Discovery/
ObjDir		:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/Discovery/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk

CPPFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow)"

vpath %cpp $(SrcDir)

Objs	:=	\
	$(ObjDir)Devices${OBJ_SUFFIX}\
	$(ObjDir)NetworkInterfaces${OBJ_SUFFIX}\
	$(ObjDir)Networks${OBJ_SUFFIX}\
	$(ObjDir)PortScanner${OBJ_SUFFIX}\

all:$(ObjDir) $(Objs)


