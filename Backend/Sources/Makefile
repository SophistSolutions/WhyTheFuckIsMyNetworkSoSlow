export TOP_ROOT?=$(shell realpath ../../)/
export StroikaRoot?=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk
endif


SrcDir				=	$(TOP_ROOT)Backend/Sources/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend/


include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

CXXFLAGS				+=	-I$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/


TARGETEXEDir			=	$(StroikaRoot)Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/
TARGETEXE				=	$(TARGETEXEDir)WhyTheFuckIsMyNetworkSoSlow-Backend


SubDirs=\
	Common \
	Discovery \
	WebServices \

.PHONY: $(SubDirs)


$(SubDirs): $(ObjDir)
	@$(MAKE) --directory $@ -s


SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) --directory $(dir) -s list-objs))


Objs	=	\
	$(ObjDir)Main.o\
	$(ObjDir)Service.o\



VERSION_FILE_IN=$(TOP_ROOT)VERSION
VERSION_FILE_OUT=$(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/AppVersion.h
$(VERSION_FILE_OUT):	$(VERSION_FILE_IN)
	@mkdir -p $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow
	@$(StroikaRoot)ScriptsLib/MakeVersionFile $(VERSION_FILE_IN) $(VERSION_FILE_OUT) AppCPlusPlusVersion kAppVersion __WhyTheFuckIsMyNetworkSoSlow_AppVersion_H__


$(ObjDir)Main.o $(ObjDir)Service.o:		$(VERSION_FILE_OUT)

all:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow/Backend:"
ifeq (Unix,$(ProjectPlatformSubdir))
	@mkdir -p $(TARGETEXEDir)
	@$(MAKE) --silent $(TARGETEXE) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) ECHO_BUILD_LINES=$(ECHO_BUILD_LINES)
else
	@$(MAKE) --silent $(VERSION_FILE_OUT)
	@if [[ "$(CONFIGURATION)" == "Debug-U-32" || "$(CONFIGURATION)" == "Release-U-32" || "$(CONFIGURATION)" == "Debug-U-64" || "$(CONFIGURATION)" == "Release-U-64" ]] ; then\
		perl $(StroikaRoot)ScriptsLib/RunArgumentsWithCommonBuildVars $(CONFIGURATION) "MSBuild.exe /nologo /v:quiet /clp:NoSummary ../Projects/$(ProjectPlatformSubdir)/WhyTheFuckIsMyNetworkSoSlow-Backend.vcxproj /p:`$(StroikaRoot)ScriptsLib/GetVisualStudioConfigLine $(CONFIGURATION)` /target:Build";\
	else\
		$(StroikaRoot)/ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) "SKIPPED for unsupported configuration";\
	fi
endif




$(TARGETEXE):	$(ObjDir) $(SubDirs) $(Objs)
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Linking $(subst $(StroikaRoot),\$$StroikaRoot/, $(TARGETEXE))..."
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then\
	    $(StroikaRoot)/ScriptsLib/PrintLevelLeader $$(($(MAKE_INDENT_LEVEL)+1)) && $(ECHO) "$(Linker) $(StroikaLinkerPrefixArgs) -o $(TARGETEXE) $(Objs) $(SUBDIR_OBJS) $(StroikaLinkerSuffixArgs)";\
	fi
	@$(Linker) $(StroikaLinkerPrefixArgs) -o $(TARGETEXE) $(Objs) $(SUBDIR_OBJS) $(StroikaLinkerSuffixArgs)





clean::
	@rm -f $(SUBDIR_OBJS)

