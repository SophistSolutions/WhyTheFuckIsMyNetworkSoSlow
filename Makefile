export TOP_ROOT=$(abspath ./)/
StroikaRoot=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifeq (,$(wildcard $(StroikaRoot)Makefile))
$(error  "submodules missing: perhaps you should run `git submodule update --init --recursive`")
endif



include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk



#Handy shortcut
CONFIGURATION_TAGS?=$(TAGS)


APPLY_CONFIGS::=$(or $(CONFIGURATION), $(shell $(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)"))

all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow all{$(CONFIGURATION)}:"
	@$(MAKE) -silent ConfigurationFiles MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@#Cannot use APPLY_CONFIGS here because ConfigurationFiles may have changed and evaluated before here
	@for i in `$(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)"` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=html --no-print-directory  MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=Backend --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --silent installers MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
endif


ConfigurationFiles:
	@$(MAKE) -silent default-configurations MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))


STROIKA_CONFIG_PARAMS_COMMON=

STROIKA_CONFIG_PARAMS_DEBUG=--apply-default-debug-flags
STROIKA_CONFIG_PARAMS_RELEASE=--apply-default-release-flags

.PHONY: default-configurations
default-configurations:
	@make -s build-root
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) Configuring...
ifeq ($(shell uname), Darwin)
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
else ifeq ($(shell uname -o), Cygwin)
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug-U-64 --arch x86_64 --config-tag Windows --config-tag 64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-U-32 --arch x86 --config-tag Windows --config-tag 32 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-U-64 --arch x86_64 --config-tag Windows --config-tag 64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-Logging-U-64 --arch x86_64 --config-tag Windows --config-tag 64 --trace2file enable $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
else
	@#try g++8 and then 9 then 10; if both all take more recent by overwriting configuration;
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix --only-if-has-compiler --compiler-driver 'g++' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix --only-if-has-compiler --compiler-driver 'g++' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-8' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-8' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-9' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-9' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-10' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-10' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-release --config-tag Unix --config-tag raspberrypi --apply-default-release-flags --only-if-has-compiler --compiler-driver 'arm-linux-gnueabihf-g++-8' --cross-compiling true);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-debug --config-tag Unix --config-tag raspberrypi --apply-default-debug-flags --only-if-has-compiler --trace2file enable --compiler-driver 'arm-linux-gnueabihf-g++-8' --cross-compiling true --append-CXXFLAGS -Wno-psabi);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-release --config-tag Unix --config-tag raspberrypi --apply-default-release-flags --only-if-has-compiler --compiler-driver 'arm-linux-gnueabihf-g++-9' --cross-compiling true);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-debug --config-tag Unix --config-tag raspberrypi --apply-default-debug-flags --only-if-has-compiler --trace2file enable --compiler-driver 'arm-linux-gnueabihf-g++-9' --cross-compiling true --append-CXXFLAGS -Wno-psabi);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-release --config-tag Unix --config-tag raspberrypi --apply-default-release-flags --only-if-has-compiler --compiler-driver 'arm-linux-gnueabihf-g++-10' --cross-compiling true);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-debug --config-tag Unix --config-tag raspberrypi --apply-default-debug-flags --only-if-has-compiler --trace2file enable --compiler-driver 'arm-linux-gnueabihf-g++-10' --cross-compiling true --append-CXXFLAGS -Wno-psabi);
endif
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO)  Configuring...done

build-root:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) Making BuildRoot...
	@export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd ThirdPartyComponents/Stroika/StroikaRoot && ./ScriptsLib/MakeBuildRoot ../../../

list-configurations list-configuration-tags:
	@$(MAKE) --directory ThirdPartyComponents/Stroika/StroikaRoot --silent CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)" $@


clean clobber:
ifeq ($(CONFIGURATION),)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "WhyTheFuckIsMyNetworkSoSlow $(call FUNCTION_CAPITALIZE_WORD,$@):"
ifeq ($(CONFIGURATION_TAGS),)
	@if [ "$@"=="clobber" ] ; then \
		rm -rf IntermediateFiles/* Builds/*;\
	fi
	@if [ "$@"=="clean" ] ; then \
		rm -rf IntermediateFiles/*;\
	fi
	@$(MAKE) --silent --directory html $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
	@# with no config specified, Backend NYI make clean/clobber (and not needed)
	@#$(MAKE) --silent --directory Backend $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
	@$(MAKE) --silent --directory ThirdPartyComponents $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));
else
	@for i in $(APPLY_CONFIGS) ; do\
		$(MAKE) --no-print-directory $@ CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
endif
else
	@$(StroikaRoot)ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "WhyTheFuckIsMyNetworkSoSlow $(call FUNCTION_CAPITALIZE_WORD,$@) {$(CONFIGURATION)}:"
	@$(MAKE) --directory Backend --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory html --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(MAKE) --directory ThirdPartyComponents --no-print-directory $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
endif


.PHONY: installers installer-deb installer-rpm installer-wix
installers installer-deb installer-rpm installer-wix:   $(TARGETEXE)
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Building Installers:"
	@$(MAKE) --no-print-directory --directory Installers MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@


update-submodules:
	git submodule update --init --recursive


latest-submodules:
ifeq ($(BRANCH),)
	(cd $(StroikaRoot) && git checkout V2.1-Release && git pull)
else
	(cd $(StroikaRoot) && git checkout $(BRANCH) && git pull)
endif


format-code:
	@$(MAKE) --directory=Backend --no-print-directory format-code


INSTALLER_TAG=$(shell $(StroikaRoot)/ScriptsLib/ExtractVersionInformation VERSION FullVersionString)
define CopyFileIf_
	if [ -e Builds/${1}/WhyTheFuckIsMyNetworkSoSlow/${2} ] ; then\
    	cp Builds/${1}/WhyTheFuckIsMyNetworkSoSlow/${2} Release-$(INSTALLER_TAG)/${3};\
		echo "Added Release-$(INSTALLER_TAG)/${3}";\
	else\
		echo "Skipped Release-$(INSTALLER_TAG)/${3}";\
	fi
endef
release-directory:
	@rm -rf Release-$(INSTALLER_TAG) && mkdir Release-$(INSTALLER_TAG)
	@$(call CopyFileIf_,Debug,whythefuckismynetworksoslow-$(INSTALLER_TAG).Linux.x86_64.deb,whythefuckismynetworksoslow-$(INSTALLER_TAG)-Debug.Linux.x86_64.deb)
	@$(call CopyFileIf_,Release,whythefuckismynetworksoslow-$(INSTALLER_TAG).Linux.x86_64.deb,whythefuckismynetworksoslow-$(INSTALLER_TAG).Linux.x86_64.deb)
	@$(call CopyFileIf_,raspberrypi-debug,whythefuckismynetworksoslow-$(INSTALLER_TAG).Linux.armhf.deb,whythefuckismynetworksoslow-$(INSTALLER_TAG)-Debug.Linux.armhf.deb)
	@$(call CopyFileIf_,raspberrypi-release,whythefuckismynetworksoslow-$(INSTALLER_TAG).Linux.armhf.deb,whythefuckismynetworksoslow-$(INSTALLER_TAG).Linux.armhf.deb)
	@$(call CopyFileIf_,Debug-U-64,WhyTheFuckIsMyNetworkSoSlow-Windows-x86_64-Debug-U-64.msi,WhyTheFuckIsMyNetworkSoSlow-$(INSTALLER_TAG)-Debug.Windows.x86_64.msi)
	@$(call CopyFileIf_,Release-U-32,WhyTheFuckIsMyNetworkSoSlow-Windows-x86-Release-U-32.msi,WhyTheFuckIsMyNetworkSoSlow-$(INSTALLER_TAG).Windows.x86.msi)
	@$(call CopyFileIf_,Release-U-64,WhyTheFuckIsMyNetworkSoSlow-Windows-x86_64-Release-U-64.msi,WhyTheFuckIsMyNetworkSoSlow-$(INSTALLER_TAG).Windows.x86_64.msi)
