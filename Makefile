all:
	@$(MAKE) --directory=html --no-print-directory all

update-submodules:
	git submodule update --init --recursive
