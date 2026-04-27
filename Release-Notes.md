# WhyTheFuckIsMyNetworkSoSlow Release Notes

## About

High level summary of changes in WhyTheFuckIsMyNetworkSoSlow.

## History


- HTMLUI
  - Show new Model features (like network address, name)
  - todo notes 
  - cleanups 
  - NetworkInterface 
    - code showing Seen range and showing it in breadcrumbs
    - format baud rate

- Model
  - ToString() support throughout
  - gateways and dns fields of networks now sets;
  - and new fGatewayHardwareAddresses field of network
  - added fAggregatesFingerprints to network model object; 
  - Added 'priorited' names list to Network objects (as I had before to Devices)
  - devices user settings comparable
  - support for aggregation of devices/networks in usersettings
  - capture network seen time in discovery instead of saying 'now' at top level of IntegrationMgr
  - renamed Device/NetworkInterface/Network::fGUID to fID
  - Prioritized names support
    - Networks, Devices
  - UserSettings
    - Tags: https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/39
      - network / devices rollups cleanups so usersettings drive combining of networks and devices. 
    - For NetworkInterface/Network/Device
    - support PATCH for /userOverrides/aggregateHardwareAddresses on /networks API (setting, but not code to handle rollups yet)
    - Device draft support for Device::UserSettings::fAggregateDeviceHardwareAddresses
    - Comparable support
    - Devuce
      - Device::UserSettings::fAggregateDeviceHardwareAddresses


- Backend
  - Rollup Logic
    - **mostly new**
    - use new GenerateFingerprintFromProperties () for Networks rollup logic (kept much of old logic ifdefed for future refernece - may want to move soem of that over)
    - Big rewrite of Network rollup logic (now much more efficent and should proplerly handle re-rollups as needed if rules for rollups change (rule change NYI)); much cleaner RolledUpNetworks object
    - on change network usersettings, invalidate rollups for networks
    - Same refactor for RolledUpDevices and RolledUpNetworkInterfaces
    - Caching rollup
    - Backend/Sources/IntegratedModel/Mgr: lots of code cleanups/refactoring; sb nothing functional changed
  - https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/38 - Common::PrioritizedNames support - complete on WS API side (redundantly return old name til I fix html code)
  - Execution/Exceptions
    - more DeclareActivity cleanups so better logging messages
  - Stroika
    - Upgrade from Stroika v2.1 to v3.0d23
    - Tons of changes to accomodate/take advantage
      - update DbgTrace format code to new style {} instead of %
  - Misc Coding
-     capture this for c++20 language conformance

- https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/39 -  Device::UserOverrides support, including GET and PATCH

- also update returned rollup device names to include user settings name

- re-engineer date range mapper code for Seen - so uses kDateRangeMapper_ instead of global range mapper

#if 0

commit cdb8de96d5bd660383ada6c9139a83f98d104eac
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Sep 17 17:39:36 2022 -0400

    also update returned rollup device names to include user settings name

commit 1a991378732338213bd7c6de73cb6ab5e65281f3
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Sep 19 10:40:21 2022 -0400

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/39 - json patch api I think fully working for devices user settings

commit 779ed1d8647f7986a985fd444e2a3976a4830c35
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Sep 19 19:56:44 2022 -0400

    changed one Assert to WeakAssert () with comments about debugging this issue later

commit 4135b54f7205d23ad5b915809c95d048a44898b6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 21 11:01:16 2022 -0400

    re-engineer date range mapper code for Seen - so uses kDateRangeMapper_ instead of global range mapper (which causes problems as they are combined since only one of each type of mapper) and easily overwritten/bad design to leave global)

commit 9661db0da9086c55e66717b86ff94ed103eedf24
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 21 11:15:47 2022 -0400

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/39 - draft support for storing usersettings on networks as well - persist do DB and show up in webservices (but not merged into network name since need to do name/names change there as well)

commit 23bef7368b9f0edab7ec03872b89842a082ca881
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Sep 24 23:16:40 2022 -0400

    up db filename cuz not backward compat; gateways and dns fields of networks now sets; and new fGatewayHardwareAddresses field of network

commit ad16d6631f8702dfafb4bace01e370e0fbf5eac8
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Sep 25 16:32:59 2022 -0400

    use new ComputeProbablyUniqueIDForNetwork () for Networks rollup logic (kept much of old logic ifdefed for future refernece - may want to move soem of that over); fixed serious recent regression in Network::Rollup (); related to that rollup change now store new KEY in GenNewNetworkID () table

commit d7b8b0bd48e78d1c0a910650e9907f49ff110b6d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Sep 25 16:53:50 2022 -0400

    show gateway-hardware-address (newly added to date model) on network details page

commit d955ef6fdfdb93f33253e2820b038703b4fcfe17
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Sep 27 11:18:29 2022 -0400

    renamed network ComputeProbablyUniqueIDForNetwork -> GenerateFingerprintFromProperties (so calling it fingerprint and added fingerprint type); added fAggregatesFingerprints to network model object; Big rewrite of Network rollup logic (now much more efficent and should proplerly handle re-rollups as needed if rules for rollups change (rule change NYI); much cleaner RolledUpNetworks object

commit fd645ec611bb2580336ff2c09a845f3d52b3e2eb
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Sep 27 13:34:35 2022 -0400

    update html to show new networks 'names' instead of friendly name

commit cbc471cec26d3539cb6aa53a43d07c8bcac4b56d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Sep 27 14:14:40 2022 -0400

    support using names instead of fFriendlyName for networks i html and a few cleanups to backend code

commit d970500d4cb2576fb3c5202897f8fe2e06007bed
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Sep 27 14:17:54 2022 -0400

    html tweak display of network name in details page

commit 4a1707ed59dad86560e0f4bbdb9efa5eb4c95c17
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Sep 27 14:45:14 2022 -0400

    todo; html tweak display of network name when preferred chosen, and better docs / example on networks PATCH API

commit 4a473eab57d8a0c7c0264862363d0e0210882928
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 28 22:25:33 2022 -0400

    on change network usersettings, invalidate rollups for networks

commit 31ff8c5805225e3abf3f77020008e20360eaf07f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 28 23:59:01 2022 -0400

    did same cleanup / refactoring for devices rollup I had done earlier for networks; not really tested much but should be much faster doing rollups on large networks (as long as not too mcuh changed - may need to tweak for that case)

commit a5663550e275b5378bf3a9a6de5a636230698fbe
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Sep 29 00:32:22 2022 -0400

    fixed RolledUpDevices code so when change to usersettings we auto recompute rollup

commit 2a40467453f18105fbebe064c810723b17579c3a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Oct 2 11:41:58 2022 -0400

    workaround issue with nested object ptr references with db connection pointers

commit e8873671c19899a26c78cff8feb730db9aff7474
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Oct 2 11:44:13 2022 -0400

    adjusted rollup network fingerprint so discards diff between nets if they only differ in ipv6 addresses (as I had before)

commit 579ca358d0b171f3e101dc10b54d3451c5ea3155
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 3 03:42:27 2022 -0400

    capture network seen time in discovery instead of saying 'now' at top level of IntegrationMgr

commit 2ad156728b211bebf8a039b06f3a5c7f28007ae0
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Oct 5 11:07:11 2022 -0400

    more cleanups of backend network rollup code to better handle merges, and user settings flags

commit 53de281b6a3df0ddb1941e825f8acf2721a2bbe5
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Oct 7 14:36:30 2022 -0400

    support PATCH for /userOverrides/aggregateHardwareAddresses on /networks API (setting, but not code to handle rollups yet)

commit 0c2440dfa601431626d1bcd264fdb4db3ca03654
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Oct 9 12:36:10 2022 -0400

    Backend: start restructure Backend Network rollup code so rolledupnetworks is private and have explicit InvalidateCache method. Slight related device rollup refactoring (but more to go there)

commit 9948495d033f9d273b7a5b0f113043b2b12b7184
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 10 02:42:16 2022 -0400

    Backend: apply same refactoring to rollupdevices - now RollupDevices::GetCached() and Invalidate() methods and hide the syncronized list internally, and merge/fail return value for Merge_ call so outer loop can rebuildall once

commit 1ea19014a8881f74af6ebd4d68a024e26dad93c1
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Oct 12 03:08:33 2022 -0400

    fixed network rollup code to have starter rollups, and fill in from DB with user settings

commit 73273252d1531523e93c7fa2769211b8f4f803a9
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Oct 13 15:04:28 2022 -0400

    slight progress celaning up backen d rollup caches, but still broken (and notes in todo about fixes)

commit e141ff12961bcc61efa30a7376e0232fd6433871
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Oct 13 15:04:31 2022 -0400

    slight progress celaning up backen d rollup caches, but still broken (and notes in todo about fixes)

commit 547dad777bb9bd33252202cf129b8304304e5ba4
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 17 14:33:20 2022 -0400

    Backend/Sources/IntegratedModel/Mgr: lots of code cleanups/refactoring; sb nothing functional changed

commit 8de5820e44ffeb2e7218858fe33a7e553f905357
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Oct 18 20:15:38 2022 -0400

    improved timing logic for Discovery/Devices discovery loop by random scans(bloom filter)

commit 7ea2586ec19a11e717ba5184fc32e2e9e74b055c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Oct 18 20:42:16 2022 -0400

    cleanup reporting logic for issues starting/restarting ssdp listener/searcher

commit ecea5aa29bafd325693691e283db77232ab0092b
Author: Lewis G. Pringle, Jr <lewis@sophists.com>
Date:   Tue Oct 18 22:06:59 2022 -0400

    .github workflow update version of actions due to warnings from github runner

commit 86c4d99a1983f0e2bc7d3f28f67e9f706301e292
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Oct 19 16:17:09 2022 -0400

    support PATCH of networks userOverrides/aggregateFingerprint

commit 4adda24364a1e17ce5fa37c4f1f9f36c104d328a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Oct 21 20:33:39 2022 -0400

    latest stroika, and https://stroika.atlassian.net/browse/STK-940 workaround

commit 610a4e925bc0e30f583e5c5536cc3743ef75cbe8
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 24 10:05:52 2022 -0400

    latest stroila; delete /Workspaces/VisualStudio.net/Microsoft.Cpp.stroika.user.props from repository (autocreate);  .gitignore in Workspaces/VisualStudio.Net, and fix make project-files

commit 2683d3100774faedcb242c26fbd6fe8351b30057
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 24 11:14:28 2022 -0400

    latest stroika and more attempts/cleanups project-file build scripts

commit 9f21169ed5a3cb8fc5e1f95a1a934ce865b8d8cf
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 24 11:39:15 2022 -0400

    latest stroikal and symbolic link for Workspaces/VisualStudio.Net/Microsoft.Cpp.stroika.user-default.props now OK

commit 905b25678e1dd792d26f87ab1f085070f64583f6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 24 16:45:41 2022 -0400

    Workaround https://stroika.atlassian.net/browse/STK-943

commit 499606b7fd8cc1d7115753676c937bd4d7d75d16
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 24 18:01:33 2022 -0400

    tweak what we checkin to DevRoot/Workspaces/VisualStudio.net - lose .gitignore

commit 2377a63470b474028b124c495427c26caf9a990d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Oct 24 19:36:30 2022 -0400

    no longer need /Workspaces/VisualStudio.net/Microsoft.Cpp.stroika.user-default.props

commit 26f6dae4062691ebd653f7ce4a8d1a4abf028b42
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Oct 25 15:30:09 2022 -0400

    minor makefile cleanups based on latest version of Stroika Skel

commit 6e42260d24c1922792ce62bce899ff00871b1fed
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Nov 1 21:41:49 2022 -0400

    more cleanups to Networks rollup code, todo logic and a few related (unrelated) name changes

commit 75a6167f9336a39c84301f6d4aea42e9f3ca52f6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 3 13:55:22 2022 -0400

    small cleanups to Network/NetworkInterface contructors (explicit so no accidental converisons) and ToString methdo on NetworkInterface

commit b5908aa6df8c2f20f98028d68efa0c640657b0f3
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 3 14:00:12 2022 -0400

    status for network interface is optional in Model/VariantMapper code

commit 8d0f791673f82fe98a1c990452ff704a54c3bf50
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 3 14:04:09 2022 -0400

    lose qCompilerAndStdLib_template_specialization_internalErrorWithSpecializationSignifier_Buggy workaround

commit 04bbf44a27b73eb8fea2bba038492b51569afc06
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 3 17:39:49 2022 -0400

    progress on networkinterface rollup (but nothing changed yet in web service output)

commit 794ad2ab7ba7f7ddecf84c0de999200f1577528e
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Nov 5 11:44:31 2022 -0400

    todo notes, and network interfaces mostly working with aggregation - fist draft - but devices and networks aggregations not pointing them properly yet

commit 7dd4b07b11dbd5ad74c0e7743188fce05398f778
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Nov 5 23:16:13 2022 -0400

    NetworkInterface::Rollup () refactoring

commit fdb06cb70d84c8e0f9a0617d44632ad35f987a57
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Nov 11 19:21:24 2022 -0500

    latest stk, docs about discovery networks and slight progress on integration model networks

commit 4bb916e79d1b73d4c9820c02605c16329d82c1bd
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Nov 11 20:40:18 2022 -0500

    networks now show aggregated network interface ids

commit e3cb5438195cf47bd883a661262eb9565be4d68f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Nov 12 08:57:08 2022 -0500

    got devices rollup supporting rolled up network interfaces as well

commit 2821f32a90aff656180a69a248bef7fbcd80d3ab
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Nov 12 09:47:58 2022 -0500

    code cleanups and docs cleanups in integrationmgr rollup code - now I think pretty clear and decently done (but hard to test without GUI so can easily explore)

commit 789948b4559b95b6dc6bec85a3a6d86ef6a5b139
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Nov 12 13:09:03 2022 -0500

    docker container support for building wtf dev container for vs2k22, and a bit of support for forcing based on v2.1 or ?? not sure which I should force - based on - in container files

commit 89743508dd568b9f48922db826d236f57fd7d050
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Nov 14 22:11:02 2022 -0500

    Added newDev.fTypes += DeviceType::eWTFCollector; and a few costmetic cleanups

commit 57bca6646d857220f060cfd364cb7dfc7ccef3d1
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Nov 15 13:13:11 2022 -0500

    latest test build of stk 2.1.10x with new Map/Reduce code/names; used new Map/Reduce names and in more places than I had before; and lost no longer usefile filter-only-running WSAPI option; and lost kSupportPersistedNetworkInterfaces_ flag (always support persisted now)

commit 233c8b7864cc0f501e16f7598579fc4a143e57dd
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Nov 15 15:37:14 2022 -0500

    network interface rollup utility functions added - GetConcreteNeworkInterfaces and GetConcreteIDsForRollup

commit 3f09b3bf6955b3d68b8f73d5b45cd6358f8d3b29
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 08:01:47 2022 -0500

    added preliminary network user settings fAggregateNetworkInterfacesMatching support (draft not working)

commit 39279b7d64f6952dc1692f216a1d6a9a79d83e7b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 08:08:16 2022 -0500

    NetworkInterface::Rollup Ensure (r.GenerateFingerprintFromProperties () == instanceNetwork2Add.GenerateFingerprintFromProperties ());

commit 81463aa6ebf6e6e46ed1b887a01d08c3b8a0ef88
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 13:23:37 2022 -0500

    fixed generation of Network::UserOverridesType defaults for fingerprints etc - so we get better default behavior (but still must allow api/ui to update/revise)

commit 384e9b01ce843e2d4da8d787a19595d0fdf11586
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 13:38:35 2022 -0500

    lose the various DONT speciations in Device and Network UserSettings - since makes the definitions easier to understand (no contradications). See if we can make this work.

commit 09154a4d4ded3dd6d924760cf2c05bca20fc3303
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 15:19:55 2022 -0500

    patch support for NETWORK /userOverrides/aggregateNetworkInterfacesMatching

commit 71d4d9e5e55e4eab2e0c51b0c5a42d3f35d92bb2
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 17:22:46 2022 -0500

    Started adding Cache-Control support for GET /...{device,network,network-interface}/{id} - with 30s hardwired for now but in impl code where we can start to figure out right ttl

commit 96a3c73d23e33be830eddb0a9dfce2edd5d8e0d0
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 18:03:13 2022 -0500

    better support for computing TTL for get device/network/network-interface (id) calls - not great but good enuf for now to move on

commit e23d8921bfc409ce38e2bc57e3ed8cd7ddc5ad27
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 16 23:19:47 2022 -0500

    Model::NeworkInterface should not INHERIT from Stroika IO::Network::Interface

commit d24bd880ce6fd04fe6e79329be9079ddb2881f38
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 17 09:45:29 2022 -0500

    revised fingerprint algorithm (no real semantic change but changed what htey compute so upped version of DB

commit a6b3798ac8a6e7beaaad6248d2701af0ce8761fb
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 17 10:05:40 2022 -0500

    cleanup code for fingerprint generation on networks

commit b645a8a85fd32926180d48ba738df0327c0accdb
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 23 09:24:52 2022 -0500

    added debug code (dbgtrace) for linux only busy/spin CPU issue

commit 39bacfb9136c1009300a445e098b98ae3a5ebbb9
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 23 13:44:05 2022 -0500

    marked checked in TraceContextBumpers etc wtih comment https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/78 so can track down/remove when no longer needed to debug this

commit a075cc8afc2fe8c2dda608cb300e3e4814d505d1
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 23 16:02:46 2022 -0500

    upped db versionname cuz probably have stable DB format for now - moving on to doing GUI stuff for a bit

commit f258ac280fa8adb9e39ffdc3727ad3fdae84e66c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 23 16:16:01 2022 -0500

    npm upgrade and other small packag.ejson cleanups

commit 4ffd1c24d73956c7861a2467428654a9698e8464
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 23 16:19:36 2022 -0500

    html: minor tweaks to about page

commit 943902d0303f91794073419d42941a6b6380e9e2
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 23 16:43:38 2022 -0500

    html: new PluralizeNoun utility and used in a few places in GUI

commit 7d300d77eaa8a5ea0a875d3ef5289a0daf63ce41
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 23 17:06:02 2022 -0500

    html more fixes to network page(s)

commit b6d54571d87ccfd7540f9b36ce4306f6ef50dfb9
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 24 09:16:25 2022 -0500

    new components network-interface-details and used (very preliminary)

commit dc75062dea1c79dfad29ff8962aaeb1e207311f3
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 24 11:23:14 2022 -0500

    html progress supporting network interfaces display

commit 5ca6b7bf1a5e4348a90b6953c2dd9e45217de963
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 24 16:12:05 2022 -0500

    much more progress on network interfaces, including throttling the download of interesfaces and nearly finishing ui for viewing (ready to test a bit)

commit 1e0d216e7fcbb5e8b33c6f9a41b306a04b6c070d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 24 18:20:17 2022 -0500

    html: show wirelessInformation in network-interface statistics

commit 821d2a8883f582b9d9a6b722efd9ec0cf3087898
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Nov 24 20:45:32 2022 -0500

    hopefully fixed/improved default CacheControl settings

commit a75aa0eed81a5dc303d39bec3e99cbe769fd7718
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Nov 26 14:26:22 2022 -0500

    Backend:
      Support generating fAggregatedBy on Device and Network objects, when they are aggregated objects.
      And several small cleanups
    
    html:
       + Many cleanups
         - no longer call defineComponent (not needed - automatic using <script setup...>
         - re-implemented layout page support for breadcrumbs using new breadcrumbs events (triggered optionally inside router views)
           and new ToolbarBreadcrumbs component.
         - Device/ID and Network/ID pages now take advanrage of this to customize the breadcrumps to include names of devices/networks, and
           in case of aggregated items, adding a back pointer to the aggregating item.
         - In Devices/Networks page, now also allow name to link to (without opening a new window) the details for that network or device
         - Network Details component takes optional network object instead of ID, and used in NetworkPage


commit d22c929b995e575ef61db422a5f852c79a2b8f34
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Nov 28 09:40:37 2022 -0500

    latest stroika https://stroika.atlassian.net/browse/STK-960 fix

commit 616c3d6c50ab43e9fd03642b23d212a221aba8e5
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Nov 29 10:21:45 2022 -0500

    latest stk; added aggregatedBy support to NetworkInterface (gui code and WSAPI); lose (now obsolete) historicalSnapshot - since implied by aggregatedBy; todo notes

commit de406f037c645f9665474fcfa354af8a19a8a404
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 30 09:36:51 2022 -0500

    did first draft network-interface page, as well as updating compoennt to optionally be given arg interface object instead of id

commit 6540aab53602a3eb032bd6d29a14122c35eaef01
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 30 10:15:13 2022 -0500

    html more cleanups - coding style - typing

commit 6a33af5735d98eabb80771fc93cddcb850a85665
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 30 14:51:56 2022 -0500

    tons of changes: Backend RolledUpNetworkInterfaces now creates attachedToDevices (but possibly buggy - some debug messages left in cuz we are missing some data); and display support in getSeenForNetworkInterface and FormatIDateTimeRange and showing seen information on network interfaces now (mostly - still needs work)

commit cd7a694c8b4975673f4550bb26a02eed8d3252b9
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 30 21:18:35 2022 -0500

    html: use throwIfError_ () so 4xx and 5xx errors treated as exceptions in html code - not just returning bad objects

commit e7f322fd6396642f2125b95fc5c2004f35262ca2
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 30 21:19:34 2022 -0500

    cosmetic, and lose import of defineComponent, defineProps, defineEmits since not needed (warnings from compiler)

commit 6494422c35fc1d36967e98b5c9e5c7b7dc52af66
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 30 21:21:26 2022 -0500

    worakround issue with tmp code for RolledUpNetworkInterfaces CTOR - work in progress - but get compiling for release builds

commit 9c30f1cb466243adf1780c688d01704246f55795
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 3 15:36:23 2022 -0500

    Cleanup Network rollup code handling of orphaned raw interfaces (just document - not a big deal - documented why and prepare to handle at some point in the future)

commit c7ecb6fc89e358675c8444274b37350faa7ef7aa
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 3 16:12:26 2022 -0500

    Added ticket https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/80 to track minor cleanup

commit 713e0c5f6e5c6d64c26805984e281a52d19918d6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 3 17:30:18 2022 -0500

    cosmetic; and fixed Mgr::GetNetworkInterface(BYID) for ROLLUP case, to return the appropriate fAttachedToDevices field

commit 06072dd77b10848674bf55d9dfbed1ff087f594e
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 8 20:36:23 2022 -0500

    tweak Cache control values returned; updated todo list;

commit e98626d42283e5a4856826a3bade0af99e3d5e3f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 8 21:16:42 2022 -0500

    Changed projectfile to NOT have WTF depend on Stroika. It DOES. But GUI of Visaulstudio works better with this setting

commit 19290ed006bd152936ecf7ce3ece6c18a7c5f6f3
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Dec 8 21:30:31 2022 -0500

    more cleanups - use fID instead of fGUID for Model::Device and a few places cleaned up new Map<> usage

commit 95e4ffc762930e7e77943c1aa43b16c0875e9b82
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 9 09:56:26 2022 -0500

    for device details page, show 'Seen' field for attached networks

commit 838c8f897c37bcd1b0bd74b7a0b6d1dbddeb2a31
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 9 10:24:28 2022 -0500

    use less of currentDeviceDetails.attachedNetworks but still use some

commit 4d1251669de999b3271c88d6a8a4cc7c549f8fb1
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Dec 9 10:24:46 2022 -0500

    use unused include (html)

commit 8e06684f24e92cd76cc8e97473b31816102d2465
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 10:07:08 2022 -0500

    Added check to handle bad data in ws call so we always set d.name (avoiding error later) - and console.log

commit d5f1eeae4bd33b04dc51657f981fa0fe88af3b6c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 10:09:11 2022 -0500

    html: fixed fetchNetworks code to also patch INetwork objects

commit 44c7a634bf9ea2420bebecd05dd64a0c6159cf8a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 10:11:41 2022 -0500

    replace a few more uses of momentjs with luxon

commit b6ab421789360fd4c978c908f7aa8305cabb8673
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 11:30:28 2022 -0500

    check kCompileTimeConfiguration.DEBUG_MODE before doing console.log

commit bd3696efc08f0b9bd4552f78692a375563806dd6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 11:52:25 2022 -0500

    On Device Details page, show checkboxes for showOldNetworks and showInactiveInterfaces and use to filter those details (much more todo here but this is a start)

commit c5378a536e23b118203bee255826790e72b8fecc
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 11:52:59 2022 -0500

    Improved aggregation of devices display (though not testable til we support irreversible aggregation)

commit 1866931fb220a27d80aa30985d409a773c0913c7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 12:08:19 2022 -0500

    showSeenDetails option on html device details page

commit 647632708fc6c53551f56656fb01914d96099a2c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 12:19:18 2022 -0500

    html: cleanup devices summary of use device details and links to device details page links

commit 3564558006d4a1c28c3e4a3b97d8ee6ca043aabd
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 12:34:08 2022 -0500

    html: more modest tweaks to devices / device page links

commit 8bc40db71924e56ac84a4a2a94155e5cfc535e5d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 12:48:59 2022 -0500

    refactoring: use FormatIDateTimeRange where we had hardwired in the code its basic implemtnation

commit 20e0118642eae562feb6384ea3cd7cc9b2dabf9f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 13:02:38 2022 -0500

    device component now takes either deviceid or device object as parameter

commit 062fcc4731fc9b8076e3b99646450e6b29305b2c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Dec 10 20:51:29 2022 -0500

    todo notes; and start at (just debug info dump) support for UserOverrides (show but dont edit yet)

commit 9a561f94c64cea41c42477992c94a0c2a867b165
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Dec 11 09:52:05 2022 -0500

    used PluralizeNoun() utility more - so we automatically do pluralize properly for fields in html ui

commit 087850e9c86ac5332c5adbc5956dd4213438232a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Dec 13 15:21:46 2022 -0500

    Backend: WSAPI GetDevice(ID) returns slightly longer TTL for rollups, when we haven't seen it in > 15 minutes (except at startup where that info is unreliable)

commit 86606ed0bca31aedb6b6a0f5ca443755fb80c21a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Dec 13 15:34:55 2022 -0500

    new /html/src/utils/Objects.ts for Equals function

commit c6f9943ccd9c355316c4766c7f141f388e3f5e60
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Dec 13 15:38:45 2022 -0500

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/71 - mostly restructure get device code to fetch without recurse=true, and separately fetch each item (trying to leverage ETAGs and conditional gets and Cache headers) - mostly works - not clearly better

commit 199c15ea9a56f75da399aba7585527a393921245
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Dec 13 17:18:32 2022 -0500

    html API proxy cleanups - mostly use async style over .then/.catch style (mixed style confusing)

commit 5f2a99ee4baac348045e54f1f8a220800a2e2b28
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Dec 13 17:30:26 2022 -0500

    more cleanup of return=recurse stuff in html code

commit 1e4f6fa6fd3e39a8e51fb263e8a3c1c97acd1726
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Dec 13 18:04:33 2022 -0500

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/71 completed - I think -but for cleanup and review - now avoid using recurse=true APIs from html - fetch pointers and then separately fetch the objects to try and leverage etags etc

commit 833bd1a9710e6a936d3dd1e20adb03fc4dbd2af8
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Feb 23 21:36:00 2023 -0500

    a few small stroika v3 cleanups and some name cleanups

commit cde42c9a32d62c5382294bc9ca705942b3f9f7bf
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Feb 23 21:55:40 2023 -0500

    loosen network rollup code so MapAggregatedNetInterfaceID2ItsRollupID doesnt assert when not found cuz caches not necessarily coherent

commit b67e9c25c38e7f129330ce08d56ae4cda728415d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Feb 24 09:37:00 2023 -0500

    cleanup MapAggregatedNetInterfaceID2ItsRollupID

commit 30009848f71df39da48aefc55dead0200e0850ee
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Feb 25 12:54:44 2023 -0500

    IntegrationModel (Mgr) factoring

commit a4676d566f2a3dab5758d26d8f71de868ff93548
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Feb 25 13:25:31 2023 -0500

    migrated KeyedCollection defs for IntegrationModel objects to Model layer, so can be used more widely

commit c8a2adc0363c61878a32b4bd16a0aabd8d779d44
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Feb 26 09:26:43 2023 -0500

    factored out DBAccess code from IntegrationMgr (mostly)

commit 22f5c8cecd98d57ae2982848603ffefda04b20cd
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Feb 26 10:46:18 2023 -0500

    docs/cleanups to IntegrationModel factoring effort

commit 2ab70b5d38e975fda45cec1ad6d19d177c257974
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Feb 26 12:12:14 2023 -0500

    factored RolledUpNetworkInterfaces out of IntegrationModel::Mgr cpp file

commit 7a9f17dce45af67c775e432c9f8d5d778da54b10
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Feb 26 13:18:14 2023 -0500

    factored out Private RolledUpNetworks from IntegratedModel code

commit 6edce911227ce042e0177c1566e3eab3326182ce
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Feb 26 13:56:50 2023 -0500

    finished factoring of Private stuff to subfiles for IntegratedModel Mgr

commit 361fda8c58e38a361668dad97649380efa205f3f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Feb 27 05:30:06 2023 -0500

    more cleanups of IntegratedModel Mgr code - in preps for tiny post-DBLoad processing addition

commit cfab53e1991577b0f74cb8e099de4fccb2a0e982
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Feb 27 05:31:26 2023 -0500

    more cleanups of IntegratedModel Mgr code - in preps for tiny post-DBLoad processing addition

commit af0884f283b393a6b460011d9c4512e0eb0c5778
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Feb 27 12:16:03 2023 -0500

    on load, new experiemntal PruneBadNetworks_ () logic (anda  few restructurings to accomoidate)

commit 75b16f45c7d021a9ec93ed6ff38afb84bd1d38fa
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Mar 2 20:25:48 2023 -0500

    for backend, use Options::ThreadingMode::eSerialized instead of Options::ThreadingMode::eMultiThread as experiemnt, cuz seeing flakies on unix, and saw one thread mutex issue (inside sqlite in debugger) on windows - see if this is related

commit 86c21f2475739bd73235e3d555d6569fb0b322d3
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Mar 2 20:26:55 2023 -0500

    lose selectedNetwork in pina store; and fixed up device loading details ids/ and basically the suppression of extra calls code - hopwfully right now

commit 0cfff8304ef7f148ba3a931db460d894ced0b138
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Mar 3 10:56:22 2023 -0500

    use flatMap instead of map for Net-State store to fix issue with finding null ptrs on load

commit 061dd8d3821ff6ed765492a2f99d53f7fe45fa52
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 5 18:08:11 2023 -0500

    some cleanups on network details page for editing network name - but quite incomplete

commit 0b60cae2dbe709ceea7513ec93739d183337add7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Mar 9 10:17:46 2023 -0500

    progress on edit network name code, but incomplete and buggy (html gui)

commit fd6d9616d04c6f76f77c8973c8069d328bee42b4
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 11 09:29:07 2023 -0500

    mostly fixed network name editing (all but event gen and listen and maybe cleanups)

commit 4b682c87d86d15d78793008ddb5b179fcdbe4eee
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 11 10:08:25 2023 -0500

    todo notes and network name edit ui cleanups

commit e8a3ca924519aa6dd1b54385880cbdd9eb0fd50a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 11 11:26:02 2023 -0500

    factored new PopupEditTextField.vue - works but needs paramterizing and cleanup of call

commit b9937dc18a0baff2296e3d7fff2d1ad9de48bbac
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 11 11:38:16 2023 -0500

    on pupeditextevfield factored configurable values

commit a7df68d87a5e02bcba95ecb88e2f21a4c8df0a44
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 11 12:07:27 2023 -0500

    solid cleanued up ose of PopupEditTextField in network name field editor location, exccept for WSAPI call to set/patch it and refresh

commit a2c6810e65c0da49085386aeb10225e37c6d0992
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 11 13:44:13 2023 -0500

    patch support for patchNetworkUserProps_name and used to set netowrk name from GUI; and added notify plugin support for success/failure updating

commit 9cd3fd3e4543a9d0ad846ffa512f418d176e5b94
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 11 20:43:18 2023 -0500

    latest 2.1 stroika; added edit-device-name function to device page; and todo notes

commit 50257ad7ce15347a09bab9de751afb8b22ab6cdf
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 12 16:01:52 2023 -0400

    in networks page use click.passive so we dont open box AND follow link for link/ahrefs; and did same logic for name/link on devicenames on devices page

commit 7133b6a9c71ee7d97ccdcd03c4940459f4006489
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 12 16:41:03 2023 -0400

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/39 supported device and network notes

commit bcad038a92cdc7e9db06c1ad81dbd736665603c4
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 25 11:50:57 2023 -0400

    new config ubuntu-22.04-g++-12 (Debug-tsan-ubsan) to test WTF with tsan

commit 90278bc3f774f0da3acb0c403c89b69e225595ac
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Mar 25 18:01:59 2023 -0400

    lots of html ui cleanup to watchers and breadcrumbs names,  and <q-card-section zero padding vertical not margin

commit 85cd678f27641ae2039ab4e1b3037d3bb92922f7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 08:44:44 2023 -0400

    cleanup - where we put snapshot in some html pages

commit 8ec1f7ab3d7e47f9bd934d9468033cb3eee65558
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 08:50:16 2023 -0400

    prettierrc set to endOfLine auto, and re-ran

commit e4e1f5f5f0a0d5bdf584af4bf83d55e70aca579b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 09:27:25 2023 -0400

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/87 - added main menu to left drawer side; about to cahgne RHS to context menu

commit 4376a75169b2a75e7b7603465d52be0669377636
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 11:43:03 2023 -0400

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/87 - added new context menu

commit e194337de878752683d3ad922427f0298ce55ede
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 11:43:10 2023 -0400

    https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/87 - added new context menu

commit b1ff89cc87fce1d118f63f0a7b6a160e9da43ed5
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 13:37:41 2023 -0400

    minor cleanups to html new menu stuff

commit 11723eea9c2c3a4500f2fc1e652596bff7a9598e
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 13:55:50 2023 -0400

    cometic; and https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/88 collector icon

commit 94df4fce9b79d43e2377656055ecaa6acf5b08db
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 26 13:55:54 2023 -0400

    cometic; and https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/88 collector icon

commit 5c93642fe263b01da989992abf92d3b89e8f7b82
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Apr 9 11:27:58 2023 -0400

    moved rowClicks event from row to toggle icon, so slightly less confusing ui about clicking

commit f70d2d981c58a704afd6e88500fa29f3a634e06c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Apr 9 18:39:26 2023 -0400

    mostly comsetic fixes to src/components/PopupEditTextField.vue

commit e09bf4dfc5768636f0310e72d9d5d41175041df7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Apr 9 18:40:18 2023 -0400

    first draft (seems to be working) tags support for devices (still to cleanup and do for networks)

commit 0772e66e6f26a3450a3d64df346cafb4801f0e77
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Apr 10 10:13:23 2023 -0400

    todo cleanups on html/src/components/PopupEditTagsListField.vue  - work in progress

commit 4f38c5dca438071bb58a798432c72d91d9ed2e94
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Apr 14 09:28:47 2023 -0400

    fixed bug iwth tag updating (but about to redo whole UI)

commit b1e72be1b1e94ea510b8dc8f03cc8a3445562d4b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Jun 15 13:39:46 2023 -0400

    cosmetic, and accomodate latest stroika v3 dev brnach(but not updated dependencies)

commit 7f0c58787cfc8cde41c33a802e05d1780fc596e4
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jul 9 12:44:06 2023 -0400

    todo notes and partial debugging progress (commented out) in tags code

commit 67474a9b9997a9ba49922e6831d4af7010f606d7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Aug 15 13:56:44 2023 -0400

    Several fixes - but MOST IMPORTANT is each TableCOnnection uses a single DB::Connection::Ptr; and other minor cleanups and things relating to Stroika v3 compat

commit fcc4e631cfe2e6afed348c3bddbf9343d757dd10
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 13 14:48:27 2023 -0400

    go back to Options::ThreadingMode::eMultiThread;

commit a2373f47e761d23bb87e32cf6d323e06a620c0a7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 13 15:31:05 2023 -0400

    lose support for building for Ubuntu 18.04 - too much of a PITA to get npm working there

commit 8ccffbeab92535488358972a843f50139ff77258
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Sep 13 15:46:49 2023 -0400

    update to newer way to install latest npm from docker container since old way gives deprecation warnings - in docker contianers for ubuntu 20.04 and 22.04

commit 9b74427967ca595f850b5e524f004b871126502b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Nov 6 11:53:41 2023 -0500

    added new Profile configuration

commit aee2c69aeddcc7d0914f880b86c4454919562689
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Nov 27 21:20:31 2023 -0500

    start using latest Stroika v3.0d5x - and assume c++20 compiler - so lose #if __cpp_designated_initializers and use more recent Stroika fatures (starting)

commit 811525e655330a762b952be5d2950376f34b7216
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Nov 28 22:44:47 2023 -0500

    a few minor lceanups for use of new Stroika; and added webserver connectionmgr stats to 'about' WSAPI info (just rough draft of this code/data/api)

commit bc1cbc1da14bec122626622c8889cc8010c23007
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jan 14 18:37:19 2024 -0500

    make distclean; and updated to latest stroika

commit 754e13841713fd7da60936ef3eb4a0f9c413ccea
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Jan 31 13:14:06 2024 -0500

    various makefile fixes (clobber message , distrclean and latest-submodules); and tested latest stroika

commit 0eb101356e5961abc2c2cebb51fc47edd2b41448
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Mar 20 12:51:43 2024 -0400

    latest stk (broken now); make format-code; and a few cosmetic renames; and update String code in Model (about 1/2 of it)

commit 2ef89e7461bc4c5eb187e5d7b858f78ca13f8009
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Mar 20 13:00:37 2024 -0400

    updated commandline code so works again with latest stroika

commit 14a4e984dda9a5e4aefc2c6dda0c6c279b42d5e4
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jun 4 12:23:51 2024 -0400

    test wtih latest stk pre-release

commit f516f7075711c5f44c4780a5309bde346ee6d308
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jun 30 16:02:50 2024 -0400

    .notparallel makefile fix; latest stk pre-release test

commit 825a097ca12cc625dcb023a34b6ae369de0e88dc
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 9 20:52:57 2024 -0400

    Simplified mapper.AddClass<NetworkInterface... code using improved overloads/mapper API in latest stroika

commit 62296cbac501057ee335b9a1dcf26cc2feadcd58
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 9 20:58:45 2024 -0400

    cosmetic cleanup to mapper.AddClass<> calls

commit 6f3b1fd70282769544d8c7985b44a6fd81127dc6
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jul 14 12:42:51 2024 -0400

    latest stroika (3.0d7); cleanup use of ObjectVariantMapper::StructFieldInfo/DataExchange::StructFieldMetaInfo in AddClass calls; lose some __cpp_impl_three_way_comparison < 201711 workarounds

commit 9560f1eaa046ab95437ea77c80e91aa4028ed388
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Jul 15 08:25:32 2024 -0400

    test latest stroika pre-release and updated AddClass usage to new style

commit 9fcf7743d4378f70f75dc666814a280887666b3b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Jul 15 10:47:03 2024 -0400

    WTF configuration has Logger config section now and ability to log to stdout

commit b3a8eb0bfa3854bcb6e7737a42b0948eeda299fd
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Jul 19 11:10:53 2024 -0400

    new dockerfile for windows build container

commit 92d43cfe4392bd9bdd380cb948a72d56591cb2e7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Jul 19 11:15:58 2024 -0400

    new workflow workflows/build-Dev-Docker-Containers.yml

commit f884e794151826eddd6a8f93e943447648df4974
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Fri Jul 19 12:01:10 2024 -0400

    fix issues with docker tag images (makefile) and added Ubuntu 24.04 support to build containers

commit 2708eee71c65c1b4a9f00f28ee76fb8fa6a878d3
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Jul 19 13:04:54 2024 -0400

    lose Ubuntu-2004 dockerfile support

commit b734b9c73354432a9a004242df46d5e1555a225b
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Sat Jul 20 08:45:58 2024 -0400

    .github/workflows/build-Dev-Docker-Containers-Matrix.json

commit d0511dd344280b8f8a9d54ab34248578c7556029
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Sat Jul 20 09:40:51 2024 -0400

    fixed default configurations for more recent stroika

commit 631bb84f140abc15a89bc8be3e334c30e80936d3
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Sat Jul 20 15:23:59 2024 -0400

    minor kStroika_Version_FullVersion <= Stroika_Make_FULL_VERSION (3, 0, kStroika_Version_Stage_Dev, 8, 1) BWA hacks - incomplete - for g++12 / formttable

commit ae1f8a3d7bf908a615b35a97e5a2cfc3b4ef7707
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Sat Jul 20 15:43:57 2024 -0400

    draft build-N-test.yml workflow

commit 0351c57a98d2e2b89a24dbda3642bd47bfb65d82
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Mon Jul 22 13:50:24 2024 -0400

    Added quasar to docker build for windows

commit e74d70413e797c2abb54084779e3ebfeb70d6e38
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Wed Jul 24 12:35:16 2024 -0400

    try compiling on github actions with C++20 for now

commit 0b8678cbd7834bc2bb0956fea7e1ad1ba81914a4
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Jul 25 21:15:40 2024 -0400

    use latest upload-artifcacts cmponent in workflow

commit ad9057b5c981cda418b63fcb7efabafe875f41e2
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Jul 27 12:55:08 2024 -0400

    react to new stroika .fAfter in ObjectVariantMapper

commit e1e9a031db8e0017cc357f90c37babf99936cb3b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Jul 27 12:55:35 2024 -0400

    fixed dockerfile install quasar for windows

commit 6468036a390e83be77c2b5c6ef0cc1ab8b72bd3a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 30 10:57:53 2024 -0400

    now run github action workflow on push (new workflow)

commit a06c2ce855fd0dfe9c3ac58e2f14cce2f440924a
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Tue Jul 30 13:21:37 2024 -0400

    Minor makefile cleanups to remove Add '+' to parent make rule. warning; and related

commit 19894d483fd31e83edc39e42e72d36b241306729
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Tue Jul 30 13:23:04 2024 -0400

    ECHO_BUILD_LINES=1 hack in github workflow for windows to debug why fialing

commit 2a565f17bed53c43b93288f9c5ed11393141c3b0
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 30 15:07:24 2024 -0400

    added a few .github workflow configurations

commit 91592e3d2f713d26eff8f661f320c910183c54f5
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 30 15:08:19 2024 -0400

    use CPPFLAGS not CXXFLAGS for Makefile -I lines, and added a few missing ones in a couple makefiles to (hopefully) address issue building on github actions

commit ded1e14ea4a73d2ede5eddfe388a8e49f721ad40
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 30 20:15:10 2024 -0400

    another try to debug issue with wtf build on github actions

commit 5edde7fb1c9151521c80aa83acdbc079421455d7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 30 21:24:27 2024 -0400

    maybe some weird thing about -I path ending in slash not working on github windows action - crazy  - but test

commit 7856237dd0e0765bf47c32b587b266acb33f4e7d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jul 30 22:39:33 2024 -0400

    maybe some weird thing about -I path ending in slash not working on github windows action - crazy  - but test

commit f0b3abf37803b2de11ff05238f09a62f75c23235
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Oct 22 12:40:25 2024 -0400

    tweak html make to not build html if not needed

commit 032cec9ea98c2b63a30f61f71d14b3c6907b7508
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Nov 13 10:35:00 2024 -0300

    use latest stroika and switch from deprecated mkRequestHandler etc

commit a321947ca7bee8b1e8cc61bc333cffb3f47eb518
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Nov 19 12:24:43 2024 -0500

    latest stroika; make format-code; log name of datebase and a few other minor cleanups

commit c8450a8a6d4bbe60296a205b6c87dda31442dcf2
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Dec 11 15:26:06 2024 -0500

    latest stk prerelease and a few updates to accomodate deprecations and cleanup and api improvements

commit 5b203432ccec4e2fc049e80a753db5f39d459a99
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 7 13:48:58 2025 -0500

    applied healthcheck and connections api changes from HTMLUI sample to WTF

commit af613a00039b2292523ffd4e82565f99c145f0cf
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 7 20:00:15 2025 -0500

    minor cleanup and latest stk

commit 07fd4f301c39704d9892f1b639de98ac62e6d716
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Feb 10 10:12:05 2025 -0500

    latest stk, and react to a few changes and format

commit af127e54b7cc04c8c956f52f50176c6f2897cc50
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 26 11:29:40 2025 -0500

    latest stk; and react to some depreactions and changes in HTMLUI sample

commit 21ef1a97af71c727f0679341043f40d9746ec868
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 9 09:42:53 2025 -0400

    Cleanup messages about startup issues with firewall and failed start ssdp listener/searcher

commit 4a982e5c41450d9b6b549e77cfb0beac652155a2
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 9 10:04:46 2025 -0400

    npm upgrade, and audit/fix - changed to newer quasr etc - vite, and ts quasar config

commit 1ddaf8f49b59fd0a391ba3523e6f156948ed9927
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Mar 9 10:43:47 2025 -0400

    update About WSAPI and GUI - for chagnes made in HTMLUI sample (webserver stats and reduced api stats)

commit e9b940165516ae736d43be636cc6ddc36fb3644d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon May 5 20:24:48 2025 -0400

    solved busy-timeout issue - (or worked around): options.fBusyTimeout = 10min seems to work quite well- still must better udnerstand, but good to have working decently

commit d73c825c3cac26bf5e7b3725590c528b3a609577
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Jan 10 21:11:30 2026 -0500

    HTMLUI - package.json use node: 22 or greater; and npm upgrade

commit 8ccb2d3e9e68baabaf22537f7dc1d6075f184804
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Jan 10 21:18:33 2026 -0500

    changed build docker containers to use node 24

commit e612f784c1122dba648363dcba8e275e23d42d42
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jan 11 20:22:09 2026 -0500

    lose obsolete  .github/workflows/build-N-test-v1-Dev.yml  .github/workflows/build-N-test-v1-Release.yml (use build-N-test now)

commit e4cea3cffeffc20c8987d94da6f5d2e99f7a3027
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Jan 11 20:25:23 2026 -0500

    fix github action runner for windows to use windows-2025 to match docker container

commit 9d1952b74de25ecb16b9afaf31b348efed670394
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Jan 12 07:14:23 2026 -0500

    fixed missing make buildroot in github action for windows (so didn;t build on windows/github actions)

commit 1aa534c221aa5bfd8d795289bdd23f9cfdf1c5b8
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Jan 12 09:06:35 2026 -0500

    fixed missing make buildroot in github action for windows (so didn;t build on windows/github actions)

commit a564342ddfcb9aa408d4ad0088ec8024294d4837
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 11:19:13 2026 -0500

    fixed a few caes of using operator<=default that should have been operator== = default

commit 788739f804110294f6bf65c93042e78f3601bd33
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 11:30:56 2026 -0500

    git action: try to archive windows installer/build artifacts

commit fa7fd4d53266734c586a32fea1be60e654128e85
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 11:33:32 2026 -0500

    github action - added macos configurations to build

commit 586ff6855ae1ca06d9cb3809a3a55dac942c0e67
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 15:30:33 2026 -0500

    github action progress for MacOS

commit 18db91de70eb73d91971721de871c4d2c21673b0
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 15:41:32 2026 -0500

    more fixes to github actions MacOS (Debugging)

commit ac45c6a2748d264820ec0c4d442bbb1e9c64bfba
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 15:44:52 2026 -0500

    more cleanups to .github actions for build macos (and tweak windows)

commit de21af16ac573cb439c439e63ce9b1157e3411b4
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 16:41:39 2026 -0500

    progress on github actions for building macos

commit 4ed9346d0e462991b5256e0636be87ff6185d54c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 16:58:43 2026 -0500

    progress on github actions for building macos

commit f1cc794102736e05739104b9e78787cef31fc40d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 17:41:31 2026 -0500

    progress on github workflow action build for macos

commit dd50200a4b40c8c366a7026fd7f95b51313ec766
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 20:13:14 2026 -0500

    progress on github workflow action build for macos

commit fa38c8b25e6cce6243163179cf6cffa778298bfc
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Jan 13 20:44:38 2026 -0500

    fixed warning - IntegratedModel::Private_::DBAccess::Mgr should have virtual DTOR

commit 2df5e574bfbe128fff9820610bbe4b983fac71bb
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Jan 14 10:13:05 2026 -0500

    various cleanups to accomodate last year of stroika development - including important fix of #if qPlatform... to qStroika_Foundation_Common_Platform_...

commit 175b219dc354617cedfaff8b312b01afb9d4f54a
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Jan 14 10:20:47 2026 -0500

    minor cleanups to .github workflows - especially macos archive results

commit d7e345393a6c9e6c1bb66bff1a4e8aedf21efc4b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Jan 14 14:13:31 2026 -0500

    htmlui: cosmetic; and fixed small bug in breadcrumbs UI (href not :to)

commit f1a93dff3f1b7980aa43147d59c4c449864be625
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Jan 14 14:35:21 2026 -0500

    try 1min for busytimeout on sqllite

commit 72ff4655b7e112256871e0143b2f8ec03f31a1a7
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Jan 14 14:35:39 2026 -0500

    SubstituteBackVariables makefile fixes

commit 279bb5fbd2dc79132e3e90708676fcdbaa44016b
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Wed Jan 14 15:18:38 2026 -0500

    fixed rpm installer (missing files)

commit 90d59aa40adcdca64aabc8fdf0ad0067caddc29f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Jan 24 09:54:33 2026 -0300

    early draft of qUseNewDocumentDBAPI

commit cffdf27a5c85f322b13a834834a84e68469f46e0
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 4 07:16:03 2026 -0500

    html: lose some obsolete uses of momentjs, and added lastSuccessfulAPICall and used in about page

commit e69fca7794c5408c997cacbb3ffbfaac57b89cfe
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 4 13:15:44 2026 -0500

    fixed small bug with device discovery backend code, and improved logging about sDiscoveredDevices_ succeeded so clearing retry count message

commit 29f8709bbd2576c293df98a6fc9e684846bccc39
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 4 13:17:15 2026 -0500

    html: small cleanups to about page (write last message)

commit bd84696033a43cce6961b8914cc4e96d3cc862b9
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 4 13:38:18 2026 -0500

    html devices page cleanup - lose momentjs

commit 67d9e70807b90c462dc76c65b1a7906dfb742c39
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 4 14:10:55 2026 -0500

    html: mostly cosmetic but fixed home page to use luxon instead of momentjs

commit 96a75eddc8a5df4610004e6436cb3833da63294d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 4 14:17:51 2026 -0500

    networks page no longer uses momentjs

commit cfffc6d55f443ffb10f4547b4cf5414b81a01d0c
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 4 14:19:10 2026 -0500

    npm update and no longer refrence momentjs package

commit a6686c85db628ea1a08a4a5f70bd8dbcb9eb2456
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Feb 6 18:15:53 2026 -0500

    latest stk; GetDBFileSize () now uses Document::Connection::Ptr; turn qUseNewDocumentDBAPI by default; incomplete draft of mkOperationalStatisticsMgrProcessDBCmd () support for qUseNewDocumentDBAPI

commit b377b5383fbf41e984dfff0c4ce9b2603b516862
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Feb 7 12:53:49 2026 -0500

    fOperationLoggingCallback set for DocumentDB mode and fRetryOnSharingViolationFor set for windows, and other minor cleanups

commit 304a85bbd699169cbf60fdc2b5ef4cee24d546dc
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sat Feb 7 18:50:16 2026 -0500

    latest stroika; tweaked some messages; cleaned up MAIN logic for configuring logger and other startup stuff (using DefaultLoggingCrashSignalHandler etc)

commit 21001babd6e1e9e18108481bb7750a10f18b175b
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Feb 8 10:05:23 2026 -0500

    document html prettyPrintMSDuration () doesnt work right with sub-ms values

commit 3ecba963de84a4aed5d1a04345066a5290de9590
Author: Lewis Pringle <lewis@sophists.com>
Date:   Sun Feb 8 10:07:14 2026 -0500

    configuration support for storing SQLiteStorage/DirectoryJSONStorage/SingleFileJSONStorage configuraiton data; improved logging about startup opening database and app config file location; fixed type in name of networkintefaces (NetworkInteraces->NetworkInterfaces) table

commit 284a474d28dc3707fe66283604cfb43381e6fc04
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Sun Feb 8 10:35:31 2026 -0500

    update scripts/docs for running docker based builds - use ubuntu 22.04 and 24.04 for now

commit db23336c0bd4ad532b7a449a3892dd05cdc0d775
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 11 11:07:31 2026 -0500

    html: prettyPrintMSDuration () now handles microseconds

commit f48e13a78e77e884ac4031e978aa93fe132a966e
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Feb 17 11:18:31 2026 -0500

    Updated DBStats to use ICommonStatistics

commit 778e221117e9e612ac8575d434cedad56d467dab
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Wed Feb 18 10:27:03 2026 -0500

    embelish shutdown message with 'activity'

commit c69f644d0b4a13c707911339e174ad997fa3846d
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 18 10:53:01 2026 -0500

    Added Execution::Thread::SuppressInterruptionInContext in a few of the activator destructors so they are assured of completing during shutdown

commit 4985067e9ec2493b0b3192be004e57b188dee885
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 18 14:44:17 2026 -0500

    cosmetic and upgraded DbgTrace usage to new format string

commit e9c4d907f41d6d906486ac931a14252bcabb8f6f
Author: Lewis Pringle <lewis@sophists.com>
Date:   Wed Feb 18 20:13:09 2026 -0500

    lose qUseNewDocumentDBAPI #define - just always use new way, and a few other cleanups

commit 3d3eddac0e8dcd6ddeb4eed42dea073a9bfe7299
Author: Lewis Pringle <lewis@sophists.com>
Date:   Thu Feb 19 23:05:02 2026 -0500

    latest stk; new config file 'BackupData' feature - if present, writes backup.json file summarizing data in one easy to read json

commit 76efbda8d9fc6eba67204e31a08427240f853eca
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Feb 24 19:53:14 2026 -0500

    latest stk; support DBStartupLoadFrom config feature (loading db from json file); and keep a single statatic copy of DB object so only initialized once

commit ed9f92858dd53fa85e80bbeedc02da052e533cc0
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Mar 16 15:39:11 2026 -0400

    latest stk; log Debugging Log2TraceFile name, and minor TimedCache code cleanups for recent stroika changes

commit 9f00631fcc5f099a9d7002972633daa75f185524
Author: Lewis Pringle <lewis@sophists.com>
Date:   Mon Mar 23 16:51:54 2026 -0400

    latest stk, and use cleaned up TimedCache support

commit 55e26bbe30fba7ea8d2b21dded8c0364be9a2444
Author: Lewis Pringle <lewis@sophists.com>
Date:   Fri Mar 27 10:59:08 2026 -0400

    latest stroika; and react to name changes / deprecations with TimedCache/SynchronizedCallerStalenessCache code

commit 56f6e787315ce9d6b35f39fbe538e1cbbc725453
Author: Lewis Pringle <lewis@sophists.com>
Date:   Tue Apr 7 08:26:29 2026 -0400

    Code cleanups to RolledUpNetworkInterfaces (rollup/get cached), and debug messages to debug why sometimes hangs/crashes on unix

commit 8321142bc01f40761a1dd50be4d0619b88837bdd
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Sun Apr 12 10:12:54 2026 -0400

    fixup use of locking in Cache usage for RolledUpXXX

commit 3099119f08d8de1a54edd6c3875c87199f4773c5
Author: Lewis G. Pringle, Jr. <lewis@sophists.com>
Date:   Sun Apr 12 10:13:16 2026 -0400

    Added Release-Logging configuration

#endif

----



### 1.0d19 {2022-09-10}

#### TLDR
- Use Stroika 2.1.4
- Better handle SQLLite BUSY exceptions (https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/35)
- Dont discover devices with no hardware address
- Re-use (where possible) rollup ids across runs (https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/69)
- html
  - Use luxon to fix dates sorting issue etc
  - Primitive throttling of requests
  - Added network external address

#### Change-Details

- Backend
  - Common/database
    - https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/35 - SQLLite BUSY exceptions timeout changes and comments to ammeliorate
    - db v13
    - https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/35 - reset  
      ~~~
      options.fBusyTimeout = 2.5s;
      options.fJournalMode = JournalModeType::eWAL2;
      ~~~
      and document much better choices with respect to this issue
  - WebService
    - fixed default port# used when no configuration
  - discovery
    - dont return devices with no hardware address
  - IntegrationMgr
    - refactor internals of DBAccess_ startup logic (newMgr code ) internal to IntegratedModel/Mgr.cpp
    - refactored device rollup code slighlty, to avoid race/quirk (not traditional race) on startup - due to fact that we pick which guy to roll into randomly and there is an ambiguity if there are partial overlaps.
    - https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/69: advisory cache hw addr to guid list when generating new guids for rollup devices and networks
    - better logging on bad rollup id merges; better logging/error recovery when bad load of devices from database
    - imporved startup logging about database load/read
    - Assure only one DB load for madeItToEndOfLoadDBCode (DBAccess code); 
  - Stroika v2.1.4
- html
  - small cleanups to networks details view (mostly added exteranl address)
  - use luxon in place of momentjs in a few places; and now API module/calls fills in dates as JS Date objects now (instead of strings); https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/42
  - primitive throttling of requests so if too many requests outstanding, we just drop our fetchactive on the floor
  - fixed lastSeenAt field in devices page so now sortable properly
- Documentation
  - README notes
  
----


###  1.0d18 {2022-08-12}

#### TLDR
- Use Stroika 2.1.3 (and react to many changes/new apis)
- html about page shows connection/database statistics
- BLOBS persisted
- Various fixes to rollups and discovery
- Default sort order better
- Aggregated objects now organized/labeled better by date in UI
- Docker container build improvements
- Better stability/reliability

#### Change-Details

- Todo Cleanup (moved 1/2 to using github issues)
- Backend
  - DB
    - refactor - migrated DB code to its own module, so I can use for BLOBMgr
    - update db name (version# so lose data from previous release)
    - Lots of fiddling with SQLITE BUSY EXCEPTION  - logging etc.
    - KEY FIX WAS using WAL, but also set:
      ~~~
        options.fBusyTimeout = 1ms;
        options.fJournalMode = JournalModeType::eWAL;
      ~~~
  - BLOBMgr
    - read/write from database
    - tweak error catch/report in TransformURL2LocalStorage_, and fixed regression in TranslateURL2BLOBSTORAGE code - when Updated location of API - needed to update URL we redirect to!
    - BLOB storage API (and database) makes contentType optional cuz missing from some URLs we try to cache
  - Discovery
    - network neighbor device discovery - throw away items with no network information (log still)
    - updated default generation of network name to default to geoloc city, not adapter name
    - improved SDSP/map to device types discovery code
    - do not return MyDevice in discovery code if it has no hardware address/attached network. Probably no point in 'discovering'it in that case, and it causes problems with rollup
    - support for retrying on SSDP startup failure (maybe done but need logging)
    - Added kKerberos_ = 88 to portscan (experiment)
  - IntegratedModel
    - code cleanups and bug fixes on device rollup of attached networks (often had wrong ids)
    - clarifed docs on Device Merge/Rollup and semantics on precedence. This hopefully fixes bugs with rollup not getting latest data; better merging of debug props; and static Synchronized<RolledUpDevices> sRolledUpDevices_; so hopefully no more bugs with rollup getting dups
  - WSAPI
    - sort by ePriority improved slightly so interesting items show more at the top
    - store Capturer inside WSImpl object so started automatically when app starts, and shutdown automatically before end of main
    - Added optional ids= parameter to GetDevices and GetNetworks WSAPIs
  - Misc
    - Use Stroika 2.1.3
    - use new Execution::IntervalTimer::Manager::Activator
    - use new Stroika Logger::Activator, and revisions to Logger API (more brevity and cleaner startup/shutdown)
- html
  - Lose html-react, html-vue2-vuetify2, html-vue3-vuetify3: if we need to revert to any of these technologiues, can find it in git history
  - several improvements to home page: smarter check for what netowrks to list in home page, check active, show last time seen, link on devices sub-link to restruction in devices page to that network
  - Home Page
    - minor cleanups
  - Devices
    - cleanups to device details page, truncateWithElipsis, and sort networks in device details page
    - minor cleanups to html code (better display in historical snapshots - and show fewer guids by default and hide buttosn that make no sense here)
  - Misc
    - Implemted html SortNetworks - so come in a better UI order (most recent top)
    - new GetAttachedNetworksAsNetworks() utility - used to sort networks
    - lose apparently unused icons in public/icnosna and updated favicon
    - npm update/upgrade to latest
    - minor html cleanups (FormatLocation)
    - fixed https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/36 -  sort aggregated by date and show in text of label eversince (fistr draft for devcies)
  - UI portability
    - fixed display of buttons on tablet (html) - use q-btn for Link2DetailsPage.vue
- Backend & HTML
  - use new boot/configuraiton mechanims so on app startup, we automatically fetch a json config file from the build directory (can be filled in my C++ app without recompilign); and integrate  this with the rest of the startup app configuration stuff- refactored
  - adjusted WSAPI path to include /api/v1/ at start of API code
  - fold togetehr GUI and WS connection managers into one pool. CAN be separate, but really no need and probably simpler - at least to configure - if not separate (no more port 8080, just all on port 80)
  - use database ORM operation hooks; then used that for new OperationalStatisticsMgr which provides stats for recent API call times and DB access times
    and provided UI display of these in about page
    - use new Stroika IntervalTimer code to peridocally check number of active WS tasks and added that to reprorted statistics (I think completiing new stats in about info project, except for UI)
  - New 'seen' support - replaced lastSeen (in backend, datamodel, database, and GUI) with structured 
    seen range and 'ever' alias for having these combined.
  - OS Category
    - Added majorOSCategory field to WSAPI OperatingSystem object; and used that instead of search through fullVersionedName for keywords in ComputeServiceTypeIconURL () - so more localized in C++ code (actuall stroika or OS data) categorization and just used in GUI: should allow omre cases of raspbian and ubuntu to now show with icons (testing)
- Build System etc
  - github actions switch to macos-latest
  - Fixed docker container build
    - Added optiopnal build  INCLUDE_OPENSSL and INCLUDE_HANDY_DEV_TOOLS in docker files (defaults off)

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
  - vetur.config.js file so it finds the package.config file and editing in vscode works better
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
