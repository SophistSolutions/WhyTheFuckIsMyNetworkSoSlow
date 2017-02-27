SHELL=/bin/bash

all:
ifeq ($(CONFIGURATION),)
	@for i in `ThirdPartyComponents/Stroika/Stroika/ScriptsLib/GetConfigurations.sh` ; do\
		$(MAKE) --no-print-directory --silent all CONFIGURATION=$$i;\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all CONFIGURATION=$(CONFIGURATION)
	@ThirdPartyComponents/Stroika/Stroika/ScriptsLib/CheckValidConfiguration.sh $(CONFIGURATION)
	@$(MAKE) --directory=html --no-print-directory all
ifeq (,$(findstring CYGWIN,$(shell uname)))
	@$(MAKE) --directory=BackendApp --no-print-directory all CONFIGURATION=$(CONFIGURATION)
endif
endif

clean:
	@$(MAKE) --directory=html --no-print-directory clean
ifeq ($(CONFIGURATION),)
	@for i in `ThirdPartyComponents/Stroika/Stroika/ScriptsLib/GetConfigurations.sh` ; do\
		$(MAKE) --no-print-directory --silent clean CONFIGURATION=$$i;\
	done
else
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clean
endif

clobber:
ifeq ($(CONFIGURATION),)
	@rm -rf Builds IntermediateFiles ConfigurationFiles ThirdPartyComponents/Stroika/Stroika/{Builds,IntermediateFiles,ConfigurationFiles}
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
	(cd ThirdPartyComponents/Stroika/Stroika && git checkout V2-Release && git pull)
else
	(cd ThirdPartyComponents/Stroika/Stroika && git checkout $(BRANCH) && git pull)
endif

format-code:
	$(MAKE) --directory=BackendApp --no-print-directory format-code
