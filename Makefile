all:
	$(MAKE) --directory=html --no-print-directory all
	$(MAKE) --directory=ThirdPartyComponents --no-print-directory all

clean:
	@$(MAKE) --directory=html --no-print-directory clean
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clean

clobber:
	@$(MAKE) --directory=html --no-print-directory clobber
	@$(MAKE) --directory=ThirdPartyComponents --no-print-directory clobber

update-submodules:
	git submodule update --init --recursive
