SHELL=/bin/bash

all:
	@$(MAKE) --directory=ThirdPartyComponents/Stroika --no-print-directory configurations
ifeq ($(CONFIGURATION),)
	@for i in `ThirdPartyComponents/Stroika/StroikaRoot/ScriptsLib/GetConfigurations.sh` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i;\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all CONFIGURATION=$(CONFIGURATION)
	@ThirdPartyComponents/Stroika/StroikaRoot/ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=html --no-print-directory CONFIGURATION=$(CONFIGURATION) all
ifeq (,$(findstring CYGWIN,$(shell uname)))
	@$(MAKE) --directory=BackendApp --no-print-directory all CONFIGURATION=$(CONFIGURATION)
endif
endif

clean:
	@$(MAKE) --directory=html --no-print-directory clean
ifeq ($(CONFIGURATION),)
	@for i in `ThirdPartyComponents/Stroika/StroikaRoot/ScriptsLib/GetConfigurations.sh` ; do\
		$(MAKE) --no-print-directory --silent clean CONFIGURATION=$$i;\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clean
endif

clobber:
ifeq ($(CONFIGURATION),)
	@rm -rf Builds IntermediateFiles ConfigurationFiles ThirdPartyComponents/Stroika/StroikaRoot/{Builds,IntermediateFiles,ConfigurationFiles}
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clobber
else
	@$(MAKE) --directory=html --no-print-directory clobber
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clobber
	@$(MAKE) --directory=BackendApp --no-print-directory clobber
endif

update-submodules:
	git submodule update --init --recursive

latest-submodules:
ifeq ($(BRANCH),)
	(cd ThirdPartyComponents/Stroika/StroikaRoot && git checkout V2-Release && git pull)
else
	(cd ThirdPartyComponents/Stroika/StroikaRoot && git checkout $(BRANCH) && git pull)
endif

format-code:
	$(MAKE) --directory=BackendApp --no-print-directory format-code
