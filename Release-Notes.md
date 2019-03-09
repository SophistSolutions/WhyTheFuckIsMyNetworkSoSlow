About
=====

High level summary of changes in WhyTheFuckIsMyNetworkSoSlow.

History
=======

### 1.0d5x {2019-03-08}

- https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/compare/1.0d4...1.0d5
- Use Stroika v2.1d20 [(Stroika Release Notes)](https://github.com/SophistSolutions/Stroika/blob/v2.1d20/Release-Notes.md)
- Improved error handling/reporting
- refactor network and device discovery (big change - incomplete)
  - restructured so more modular: each module pursues discovery in dependently and accumulates in common store
  - internal caching in discovery and lookup code
  - improved naming/detection of network and device information/details (names, types, etc); more SSDP info displayed
  - fixed bug where duplicate devices appeared
  - did very rough draft implementation of arp-based device discovery
- lose bogus home page, and improved about page, and presentation url displayed for devices

----
