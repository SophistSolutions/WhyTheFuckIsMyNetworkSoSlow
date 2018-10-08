SHELL=/bin/bash

StroikaRoot=$(realpath ThirdPartyComponents/Stroika/StroikaRoot)/

include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk

#Handy shortcut
CONFIGURATION_TAGS?=$(TAGS)

.PHONY: configurations


all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow all{$(CONFIGURATION)}:"
	@$(MAKE) -silent ConfigurationFiles MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@for i in `$(MAKE) --silent list-configurations  CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)"` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=html --no-print-directory CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=BackendApp --no-print-directory all CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@rm -rf Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html && mkdir Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html && cp -r html/dist/* Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html
endif

ConfigurationFiles:
	@$(MAKE) -silent configurations  MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))

STROIKA_CONFIG_PARAMS_COMMON=

STROIKA_CONFIG_PARAMS_DEBUG=--apply-default-debug-flags
STROIKA_CONFIG_PARAMS_RELEASE=--apply-default-release-flags

configurations:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) Configuring...
	@export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd ThirdPartyComponents/Stroika/StroikaRoot && ./ScriptsLib/MakeBuildRoot.sh ../../../
	@if [ `uname` = "Darwin" ] ; then\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	elif [ `uname -o` = "Cygwin" ] ; then\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug-U-32 --config-tag Windows --config-tag 32 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-U-32 --config-tag Windows --config-tag 32 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-U-64 --config-tag Windows --config-tag 64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	else\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	fi
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO)  Configuring...done


list-configurations list-configuration-tags:
	@$(MAKE) --directory ThirdPartyComponents/Stroika/StroikaRoot --silent CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)" $@


clean:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Cleaning WhyTheFuckIsMyNetworkSoSlow{$(CONFIGURATION)}:"
	@$(MAKE) --directory=html --no-print-directory clean MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@for i in `$(StroikaRoot)/ScriptsLib/GetConfigurations.sh  --config-tags "$(CONFIGURATION_TAGS)"` ; do\
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
