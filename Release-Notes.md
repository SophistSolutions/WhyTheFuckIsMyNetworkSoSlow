# WhyTheFuckIsMyNetworkSoSlow Release Notes

## About

High level summary of changes in WhyTheFuckIsMyNetworkSoSlow.

## History

---

### 1.0d7 {2020-01-11}

- https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/compare/1.0d6...1.0d7
- Use Stroika [v2.1a4](https://github.com/SophistSolutions/Stroika/tree/v2.1a4) ([Stroika Release Notes](https://github.com/SophistSolutions/Stroika/blob/v2.1a4/Release-Notes.md))
  - many changes (e.g. improved CIDR, SSDP, locking, new SystemFirewall setup)

- CI systems
  - CircleCI - builds there automatically (Linux only so far), and produces installers which can be distributed (x64, and raspberrypi .deb files)
  - TravisCI

- Device discovery
  - experiment with different locking strategies (I think much improved)
  - improved ip address filtering
  - CIDR fixes
  - SSDP improvements (much from stroika)

- Network discovery
  - genCIDRsFromBindings() now supports removing subsumed networks (cuz I saw that in Barharbor hotel)

- Device Introspection
  - first rough draft of port scanner

- UI
  - basic sorting support
  - visual transitions as lists change

- Build System
  - For windows, react to Stroika configuration changes, and use makefile based projects
  - cleaned up makefile for html - so builds out of IntermediateFiles (so can build multiple configs without interference)
  - added windows configuration Release-Logging-U-64

- new BLOBMgr - and WSAPI for getting BLOBs, and storing them (for icons)

---

### 1.0d6 {2019-06-19}

- https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/compare/1.0d5...1.0d6
- Use Stroika v2.1d26 [(Stroika Release Notes)](https://github.com/SophistSolutions/Stroika/blob/v2.1d26/Release-Notes.md)
  - many adaptations for this, including
    - use new URI class instead of URL
    - new WebServer Router support for regex matching
    - Switch to using IO::Network::Neighbors to fetch neighbor info
    - GetSystemConfiguration* changes
    - cleanups to constexpr DefaultNames usage
  - Contains critical fix to CIDR parsing that was causing bad internet address matching

- Build System
  - added make release-directory target
  - Switch to VisualStudio.Net-2019

- Discovery
  - Discovery generally works much better (gathering more stats and small fixes to accuracy)
  - Discory::NetworksMgr now handles IPv6 addresses too, except it intentioanlly omits link-local addresses from the 'networks'
  - new kIncludeMulticastAddressesInDiscovery{false},kIncludeLinkLocalAddressesInDiscovery{false};
    flags to help testing (we want off I think but sometimes helpful to turn on to see extra data)
  - SSDP discover Icon and store in model/WSAPI
  - support new EthernetMACAddressOUIPrefixes module - include this into manufacturing info (if not gotten from SSDP)
  - use Caching layer for DNS lookups (besides DNS cache)
  - SSDP Client searcher now re-searches periodically
  - new kInclude_SSDP_Discoverer_, kInclude_MyDevice_Discoverer_, kInclude_Neighbor_Discoverer_ flags to help testing

- WSAPI
  - Support sorting (and speced out but not implemented) filtering of device lists (by Type, Priority, Address, Name)

    ~~~bash
    curl 'http://localhost:8080/devices?recurse=true&sort={"searchTerms":[{"by":"Address"},{"by":"Priority"}],"compareNetwork":"192.168.244.0/24"}'
    ~~~

  - Large refactoring:
    - new NetworkAttachmentInfo subobject
        containing list of ipaddresses and hardware addresses
        grouped by network (not network interface - was by device)
    - this replaces ipaddresses list in device
    - fixed logic for adding router type to check matching addr between gateways and given device
  - debugProps fields in WSAPI

- HTML GUI
  - Fixed link for open external device (presentation url)
  - tweaked tslint.json and ran npm run fix to apply them
  - Fixes navbar so it is fixed to top when scrolling
  - Fixes about page rendering error
  - Adds basic support for polling networks and devices for changes every 10 sec
  - use new sorting code to provide a better default sort of devices
  - dont set details of URL path in env file, but compute it from other variables (soon in the store).
  - updated GUI for new WSAPI structure (list of ip addresses) and also show hardware address
    and a few other things that were lists, include all of them.
  - display new WSModel structures for device, such as Manufacturer, Icon

---

### 1.0d5 {2019-03-08}

- https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/compare/1.0d4...1.0d5
- Use Stroika v2.1d20 [(Stroika Release Notes)](https://github.com/SophistSolutions/Stroika/blob/v2.1d20/Release-Notes.md)
- Improved error handling/reporting
- refactor network and device discovery (big change - incomplete)
  - restructured so more modular: each module pursues discovery in dependently and accumulates in common store
  - internal caching in discovery and lookup code
  - improved naming/detection of network and device information/details (names, types, etc); more SSDP info displayed
  - fixed bug where duplicate devices appeared
  - did very rough draft implementation of arp-based device discovery
  - rough draft reverse DNS device name lookup
- lose bogus home page, and improved about page, and presentation url displayed for devices

---
