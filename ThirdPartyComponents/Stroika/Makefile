export TOP_ROOT=$(shell realpath ../../)/

IntermediateFiles=		../../IntermediateFiles/

StroikaRoot=StroikaRoot/

include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk


all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building Stroika {$(CONFIGURATION)}:"
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=StroikaRoot --no-print-directory libraries CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building Stroika CONFIGURATION=$(CONFIGURATION)...done"

clobber:
	-@$(MAKE) --directory=StroikaRoot --no-print-directory --silent clobber CONFIGURATION=$(CONFIGURATION)
ifeq ($(CONFIGURATION),)
	@rm -rf StroikaRoot/ConfigurationFiles StroikaRoot/Builds StroikaRoot/IntermediateFiles
endif