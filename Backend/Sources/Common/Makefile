export TOP_ROOT=$(abspath ../../../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	-include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

SrcDir				=	$(TOP_ROOT)BackendApp/Sources/Common/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/Common/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

vpath %cpp $(SrcDir)

Objs	=	\
	$(ObjDir)AppConfiguration${OBJ_SUFFIX}\
	$(ObjDir)BLOBMgr${OBJ_SUFFIX}\
	$(ObjDir)DB${OBJ_SUFFIX}\
	$(ObjDir)EthernetMACAddressOUIPrefixes${OBJ_SUFFIX}\
	$(ObjDir)GeoLocAndISPLookup${OBJ_SUFFIX}\
	$(ObjDir)GeoLocation${OBJ_SUFFIX}\
	$(ObjDir)InternetServiceProvider${OBJ_SUFFIX}\

all:$(ObjDir) $(Objs)


