all:
	@$(MAKE) --directory=html --no-print-directory all
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory all CONFIGURATION=$(CONFIGURATION)
	@$(MAKE) --directory=BackendApp --no-print-directory all CONFIGURATION=$(CONFIGURATION)

clean:
	@$(MAKE) --directory=html --no-print-directory clean
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clean

clobber:
	@$(MAKE) --directory=html --no-print-directory clobber
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clobber
	@rm -rf Builds IntermediateFiles ConfigurationFiles

update-submodules:
	git submodule update --init --recursive

format-code:
	$(MAKE) --directory=BackendApp --no-print-directory format-code
