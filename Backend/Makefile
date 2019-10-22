export TOP_ROOT=$(abspath ../)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifneq ($(CONFIGURATION),)
include $(StroikaRoot)IntermediateFiles/$(CONFIGURATION)/Configuration.mk
endif

include $(StroikaRoot)/ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedBuildRules-Default.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk

SrcDir				=	$(TOP_ROOT)BackendApp/


SHELL=/bin/bash


all:
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@### NO IDEA WHY MAKEFLAGS= is needed - @todo - DEBUG - but get link error otherwise
	@$(MAKE) --directory=Sources --no-print-directory all
	@mkdir -p $(TOP_ROOT)/Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/data/
	@cp $(TOP_ROOT)/Builds/$(CONFIGURATION)/ThirdPartyComponents/share/OSI-MAC-PREFIXES.txt $(TOP_ROOT)/Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/data/


clean clobber::
	@$(StroikaRoot)/ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "WhyTheFuckIsMyNetworkSoSlow/Backend $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=Sources --silent $@


format-code:
	@$(StroikaRoot)ScriptsLib/FormatCode Sources *.h *.inl *.cpp
