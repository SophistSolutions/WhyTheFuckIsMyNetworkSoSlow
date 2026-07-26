export TOP_ROOT=$(abspath ../../../../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir				=	$(TOP_ROOT)BackendApp/Sources/IntegratedModel/Private_
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/IntegratedModel/Private_/

include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk

vpath %cpp $(SrcDir)

CXXFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow)"


Objs	=	\
	$(ObjDir)DBAccess${OBJ_SUFFIX}\
	$(ObjDir)FromDiscovery${OBJ_SUFFIX}\
	$(ObjDir)RolledUpDevices${OBJ_SUFFIX}\
	$(ObjDir)RolledUpNetworkInterfaces${OBJ_SUFFIX}\
	$(ObjDir)RolledUpNetworks${OBJ_SUFFIX}\

all:$(ObjDir) $(Objs)
