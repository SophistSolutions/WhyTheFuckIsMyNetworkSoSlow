# WhyTheFuckIsMyNetworkSoSlow Release Notes

## About

High level summary of changes in WhyTheFuckIsMyNetworkSoSlow.

## History

### START 1.0d18x DEV

---

### 1.0d17 {2022-06-30}

#### TLDR
- Minor backend fixes
- Nearly full rewrite of UI, based on Vue3/Quasar instead of Vue2/vuetify; UI slightly changed, but mostly the same look and feel

#### Change-Details
- Backend
  - Stroika 2.1.1x (almost point release)
    - fix windows hardware addresses to use : instead of -
    - workaround issue with Logger::Shutdown () - taking a long time
    - fix for serializing / deserializing CIDRs; 
    - latest stroika (maybe fix shutdown)
  - new IntegrationMgr function GetCorrespondingDynamicDeviceID (), and used that in the RefreshDevice WSAPI to allow rescan of aggregated devices; and cosmetic cleanups
  - uppded DB filename version to db8 cuz of incompatible change in database format (still no upgrade logic in place)
  - renamed DevicesMgr::sThe.InitiateReScan -> DevicesMgr::sThe.ReScan (); fixed  bug in GetCorrespondingDynamicDeviceID () - was looking at wrong list for dynamic device ids;
- html / UI
  - Experimented upgrading to vuetify3, but its not close to ready
  - Experimented with using react instead of vue
  - Switched to using Quasar, and vu3 3
    - this was a huge change - almost a total rewrite of html
    - major cleanup of internals of typescript and html code
  - html makefile fixes - copy html file and use cp not ln -s

---

### 1.0d16 {2022-05-22}

#### TLDR

- Backend now built using [Stroika 2.1](https://github.com/SophistSolutions/Stroika/v2.1) released version
- Fixed important Rollup (aggregate devices/networks) bugs
- fixes to rollup code and (super rare) deadlock issue
- some improved debug logging for other minor discovery issues
- build with github actions fixed

#### Change-Details

- same as 1.0d15 (typo)

- Backend
  - Use Stroika 2.1 (final released version)
    - [Release Notes](https://github.com/SophistSolutions/Stroika/blob/v2.1/Release-Notes.md)
    - react to stroika update(s): no more run2idle server stuff - just arg to run-directly
  - added misisng SuppressInterruptionInContext in Activator DTOR
  - Rollup (aggregation) of devices fixes:
    - various cleanups to Rollup code: mostly fixed issue with fLastSeenAt not rolled up correcly (randomly showed old date). Still could use work, but better
    - fixed significant bug with rollup - networks and devices - and perhaps tweaked perforamnce. Sometimes on raspberrypi I was seeing the self-discvered self-device was repeasted thousands of times. Cuz had no network attached (separate bug looking into)  : but rollup in that case didn't roll it up so it got repeated. If two raw devices have same ID, then consider them rolled up, and not a new rolled up device
  - extra debug logging/notes in json output
    - added debugprops for MyDeviceDiscoverer_-At, and further updates to device fDebugProps to make easier debugging of where data comes from
    - unclear why sometimes rasperrypi device discovered with Created-By-MyDeviceDiscoverer but no networks/interfaces' So add extra fDebugProps debug loggint to see more
    - added debug prop to debug why smoemtiems I find devices on linux with no attached networks
    - Added debugging stuff ot see why we get 'Unkown' device added on linux/hercules
  -  Hopefully temporary workaround for https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23 - deadlock bug
  - edit .service file so on failure, it auto-restarts (crashed on raspberrypi)
  - negative caching of reverse DNS lookup failure

- Build System
  - use DETECTED_HOST_OS from latest Stroika in makefiles'
  - update Microsoft.Cpp.stroika.AllConfigs.props and ExecutablePath for latest stroika (so should work with MSYS)
  - vs2k22 project and sln files
  - added symbolic links to Workspaces/VisualStudio.net/Microsoft.Cpp.stroika.ConfigurationBased.props etc and props loads in vsproj file so can build properly from visual studio
  - update .vscode files from recent Stroika Skel work
  - ubuntu 22.04 support
    - docker containers
    - github actions
  - github actions:
    - was broken, now fixed
    - fix download directory for uploading artifacts
    - check IncludeDebugSymbolsInExecutables before stripping for build of installers
    - workaround https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/22 by disabling asan on raspberrypi builds
    - includeInDevBranchBuilds true for ubuntu-20.04-X2-raspberrypi

- html
  - update dependent components
    - upgrade core-js
    - latest typescript
    - npm update/audit fix
  - tweaked html for about page

---

### 1.0d14 {2021-12-26}

#### TLDR

- Improved Stroika/Internals: performance and webserver especially
- Peristence of measurements
- IntegratedModel layer that rolls up different snapshot/historical devices into integrated current model; can view aggregaged instances in UI, or rollup
- Better about/stats page, showing memory and CPU etc usage

#### Change-Details


- revised WSAPI for About - returning list of components - About API and current process and currrent machine info, combined IO rate data etc

- IntegratedModel
  - IntegratedModel::Mgr  () model now written to DB, and loaded on each startup, so we have history - for devices seen on a network (not working perfectly, but somewhat) - writing networks/devices to DB (works using ORM)
  - Added aggregates to device and network model: new WSModel aggregatesIrreversibly idIsPersistent  and historicalSnapshot fields for network, and device objects
  - draft (dynamic) rollup logic for devices and networks: WSAPI for getall just returns rollups, but GetByID returns rollups or individuals and each rollup has a list of 'aggregates' (not yet shown in UI) whcih compprise that rolled up object


- Stroika 2.1b15
  - [Release Notes](https://github.com/SophistSolutions/Stroika/blob/v2.1b15/Release-Notes.md)
  - react to changes, like configuration name changes for windows, KeyedCollection, CallerStalenessCache <void,RollupData> to lookup rollups, default/move CTORS, ...
  - Updated Webserver implementation to reflect Stroika changes/features, like Cache-Control, etag support

- Builds
  - lose support for CircleCI (since too painful to debug problems due to limitations on number of builds per week)
  - improved support for running docker containers (esp for local sophists dev - dev-containers

- Discovery
  - Fixed SSDP discovery code to not discover devices with no network (due to kIncludeLinkLocalAddressesInDiscovery suppression)
  - lastSeenAt support for Network objects in Merged datamodel, and in UI

- HTML
  - better network name display support
  - Small cleanups to HTML GUI for devices and netowrks (dont include id in name line) and todo docs
  - npm audit fixes, including sass instaed of node-sass (since later appears deprecated)
  - 'last few minutes' filter in html ui
  - try checking in package-lock.json for html code
  - display lists of aggregates (primitive)
  - On NetworkDetails component - added extra field for networkInterface objects and displayed here (flickery but OK for now); redid Vuex code to not use mutations.mts file (simpler without and I dont see teh value of teh indirection)
  - DeviceDetails compoent no longer takes networks as arguments (autocomputes but not yet correctly); vuex store fixed to handleupdate of device or devices list and network or networks list, with separate fetches etc, and  fixed so does reactivity properly with Vue.Set() - needed for vue2; property argument is now deviceID not device, and all the data computed inside, with dyncamic fetche sand fetches right networks used, not all curreent networks
  - fixed network/networkdetails html code like I just did for devices - so polling etc done in subcompoennt and all we pass in is id argument; and at same time supported getting non-rollup networks displayed

- OpenAPI
  - very early rough draft of OpenAPI spec for API

---

### 1.0d13 {2021-01-17}

#### TLDR

- Fixed bug causing scan to over-report devices, and added GUI/API to allow rescan of a device.
- Improved performance

#### Change-Details

- New IntegratedModel::Mgr class - placeholder for future expansion (persistence)
- Makefile 'clean' fix
- Rescan-Device WSAPI, and GUI
- Fine tuned WebServer configuration so runs more threads and better performance

- Stroika 2.1b9
  - [Release Notes](https://github.com/SophistSolutions/Stroika/blob/v2.1b9/Release-Notes.md)
  - Improves network device enumeration to report network addresses and ranges properly.

---

### 1.0d12 {2021-01-07}

#### TLDR

- Major GUI cleanups - so now browsing a network and set of devices works well.
  Still no performance monitoring in GUI, nor anything editable or persistent.

- Small backend and build system improvements

#### Details

- html/gui
  - Major cleanups
    - Devices page and Networks page listbox now work well, with both list views doing the same expando style view details
    - Use components much more, and put details view for devices and networks into a component
      - now devices and networks list pages have details that can be popped into separate monitoring window
    - Menus and breadcrumbs now work cleanly (and display properly)
    - Draft HOME page
    - Cleanup all the noisy warnings/issues with the vue code - both static es-lint warnings as well as dynamic warnings in the chrome debugger.
    - new Filterbar, defined separately in each page, and done decently for Devices and Networks pages (merged search there, but now also other filters); this required lots of work on the app-bar functionalty and moved it from App.vue to component used in each page view. It shows filter summary including number of matches.
  - Minor cleanups
    - html: lose a Watch() call that got warnings and didnt appear to be needed
    - cleaned up router usage
      - eg. support selectedNetwork query argument to devices page (see <https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/14>)
    - use vue-json-viewer to display debugging json code
    - cleaned up confusion between networks and attachedinterfaces to devices output: for now attachedinterfaces only MINIMALLY (for debugging) supported. Will want to support a bit more later.
    - Added services panel in devices pages
- Build System
  - Improvements to github action workflows
  - npm latest components (but cannot switch to vue 3 yet cuz no migration tool yet)
  - Cleaned up most build eslint warnings
  - configuring branch to build in github actions so you can force a rebuild using the release workflow using the DEV branch
  - vetur.config.js file so it finds the package.config file and editring in vscode works better
- API-Server
  - map OracleVM MACADDR to mean type is new Virtual-Machine; and html: support that Virtual-Machine type and other gui cleanups
  - Added a few extra ports to scan
  - Improvements to device scanning logic (fixed bug with 0 GUID); and improvements to guessing types (and new device/service types)
  - tweaked (json not string a few) fDebugProps (need stroika improvement to add the rest)
- Notes/Docs
  - DeviceDiscoveryHints.md file

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
