# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: long-range ideas
are in `TODO.txt` and https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues. Prune entries as they are
done.

## Open

- **Finish the upgrade to Stroika 3.0d25** (2026-10-20): the submodule (the 3.0d25 draft), the `fInterfaceID` rename and
  the untracked VS `.props` links are committed. The rest of its upgrade notes ask this of WTF:
  - Discovery/Networks.cpp:164 copied the `IntervalTimer::Adder` example's `.release ()`, which leaks the timer:
    `.reset ()`.
  - `kStroika_Version_FullVersion` -> `qStroika_Version_FullVersion`: WebServices/WSImpl.cpp:118.
  - `qStroika_Foundation_Common_Platform_*` -> `qStroika_Platform_*`: 9 uses in Main.cpp, Common/AppConfiguration.h,
    Common/AppConfiguration.cpp, Common/DB.cpp.
  - `Installers/Makefile`: `light.exe` -> `$(StroikaRoot)Build/Scripts/RunWIXLight`.
  - On Linux the interface listing now has IPv6 addresses, and interfaces with no IPv4: look at which Networks appear.
  - SSDP clients now listen and search on every network and follow network changes: `SSDPDeviceDiscoverer_`'s timer that
    rebuilds a missing Listener or Search (Discovery/Devices.cpp:860-868) has nothing left to do.
  - A device description's relative URLs resolve against its `fURLBase`, when it has one: Discovery/Devices.cpp:973
    and 976 resolve against the LOCATION.

- **Linux builds, in docker** (2026-10-21; LGP wants to talk it over before anything builds on a remote machine). So
  far WTF is built and checked only on protagoras (MSVC). To decide and set up:
  - Where: a stroika-dev container on medusa (stroika-dev-2604 has the newest g++), in a clone of WTF's own under
    `/Sandbox/claude/`, synced from the Windows workspace by `git diff | ssh ... git apply`; or WTF's own build images
    (`DockerBuildContainers/`: Ubuntu 22.04 and 24.04 - none for 26.04 yet).
  - Configure inside the container (`make default-configurations`); build Debug, then Release and the .deb.
  - Running it: the stroika-dev containers are on docker's bridge network (only ssh is published), so discovery there
    sees no LAN - a run needs `--network host`, and raw sockets for ping and ARP. Port 80 and
    `/var/opt/WhyTheFuckIsMyNetworkSoSlow/` need root, unless `WebServerPort` is above 1024.
  - Then the Linux-only 3.0d25 check above: IPv6 addresses, and interfaces with no IPv4, in the interface listing.

- **Replace the WTF running on medusa** (2026-10-21) with a current build: LGP thinks the one there is broken - perhaps
  badly - probably by a Stroika WebServer bug fixed around 2026-09 (one candidate: the ConnectionManager teardown hang,
  https://github.com/SophistSolutions/Stroika/issues/1165, closed 2026-09-10). Needs the Linux build above (its .deb).
  First find out: how it runs there (the .deb's systemd unit, or docker), its version (`/api/v1/about`), what is
  broken, and whether its database is to be kept.

- **SSDP discovery, `RecieveSSDPAdvertisement_`** (Discovery/Devices.cpp:931-1077), the callback of both the SSDP
  Listener and the Search:
  - It does a DNS lookup and a GET of the device description, with no timeout, on Stroika's shared SSDP thread. Since
    Stroika 3.0d25 a network change waits for it - minutes, for a device that just went away - and so does shutdown
    (`Stop ()` and destruction wait for a running callback).
  - It drops every ssdp:byebye: a byebye has no LOCATION, so `locAddrs` is empty (a WeakAssert in Debug) and the merge,
    `fAlive` included, is skipped - https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/93.
  - Fix for both: Stroika 3.0d25's `SSDP::Client::CachingListener` (state per device, one call per change, removals
    included), with descriptions fetched on a thread of WTF's own, with a timeout - as Stroika's SSDPClient sample does.
    A smaller first step: a timeout on the GET.

- Discovery/Devices.cpp:660 tests `qDefaultTracingOn`, which Stroika never defines for readers, so that block (lock
  debugging, `qLOCK_DEBUGGING_`) has never compiled.

- Deprecated Stroika names WTF still uses (they go away at Stroika 3.0a1): `qDebug` ->
  `qStroika_Foundation_Debug_AssertionsChecked` (41 uses, 13 files); `qHasFeature_*` -> `qStroika_HasComponent_*`
  (WebServices/WSImpl.cpp, 8 uses; deprecated since Stroika 3.0d11).

- Before Stroika 3.0a1: WTF re-locks `Synchronized` objects it already holds (IntegratedModel/Private_/DBAccess.cpp:170-181
  and 190-201, Common/BLOBMgr.cpp:184-185). Fine with today's recursive default; a deadlock if
  https://github.com/SophistSolutions/Stroika/issues/1206 makes the default non-recursive.

- Visual Studio: `Workspaces/VisualStudio.net-2019` refers to Stroika's VS2019 projects, which no longer exist, and there
  is no VS2026 solution (Skel now makes 2022 and 2026 ones).

## Watching

Not work items: checks to run now and then, raised only when one finds something.
