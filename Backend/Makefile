export TOP_ROOT	:=	$(abspath ../)/
StroikaRoot		:=	$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/
SrcDir			:=	$(TOP_ROOT)BackendApp/


include $(StroikaRoot)Build/Lib/Make/SharedMakeVariables-Default.mk

include $(StroikaRoot)Build/Lib/Make/SharedBuildRules-Default.mk


SHELL=/bin/bash


all:
	@$(StroikaRoot)/Build/Scripts/CheckValidConfiguration $(CONFIGURATION)
	@### NO IDEA WHY MAKEFLAGS= is needed - @todo - DEBUG - but get link error otherwise
	@$(MAKE) --directory=Sources --no-print-directory all
	@mkdir -p $(TOP_ROOT)/Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/data/
	@cp $(TOP_ROOT)/Builds/$(CONFIGURATION)/ThirdPartyComponents/share/OSI-MAC-PREFIXES.txt $(TOP_ROOT)/Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/data/


clean clobber::
	@$(StroikaRoot)/Build/Scripts/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "WhyTheFuckIsMyNetworkSoSlow/Backend $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(StroikaRoot)/Build/Scripts/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=Sources --silent $@


format-code:
	@$(StroikaRoot)/Build/Scripts/FormatCode Sources *.h *.inl *.cpp
