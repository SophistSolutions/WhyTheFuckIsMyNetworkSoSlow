TOP_ROOT=$(shell realpath ../../)/
####delete-StroikaPlatformTargetBuildDir=$(TOP_ROOT)Builds/$(CONFIGURATION)/

StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

-include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk

SrcDir				=	$(TOP_ROOT)BackendApp/Sources/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/BackendApp/

include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

Includes				+=	-I.

ifneq ($(CONFIGURATION),)
	ProjectPlatformSubdir	=	$(shell perl $(StroikaRoot)ScriptsLib/PrintConfigurationVariable.pl $(CONFIGURATION) ProjectPlatformSubdir)
endif

TARGETEXEDir			=	$(StroikaRoot)Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/
TARGETEXE				=	$(TARGETEXEDir)WhyTheFuckIsMyNetworkSoSlow-Backend


SubDirs=\
	Discovery \
	WebServices \

.PHONY: $(SubDirs)

SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) --directory $(dir) -s list-objs CONFIGURATION=$(CONFIGURATION)))


Objs	=	\
	$(ObjDir)Main.o\
	$(ObjDir)Service.o\


$(SubDirs):
	$(info subdirs)
	@for sd in $(SubDirs);\
	do\
		$(MAKE) --directory $$sd all;\
	done

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

