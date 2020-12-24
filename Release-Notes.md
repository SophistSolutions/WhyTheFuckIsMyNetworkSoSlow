# WhyTheFuckIsMyNetworkSoSlow Release Notes

## About

High level summary of changes in WhyTheFuckIsMyNetworkSoSlow.

## History

---

### 1.0d9, 1.0d10 {2020-12-24}

- Fixed small issue building raspberry pi image

---

### 1.0d8 {2020-12-24}

#### TLDR

- Update to Stroika 2.1b8
- Start rewrite of UI for list of devices/networks pages
- Build support using docker, and build using github actions
- Backend discovery improvments, including port scan, ping etc

#### Details

- Build System
  - Docker
    - Created docker configuraitons to facilitate simple builds with docker windows, and unix
  - \<Import Project=Microsoft.Cpp.stroika.AllConfigs.props to fix intellisense
  - CI
    - Support build on circleci
    - support build with [Github Actions](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/actions)
    - travisci: added support for this but then deprecated/removed since TravisCI gave up support for opensource
  - new ReleaseScript.md showing how to build images for release using docker and copy back to current directory
- Update to Stroika 2.1b8 and various updates reacting to new features and APIS
- Backend code
  - fixed performance issues with discovery and webservices
  - use Socket::Connect(...timout) for discovery, and scan tests
  - Use Bloom Filter to track discovery (for large) networks
  - BLOBs Mgr: has Activator class (CTOR/DTOR) for static module init; moved fields into instance (from static); added AsyncAddBLOBFromURL/Lookup mechanism / data to track mapping of URL to GUID for BLOB
  - optional ScanParameters to ScanPorts() so can scan more quickly
  - lots of cleanups to Device code - adding fKnownOpenPorts prelim variable - and storing in debug props for debugging - shows found scanned port #s, and reorganized scanning code so does a better job with scan checks (still alot to improve here)
  - lastSeenAt support in web service API
  - KnownDevicePortScanner implementation
  - run ping check on background existing devices rnadomly trying each port
  - did draft of Device::GetPreferredDisplayInternetAddresses () and used to replace Unknown name with better default name (ip addr) - using new Stroika Iterbale::Join
  - map IPP/LPD services (with manufacturer has hewlet packard, epson etc) - as type Printer
  - replace use of Set with BloomFilter for randomly exploring network in RandomWalkThroughSubnetDiscoverer*::Checker*
  - treat ssdp device type urn:dial-multiscreen-org:device:dial:1 as media player
  - added debugProps for a few more things (name of device related) so can debug easier from web gui
  - Added ICMP ping check to list of open ports checked for known devies, and also updates last-seen-at field
- HTML Web GUI
  - added .prettierrc
  - new logger module mostly to address console warning
  - upgraded version of typesript and vue-router (npm upgrade)
  - Deprecated old Devices/Networks GUI, and added new experiemntal listbox based one
  - added breadcrumbs UI
  - refactored devices code in new device ui so instead of using slots, we prodcue a nrew devicesAsDisplayed object so it works with search. Then redid the serach code to use a global vueex searchString and set that from toolbar search
  - lastSeenAt and use vue.moment to display
  - show open ports in device details page
  - switched devices pages (html) to using embedded expando content instead of showing selected items in area below list
  - include operating system icons, Printer.ico, etc...
  - minor tweak to devices page, and on networks page, fixed compute of number of devices on that network

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
    - GetSystemConfiguration\* changes
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
  - new kInclude*SSDP_Discoverer*, kInclude*MyDevice_Discoverer*, kInclude*Neighbor_Discoverer* flags to help testing

- WSAPI

  - Support sorting (and speced out but not implemented) filtering of device lists (by Type, Priority, Address, Name)

    ```bash
    curl 'http://localhost:8080/devices?recurse=true&sort={"searchTerms":[{"by":"Address"},{"by":"Priority"}],"compareNetwork":"192.168.244.0/24"}'
    ```

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
