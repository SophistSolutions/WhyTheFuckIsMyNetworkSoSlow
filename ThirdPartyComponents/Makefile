export TOP_ROOT=$(shell realpath ../)/

all:
	@$(MAKE) --directory=Stroika --no-print-directory all CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))

clean:
	@$(MAKE) --directory=Stroika --no-print-directory clean  CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))

clobber:
	@$(MAKE) --directory=Stroika --no-print-directory clobber CONFIGURATION=$(CONFIGURATION) MAKE_INDENT_LEVEL=$$(($(MAKE_INDENT_LEVEL)+1))
