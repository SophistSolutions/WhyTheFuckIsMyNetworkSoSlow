IntermediateFiles=		../../IntermediateFiles/

StroikaRoot=StroikaRoot/

include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

STROIKA_CONFIG_PARAMS_COMMON=

STROIKA_CONFIG_PARAMS_DEBUG=--apply-default-debug-flags
STROIKA_CONFIG_PARAMS_RELEASE=--apply-default-release-flags


all: StroikaRoot/ConfigurationFiles
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building Stroika {$(CONFIGURATION)}:"
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=StroikaRoot --no-print-directory libraries CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building Stroika CONFIGURATION=$(CONFIGURATION)...done"

clobber:
	-@$(MAKE) --directory=StroikaRoot --no-print-directory --silent clobber CONFIGURATION=$(CONFIGURATION)
	@rm -rf StroikaRoot/ConfigurationFiles StroikaRoot/Builds StroikaRoot/IntermediateFiles
	@rm -rf ../../ConfigurationFiles

configurations:	StroikaRoot/ConfigurationFiles

StroikaRoot/ConfigurationFiles:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) Configuring...
	@export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd StroikaRoot && ./ScriptsLib/MakeBuildRoot.sh ../../../
	@if [ `uname` = "Darwin" ] ; then\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	elif [ `uname -o` = "Cygwin" ] ; then\
		make --directory=$(StroikaRoot) --no-print-directory  default-configurations MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	else\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	fi
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO)  Configuring...done
