export TOP_ROOT:=$(abspath ../../../)/
StroikaRoot	:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir		:=	$(TOP_ROOT)BackendApp/Sources/Common/
ObjDir		:=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/Common/

include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk

CPPFLAGS += -I"$(call FUNCTION_CONVERT_FILEPATH_TO_COMPILER_NATIVE,$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow)"

vpath %cpp $(SrcDir)

Objs	:=	\
	$(ObjDir)AppConfiguration${OBJ_SUFFIX}\
	$(ObjDir)BLOBMgr${OBJ_SUFFIX}\
	$(ObjDir)DB${OBJ_SUFFIX}\
	$(ObjDir)EthernetMACAddressOUIPrefixes${OBJ_SUFFIX}\
	$(ObjDir)GeoLocAndISPLookup${OBJ_SUFFIX}\
	$(ObjDir)GeoLocation${OBJ_SUFFIX}\
	$(ObjDir)InternetServiceProvider${OBJ_SUFFIX}\
	$(ObjDir)OperationalStatistics${OBJ_SUFFIX}\

all:$(ObjDir) $(Objs)


