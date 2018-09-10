SHELL=/bin/bash

StroikaRoot=$(realpath ThirdPartyComponents/Stroika/StroikaRoot)/

include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow all{$(CONFIGURATION)}:"
	@$(MAKE) --directory=ThirdPartyComponents/Stroika --no-print-directory --silent configurations MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@for i in `$(StroikaRoot)ScriptsLib/GetConfigurations.sh` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=html --no-print-directory CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=BackendApp --no-print-directory all CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@rm -rf Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html && mkdir Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html && cp -r html/dist/* Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html
endif

clean:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Cleaning WhyTheFuckIsMyNetworkSoSlow{$(CONFIGURATION)}:"
	@$(MAKE) --directory=html --no-print-directory clean MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@for i in `$(StroikaRoot)/ScriptsLib/GetConfigurations.sh` ; do\
		$(MAKE) --no-print-directory --silent clean CONFIGURATION=$$i;\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clean
endif

clobber:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Clobbering WhyTheFuckIsMyNetworkSoSlow{$(CONFIGURATION)}:"
ifeq ($(CONFIGURATION),)
	@rm -rf Builds IntermediateFiles ConfigurationFiles $(StroikaRoot)/StroikaRoot/{Builds,IntermediateFiles,ConfigurationFiles}
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clobber MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
else
	@$(MAKE) --directory=html --no-print-directory clobber MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clobber MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory=BackendApp --no-print-directory clobber MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
endif

update-submodules:
	git submodule update --init --recursive

latest-submodules:
ifeq ($(BRANCH),)
	(cd $(StroikaRoot) && git checkout V2.1-Release && git pull)
else
	(cd $(StroikaRoot) && git checkout $(BRANCH) && git pull)
endif

format-code:
	@$(MAKE) --directory=BackendApp --no-print-directory format-code
