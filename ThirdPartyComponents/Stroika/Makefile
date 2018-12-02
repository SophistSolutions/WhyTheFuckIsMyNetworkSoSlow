export TOP_ROOT=$(shell realpath ../../)/

IntermediateFiles=		../../IntermediateFiles/

StroikaRoot=StroikaRoot/

include $(StroikaRoot)/ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk


all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Stroika-Import Build {$(CONFIGURATION)}:"
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=StroikaRoot --no-print-directory libraries CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building Stroika CONFIGURATION=$(CONFIGURATION)...done"

clean clobber:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Stroika-Import $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	-@$(MAKE) --directory=StroikaRoot --no-print-directory --silent $@ CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
ifeq ($@,clobber)
	@rm -rf StroikaRoot/ConfigurationFiles StroikaRoot/Builds StroikaRoot/IntermediateFiles
endif
endif
