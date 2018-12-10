export TOP_ROOT?=$(shell realpath ../../)/
export StroikaRoot?=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
	include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk
endif


SrcDir				=	$(TOP_ROOT)BackendApp/Sources/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/BackendApp/

include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

Includes				+=	-I.


TARGETEXEDir			=	$(StroikaRoot)Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/
TARGETEXE				=	$(TARGETEXEDir)WhyTheFuckIsMyNetworkSoSlow-Backend


SubDirs=\
	Discovery \
	WebServices \

.PHONY: $(SubDirs)

$(SubDirs): $(ObjDir)
	@$(MAKE) --directory $@ -s


SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) --directory $(dir) -s list-objs))


Objs	=	\
	$(ObjDir)Main.o\
	$(ObjDir)Service.o\


all:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow/BackendApp:"
ifeq (Unix,$(ProjectPlatformSubdir))
	@mkdir -p $(TARGETEXEDir)
	@$(MAKE) --silent $(TARGETEXE) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) ECHO_BUILD_LINES=$(ECHO_BUILD_LINES)
else
	@if [[ "$(CONFIGURATION)" == "Debug-U-32" || "$(CONFIGURATION)" == "Release-U-32" || "$(CONFIGURATION)" == "Debug-U-64" || "$(CONFIGURATION)" == "Release-U-64" ]] ; then\
		perl $(StroikaRoot)ScriptsLib/RunArgumentsWithCommonBuildVars.pl $(CONFIGURATION) "MSBuild.exe /nologo /v:quiet /clp:NoSummary ../Projects/$(ProjectPlatformSubdir)/WhyTheFuckIsMyNetworkSoSlow-BackendApp.vcxproj /p:`$(StroikaRoot)ScriptsLib/GetVisualStudioConfigLine.pl $(CONFIGURATION)` /target:Build";\
	else\
		$(StroikaRoot)/ScriptsLib/PrintProgressLine.sh $$(($(MAKE_INDENT_LEVEL)+1)) "SKIPPED for unsupported configuration";\
	fi
endif


$(TARGETEXE):	$(ObjDir) $(SubDirs) $(Objs)
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Linking $(subst $(StroikaRoot),\$$StroikaRoot/, $(TARGETEXE))..."
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then\
	    $(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $$(($(MAKE_INDENT_LEVEL)+1)) && $(ECHO) "$(Linker) $(StroikaLinkerPrefixArgs) -o $(TARGETEXE) $(Objs) $(SUBDIR_OBJS) $(StroikaLinkerSuffixArgs)";\
	fi
	@$(Linker) $(StroikaLinkerPrefixArgs) -o $(TARGETEXE) $(Objs) $(SUBDIR_OBJS) $(StroikaLinkerSuffixArgs)


clean::
	@rm -f $(SUBDIR_OBJS)

