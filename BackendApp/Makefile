export TOP_ROOT=$(shell realpath ../)/
StroikaPlatformTargetBuildDir=$(TOP_ROOT)Builds/$(CONFIGURATION)/

-include $(TOP_ROOT)IntermediateFiles/$(CONFIGURATION)/Library/Configuration.mk

SrcDir				=	$(TOP_ROOT)BackendApp/

-include $(StroikaRoot)/Library/Projects/Unix/SharedBuildRules-Default.mk
-include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

SHELL=/bin/bash


all:
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@### NO IDEA WHY MAKEFLAGS= is needed - @todo - DEBUG - but get link error otherwise
	@$(MAKE) --directory=Sources --no-print-directory all CONFIGURATION=$(CONFIGURATION) MAKEFLAGS=


clean clobber::
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "$@ HTML:"
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=Sources --no-print-directory $@


format-code:
	@../ThirdPartyComponents/Stroika/StroikaRoot/ScriptsLib/FormatCode.sh Sources *.h *.inl *.cpp
