# WhyTheFuckIsMyNetworkSoSlow Release Notes

## About

High level summary of changes in WhyTheFuckIsMyNetworkSoSlow.

## History


### 1.0d14x {2021-12-26x}

#### TLDR


#### Change-Details

- Updated Webserver implementation to reflect Stroika changes/features, like Cache-Control, etag support

- revised WSAPI for About - returning list of components - About API and current process and currrent machine info, combined IO rate data etc

- IntegratedModel::Mgr  () model now written to DB, and loaded on each startup, so we have history - for devices seen on a network (not working perfectly, but somewhat) - writing networks/devices to DB (works using ORM)

- lastSeenAt support for Network objects in Merged datamodel, and in UI

- lose support for CircleCI (since too painful to debug problems due to limitations on number of builds per week)

- HTML
  - better network name display support
- Stroika 2.1b15
  - [Release Notes](https://github.com/SophistSolutions/Stroika/blob/v2.1b15/Release-Notes.md)


#if 0
    



    IntegratedModel::Mgr::GetNetworks - more progress saving lastSeenTime for networks and returning the list merged

commit a1ddb8dafdabf4347c17ac8e71ce16af4940bb95
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Jul 12 14:42:39 2021 +0100

    latest stroika; and INtegrationModelMgr: fixed so (too aggressively) updated DBAccess caches with stuff written (so when network goes offline we get right data shown)

commit 65b4264d2f97a9d59c0f458514671a3b210f2264
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 13 17:44:04 2021 +0100

    todo notes; and fixed test for 'active network in html to take diff in seconds, not matching string

commit d1b65719a2a2f80a2111d7fd071969cd659a4e45
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Jul 14 02:15:48 2021 +0100

    Small cleanups to HTML GUI for devices and netowrks (dont include id in name line) and todo docs

commit 26fab6e3ab7e55c3e8f2ab64a1107424b7afa0da
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Jul 16 16:38:54 2021 +0100

    latest stroika and react to configuration name changes for windows (to match project files for vs2k19)

commit aa757ac0b54e97b3ad1f43031b7d92110aa0fa37
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jul 18 22:21:29 2021 +0100

    added control flag kSupportPersistedNetworkInterfaces_- since we dont persist this info - dont persist REFERENCES to the IDS (fix when wehve done versioned/merged data in other structures like networks/devices);

commit 40a12ca0a2c639553db75038b7ef9278e9ee1734
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jul 18 22:23:45 2021 +0100

    On NetworkDetails component - added extra field for networkInterface objects and displayed here (flickery but OK for now); redid Vuex code to not use mutations.mts file (simpler without and I dont see teh value of teh indirection)

commit ec201fb4ae80e9173d3a7d9f1855f70920128f06
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Aug 10 02:35:38 2021 +0100

    latest stroika, using updated :ProvisionForVersion, and updated Mgr database logic, and lastSeenAt optional for devices

commit 6c7a63fabb5c75fecc266da154e5fcdbd8055b40
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Aug 25 01:26:52 2021 +0100

    fixed storage of networks/devices cache to be more efficent (would be best to rewrite with KeyedCollection)

commit 67a63fd1d541fd6b693dd019ab15e7dcbf3c7dff
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Aug 25 23:12:12 2021 +0100

    small cleanups to IngratedModel::Mgr - performance, docs and correctness

commit ee15a692e0accbffefcb5479f23c8c870431e6d6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Aug 26 15:28:20 2021 +0100

    started adding aggregates to device and network model

commit f2f5eb28a0c113d78f731fa8fc201b5b89f2e8d9
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Aug 27 13:49:46 2021 +0100

    operator==!=<> for GEOLocationInformation

commit 698300b480de1d3eaa202130a1266fca0af58f19
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Aug 27 13:51:25 2021 +0100

    On Model code - added Device::GetHardwareAddresses, and lose assert about guids equal on merge

commit 44e875344030643b8ae3d9fccb3626fadbac6f08
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Aug 27 17:49:50 2021 +0100

    first draft (dynamic) rollup logic for devices and networks: WSAPI for getall just returns rollups, but GetByID returns rollups or individuals and each rollup has a list of 'aggregates' (not yet shown in UI) whcih compprise that rolled up object

commit ef2c58158ae34d0bd2a533a4ff12a797ca7f2eaf
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Aug 27 18:19:13 2021 +0100

    display lists of aggregates (primitive)

commit d8b7a3b17c6cf2babba3cfc71bcdcabbf24f68d5
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Aug 27 18:19:59 2021 +0100

    fied 'last few minutes' filter in html ui

commit e4521b232c316f68e84bc732a70ad0c05e95dec8
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Aug 28 13:36:11 2021 -0400

    DeviceDetaisl compoent no longer takes networks as arguments (autocomputes but not yet correctly); vuex store fixed to handleupdate of device or devices list and network or networks list, with separate fetches etc, and  fixed so does reactivity properly with Vue.Set() - needed for vue2

commit 969468f2b7fb1a666ceb5bcddb6fc2ba1636ffd3
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Aug 28 13:54:25 2021 -0400

    cosmetic, and debug info on discovery code

commit f9a8796e27f1d14ada44efd6f12804bd085b5a16
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Aug 28 13:54:46 2021 -0400

    tweaked integration model rollup code but still needs work

commit 380df7e4c622c253ee10d02575daef78b525c794
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Aug 28 16:32:18 2021 -0400

    cleanup DEviceDetails component - property argument is now deviceID not device, and all the data computed inside, with dyncamic fetche sand fetches right networks used, not all curreent networks

commit 23c7fa9ade1c107423dbeea9b76b21e30df414b5
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Aug 28 19:35:22 2021 -0400

    fixed network/networkdetails html code like I just did for devices - so polling etc done in subcompoentn and all we pass in is id argument; and at same time supported getting non-rollup networks displayed

commit 3e65168b6f793c2ec29c2cc04fc76f1c7543d8a9
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Aug 28 20:47:47 2021 -0400

    renamed aggregates to aggregeatesReversibly in wsmodel for network and devices

commit cf23b9512d625978c45c0de13afd88d91e0ba720
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Aug 29 21:39:23 2021 +0100

    Support new WSModel aggregatesIrreversibly idIsPersistent  and historicalSnapshot fields for network, and device objects

commit ac03f7aedfb69de4b1b09c2ecc60efab975617c8
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Aug 30 03:44:53 2021 +0100

    latest stroika; and ShouldRollup_ (const Network& n1, const Network& n2)
    fixes for smalter matching (and specail case of hughsnet); gen random IDs for discovered networks

commit ea446e6d9bfbe83a62a40ad4690ab6a97767fbd1
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Aug 30 08:53:28 2021 -0400

    fix html ui naming of networks

commit a9da47afb26c13156b5be368b0a810e74b0fda7a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Aug 30 13:58:05 2021 -0400

    latest stroika and use CallerStalenessCache <void,RollupData> to lookup rollups

commit 6c2c5ac7a1412a3f0c0e64e8883de875a807325f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Aug 30 14:01:01 2021 -0400

    vscode checkin file

commit e952f996aa232570b767dbd5509e4d7f60586422
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Aug 31 05:09:59 2021 +0100

    todo notes; backend performance/rollup imrpvoements (but still needs alot of work)

commit f1325ec5be954d60927892cbe652ad7228eb224d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Aug 31 00:48:17 2021 -0400

    cleanup generation of uniuqie IDs for networks, which fixes issue where we were getting too many networks generated

commit 80540b24f2ba18cc6657156384179cf36621ad13
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Aug 31 17:04:45 2021 +0100

    todo docs and cleanups to allowStale flags in rollup code

commit 0a1ebea9ebb72e54ad4b7ab7054e7f05478cd9c6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Aug 31 12:14:39 2021 -0400

    integrationmgr clenaups

commit 6deecbd8382c4d4152a6b3a6543bd9cb23e4bf87
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Sep 3 22:49:51 2021 +0100

    docs on docker file runs for wtf

commit 2d47f18ebe3fac04e1b55c8a881eb5cde401580c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 22 02:29:32 2021 +0100

    latest stroika - and use new KeyedCollection<> instead of Mapping<> in a few places

commit e6b9b1cd21dce8c8520656b544595671a02fcc78
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Sep 28 02:08:06 2021 +0100

    latest stroika, minor cleanups, and docs of new bug to track down in stroika (lowpri cuz rare)

commit a8dcd9eff26c280d2570f7c90175bc5aaec3e7ef
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 29 02:11:18 2021 +0100

    cosmetic, plus latest stroika

commit 099c2f6bb3a4907086f42594a29a8b3417ca8b7f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Oct 1 02:33:06 2021 +0100

    very early rough draft of OpenAPI spec for API

commit 7f1db7c555e820c4e165398c71095a60f1d7bc71
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 4 04:31:15 2021 +0100

    latest stroika, plus minor/cosmetic

commit 955b0c756f6a93185c2e4f1de1bb2b7ae1bc79ad
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Nov 1 15:07:47 2021 +0100

    built using latest Stroika dev build

commit 84f109a62b84cbae17a7a003a5da00aef7325eac
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Nov 16 13:39:46 2021 -0500

    latest stroika (and react to one bug due to change in sematncis of safe iterators and better checking)

commit b56dbf0c072815de89c8b4b4b7eb995e05e3c104
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 25 21:15:20 2021 -0500

    latest stroika; define explicitly default/move CTORS for Device; and react to a few more changes in latest Stroika (deprecations)

commit 62292f65736349e082e8a70ad3e8c7837db3e208
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 23 14:54:39 2021 -0500

    tweka UpgradeLockNonAtomicallyQuietly sleeps before retry

commit 41461d54400df651a0b4d5c549f81b2a56104abd
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 23 16:02:10 2021 -0500

    Backend/Sources/IntegratedModel/Mgr. rollup code now rolls up if EITHER device has emply hardware address list but overlapping IP address list

commit 4fb63ada47ad5fa7999300591bde1704b62447ee
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 23 16:18:27 2021 -0500

    use KeyedCollection in a couple more places in IntegratedModel/Mgr

commit ae3b6527eda631bf895455ee0b56b429388b2d79
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 23 20:00:50 2021 -0500

    Fixed SSDP discovery code to not discover devices with no network (due to kIncludeLinkLocalAddressesInDiscovery suppression)

commit 6292338e26814daae143f5d22af17bd706f69173
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 23 21:14:37 2021 -0500

    up database version# cuz we were writing bad stuff in DB, and now that fixed, easiest way to throw away bad data

commit 480a7fb7a4a983434b476680c3e928899bb52863
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 24 09:29:04 2021 -0500

    nom audit fix

commit 20cfcb1b942b59545ab33a74e6712563fb06f41e
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 24 10:13:29 2021 -0500

    todo notes and latest stk

commit 1c9ceca0927e7f84ea58b8612c45d4c06de75682
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 24 16:28:46 2021 +0000

    try sass instaed of node-sass

commit 1e78b1407c352d3e39d31d6dd5df958cab473737
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 24 16:31:33 2021 +0000

    latest stk

commit 25a31f7fac6f621b5387ccc2bcab08c94512e15d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 24 17:35:49 2021 +0000

    try checking in package-lock.json for html code

commit 919c5518bb30b22fbb1ca862eaa2cac180e381cf
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 25 03:00:38 2021 +0000

    improved support for running docker containers (esp for local sophists dev - dev-containers

commit 6dd9d6ef75e6df566be37299fc0674eeef8b4f13
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 25 03:03:16 2021 +0000

    fix last makefile checkin

commit 71ba2de998cbeea7ff085b477d24f82313c29927
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Dec 26 01:50:40 2021 +0000

    

commit 4c5445ae088523a6311740a979f3bcc575ddefbf
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Dec 26 02:35:02 2021 +0000

    mostly cosmetic cleanup

#endif

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
