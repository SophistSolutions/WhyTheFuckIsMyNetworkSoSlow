TOP_ROOT=$(shell realpath ../../)/
StroikaPlatformTargetBuildDir=$(TOP_ROOT)Builds/$(CONFIGURATION)/

include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk

SrcDir				=	$(TOP_ROOT)BackendApp/Sources/
ObjDir				=	$(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/BackendApp/

include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

Includes				+=	-I.


TARGETEXE				=	$(StroikaRoot)Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow-Backend


SubDirs=\
	Discovery \
	WebServices \

.PHONY: $(SubDirs)

SUBDIR_OBJS := $(foreach dir, $(SubDirs), $(shell $(MAKE) --directory $(dir) -s list-objs CONFIGURATION=$(CONFIGURATION)))


Objs	=	\
	$(ObjDir)Main.o\


$(SubDirs):
	@for sd in $(SubDirs);\
	do\
		$(MAKE) --directory $$sd all CONFIGURATION=$(CONFIGURATION);\
	done

all:
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow/BackendApp:"
	@$(MAKE) --silent $(TARGETEXE) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) ECHO_BUILD_LINES=$(ECHO_BUILD_LINES)


$(TARGETEXE):	$(ObjDir) $(SubDirs) $(Objs)
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Linking $(subst $(StroikaRoot),\$$StroikaRoot/, $(TARGETEXE))..."
	@if [ $(ECHO_BUILD_LINES) -eq 1 ]; then\
	    $(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $$(($(MAKE_INDENT_LEVEL)+1)) && $(ECHO) "$(Linker) $(LinkerPrefixArgs) -o $(TARGETEXE) $(Objs) $(SUBDIR_OBJS) $(StroikaLinkerArgs) $(StroikaLibsWithSupportLibs)";\
	fi
	@$(Linker) $(LinkerPrefixArgs) -o $(TARGETEXE) $(Objs) $(SUBDIR_OBJS) $(StroikaLinkerArgs) $(StroikaLibsWithSupportLibs)


clean::
	@rm -f $(SUBDIR_OBJS)
