all:
	###quick hack impl - make links and then cd there and make
	mkdir -p ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow
	rm -f ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/Makefile
	ln -s `pwd`/Projects/Unix/Makefile ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow/Makefile
	$(MAKE) --directory ../IntermediateFiles/$(CONFIGURATION)/WhyTheFuckIsMyNetworkSoSlow all


format-code:
	@../ThirdPartyComponents/Stroika/Stroika/ScriptsLib/FormatCode.sh Sources *.h *.inl *.cpp
