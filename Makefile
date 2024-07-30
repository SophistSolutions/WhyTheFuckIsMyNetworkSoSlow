export TOP_ROOT:=$(abspath ./)/
StroikaRoot:=$(TOP_ROOT)ThirdPartyComponents/Stroika/StroikaRoot/

ifeq (,$(wildcard $(StroikaRoot)Makefile))
$(warning "*** Submodules missing: perhaps you should run `git submodule update --init --recursive` ***")
endif



include $(StroikaRoot)ScriptsLib/Makefile-Common.mk
include $(StroikaRoot)ScriptsLib/SharedMakeVariables-Default.mk


#not parallel because submakefiles use parallelism, but generally best to sequence these top level requests. Like if you say
# make clobber all you don't want those to happen at the same time. And make libraries samples wouldn't really work since all the libraries
# have to be built before the samples etc...
.NOTPARALLEL:

#Handy shortcut
CONFIGURATION_TAGS?=$(TAGS)

APPLY_CONFIGS=$(or \
				$(CONFIGURATION), \
				$(if $(CONFIGURATION_TAGS), \
					$(shell $(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)"),\
					$(if $(filter clobber, $(MAKECMDGOALS)),\
						$(shell $(StroikaRoot)ScriptsLib/GetConfigurations --all --quiet),\
						$(shell $(StroikaRoot)ScriptsLib/GetConfigurations --all-default)\
					)\
				)\
			)

all:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Building WhyTheFuckIsMyNetworkSoSlow all{$(CONFIGURATION)}:"
	@$(MAKE) -silent IntermediateFiles/ASSURE_DEFAULT_CONFIGURATIONS_BUILT MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
ifeq ($(CONFIGURATION),)
	@#Cannot use APPLY_CONFIGS here because ConfigurationFiles may have changed and evaluated before here
	@for i in `$(StroikaRoot)ScriptsLib/GetConfigurations --config-tags "$(CONFIGURATION_TAGS)" --all-default` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1));\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	@$(StroikaRoot)/ScriptsLib/CheckValidConfiguration $(CONFIGURATION)
	@$(MAKE) --directory=html --no-print-directory  MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --directory=Backend --no-print-directory MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) all
	@$(MAKE) --silent installers MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
endif


IntermediateFiles/ASSURE_DEFAULT_CONFIGURATIONS_BUILT:
ifeq ($(shell $(StroikaRoot)ScriptsLib/GetConfigurations --quiet),)
	@$(MAKE) -silent default-configurations MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
endif


STROIKA_CONFIG_PARAMS_COMMON=

STROIKA_CONFIG_PARAMS_DEBUG=--apply-default-debug-flags
STROIKA_CONFIG_PARAMS_RELEASE=--apply-default-release-flags
ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
### address sanitizer on Windows produces lots of non-obvious errors (probably false positive).
### anyhow, disable for a little bit until we have time to look into it
### 	- Opened MSFT issue - https://developercommunity.visualstudio.com/t/https:developercommunityvisualstudio/1470855?entry=myfeedback
###		ASAN works, so you can re-enable it occasionally, but it really slows things down alot and pollutes the debug log
###		with TONS of spurrious nonsense, so best to leave disabled.
### 		-- LGP 2021-07-08
STROIKA_CONFIG_PARAMS_DEBUG += --sanitize none
endif



.PHONY: default-configurations
default-configurations:
	@if [ ! -d ConfigurationFiles ] ; then $(MAKE) --silent build-root; fi
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) Configuring...
ifeq ($(DETECTED_HOST_OS), Darwin)
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
else ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --arch x86_64 --build-by-default never --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --arch x86_64 --build-by-default never --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-Logging --arch x86_64 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86_64 --trace2file enable $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug-x86 --arch x86 --build-by-default never --config-tag Windows --config-tag x86 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug-x86_64 --arch x86_64 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-x86 --arch x86 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release-x86_64 --arch x86_64 --build-by-default $(DETECTED_HOST_OS) --config-tag Windows --config-tag x86_64 $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Profile --arch x86_64 --build-by-default never --config-tag Windows --config-tag x86_64 --append-extra-suffix-linker-args -PROFILE $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
else
	@#try g++, and then successively newerer versions; if both all take more recent by overwriting configuration;
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-12' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-12' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-13' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-13' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-14' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_DEBUG));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure Release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --only-if-has-compiler --compiler-driver 'g++-14' $(STROIKA_CONFIG_PARAMS_COMMON) $(STROIKA_CONFIG_PARAMS_RELEASE));
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --config-tag raspberrypi --apply-default-release-flags --only-if-has-compiler --compiler-driver 'arm-linux-gnueabihf-g++-12' --cross-compiling true);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --config-tag raspberrypi --apply-default-debug-flags --only-if-has-compiler --trace2file enable --compiler-driver 'arm-linux-gnueabihf-g++-12' --cross-compiling true --append-CXXFLAGS -Wno-psabi);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --config-tag raspberrypi --apply-default-release-flags --only-if-has-compiler --compiler-driver 'arm-linux-gnueabihf-g++-13' --cross-compiling true);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --config-tag raspberrypi --apply-default-debug-flags --only-if-has-compiler --trace2file enable --compiler-driver 'arm-linux-gnueabihf-g++-13' --cross-compiling true --append-CXXFLAGS -Wno-psabi);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-release --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --config-tag raspberrypi --apply-default-release-flags --only-if-has-compiler --compiler-driver 'arm-linux-gnueabihf-g++-14' --cross-compiling true);
	@(export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./configure raspberrypi-debug --build-by-default $(DETECTED_HOST_OS) --config-tag Unix --config-tag raspberrypi --apply-default-debug-flags --only-if-has-compiler --trace2file enable --compiler-driver 'arm-linux-gnueabihf-g++-14' --cross-compiling true --append-CXXFLAGS -Wno-psabi);
endif
ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
	@$(MAKE) --silent Builds/__AUTOMATIC_MAKE_PROJECT_FILES__
endif
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Applying configuration(s) to vscode:"
	@for i in `$(StroikaRoot)ScriptsLib/GetConfigurations --all` ; do\
		$(StroikaRoot)ScriptsLib/ApplyConfiguration --only-vscode $$i;\
	done

build-root:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) Making BuildRoot...
	@export MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) && cd $(StroikaRoot) && ./ScriptsLib/MakeBuildRoot ../../../

apply-configurations-to-vscode:
	@$(StroikaRoot)ScriptsLib/PrintLevelLeader $(MAKE_INDENT_LEVEL) && $(ECHO) "Applying configuration(s) to vscode:"
	@for i in $(APPLY_CONFIGS) ; do\
		MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $(StroikaRoot)ScriptsLib/ApplyConfiguration --only-vscode $$i;\
	done

list-configurations list-configuration-tags apply-configurations apply-configuration apply-configurations-if-needed reconfigure:
	@$(MAKE) --directory $(StroikaRoot) --silent CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)" $@

project-files:
	@$(MAKE) --directory $(StroikaRoot) --silent CONFIGURATION_TAGS="$(CONFIGURATION_TAGS)" $@
	@#Workaround https://stroika.atlassian.net/browse/STK-943
	@cd Workspaces/VisualStudio.Net; rm -f Microsoft.Cpp.stroika.ConfigurationBased.props; $(StroikaRoot)/ScriptsLib/MakeSymbolicLink ../../ThirdPartyComponents/Stroika/StroikaRoot/Workspaces/VisualStudio.Net/Microsoft.Cpp.stroika.ConfigurationBased.props
	@cd Workspaces/VisualStudio.Net; rm -f Microsoft.Cpp.stroika.user.props; $(StroikaRoot)/ScriptsLib/MakeSymbolicLink ../../ThirdPartyComponents/Stroika/StroikaRoot/Workspaces/VisualStudio.Net/Microsoft.Cpp.stroika.user.props

distclean:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "WhyTheFuckIsMyNetworkSoSlow $(call FUNCTION_CAPITALIZE_WORD,$@):"
ifneq ($(CONFIGURATION),)
	$(error "make distclean applies to all configurations - and deletes all configurations")
endif
	@rm -rf Builds ConfigurationFiles IntermediateFiles
	@$(MAKE) --no-print-directory clobber MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
	-@$(MAKE) --no-print-directory --directory $(StroikaRoot) --silent $@ MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))

clean clobber:
ifeq ($(CONFIGURATION),)
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "WhyTheFuckIsMyNetworkSoSlow $(call FUNCTION_CAPITALIZE_WORD,$@):"
ifeq ($(CONFIGURATION_TAGS),)
	@if [ "$@"=="clobber" ] ; then \
		rm -rf IntermediateFiles/* Builds/*;\
	fi
	@if [ "$@" = "clean" ] ; then \
		rm -rf IntermediateFiles/*/WhyTheFuckIsMyNetworkSoSlow*;\
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

ifneq ($(findstring $(DETECTED_HOST_OS),MSYS-Cygwin),)
# SEE https://stroika.atlassian.net/browse/STK-940
Builds/__AUTOMATIC_MAKE_PROJECT_FILES__:
	@$(MAKE) project-files
	@touch Builds/__AUTOMATIC_MAKE_PROJECT_FILES__
endif

.PHONY: installers installer-deb installer-rpm installer-wix
installers installer-deb installer-rpm installer-wix:   $(TARGETEXE)
	@$(StroikaRoot)/ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "Building Installers:"
	@$(MAKE) --no-print-directory --directory Installers MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1)) $@


update-submodules:
	git submodule update --init --recursive


STROIKA_COMMIT?=v3-Release
latest-submodules:
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $(MAKE_INDENT_LEVEL) "WTF $(call FUNCTION_CAPITALIZE_WORD,$@):"
	@$(StroikaRoot)ScriptsLib/PrintProgressLine $$(($(MAKE_INDENT_LEVEL)+1)) "Checkout Latest Stroika (STROIKA_COMMIT=${STROIKA_COMMIT})"
	@(cd $(StroikaRoot) && git checkout $(STROIKA_COMMIT) --quiet && git pull --quiet)


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
