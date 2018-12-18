export TOP_ROOT?=$(shell realpath ./)/
export StroikaRoot?=$(realpath ThirdPartyComponents/Stroika/StroikaRoot)/

include $(StroikaRoot)/ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)/Library/Projects/Unix/SharedMakeVariables-Default.mk



#Handy shortcut
CONFIGURATION_TAGS?=$(TAGS)

.PHONY: configurations

APPLY_CONFIGS::=$(or $(CONFIGURATION), $(shell $(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)"))

all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow all{$(CONFIGURATION)}:"
	@$(MAKE) -silent ConfigurationFiles MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@#Cannot use APPLY_CONFIGS here because ConfigurationFiles may have changed and evaluated before here
	@for i in `$(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)"` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@-$(MAKE) --directory=html --no-print-directory  MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=BackendApp --no-print-directory all MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@rm -rf Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html && mkdir -p Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow && cp -r html/dist/ Builds/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/html
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
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug-U-32 --arch x86 --config-tag Windows --config-tag 32 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-U-32 --arch x86 --config-tag Windows --config-tag 32 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-U-64 --arch x86_64 --config-tag Windows --config-tag 64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	else\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));\
		(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));\
	fi
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader.sh $(MAKE_INDENT_LEVEL) && $(ECHO)  Configuring...done


list-configurations list-configuration-tags:
	@$(MAKE) --directory ThirdPartyComponents/Stroika/StroikaRoot --silent CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)" $@


clean clobber:
ifeq ($(CONFIGURATION),)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine.sh $(MAKE_INDENT_LEVEL) "WhyTheFuckIsMyNetworkSoSlow $(call FUNCTION_CAPITALIZE_WORD,$@):"
ifeq ($(CONFIGURATION_TAGS),)
	@if [ "$@"=="clobber" ] ; then \
		rm -rf IntermediateFiles/* Builds/*;\
	fi
	@if [ "$@"=="clean" ] ; then \
		rm -rf IntermediateFiles/*;\
	fi
	@$(MAKE) --silent --directory html $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
	@# with no config specified, BackendApp NYI make clean/clobber (and not needed)
	@#$(MAKE) --silent --directory BackendApp $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
	@$(MAKE) --silent --directory ThirdPartyComponents $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
else
	@for i in $(APPLY_CONFIGS) ; do\
		$(MAKE) --no-print-directory $@ CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
endif
else
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine.sh $(MAKE_INDENT_LEVEL) "WhyTheFuckIsMyNetworkSoSlow $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(MAKE) --directory BackendApp --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory html --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory ThirdPartyComponents --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
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
