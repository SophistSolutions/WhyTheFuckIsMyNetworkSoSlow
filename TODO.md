# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: long-range ideas
are in `TODO.txt` and https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues. Prune entries as they are
done.

## Open

- **Windows: move from VS2k22 to VS2k26** (LGP 2026-10-21): DockerBuildContainers has only Windows-VS2k22 (on Stroika's
  `stroika-buildvm-windows-msys-vs2k22:v3`), and CI builds Windows only in it. Stroika publishes
  `stroika-buildvm-windows-msys-vs2k26` (and `-cygwin-vs2k26`). Add Windows-VS2k26 - Dockerfile, Makefile target, the
  build-Dev-Docker-Containers matrix - and once the image is published, move build-N-test-Matrix.json's Windows entries
  to it; then retire VS2k22 (WTF is an app, so it targets the newest Visual Studio only - see AGENTS.md). Windows
  images build only on medusa-windows-dev, and only at night until Stroika's 3.0d25 release is done. (Goes with the VS
  2026 solution, below.)

- **Replace the WTF running on medusa** (2026-10-21) with a current build: LGP thinks the one there is broken - perhaps
  badly - probably by a Stroika WebServer bug fixed around 2026-09 (one candidate: the ConnectionManager teardown hang,
  https://github.com/SophistSolutions/Stroika/issues/1165, closed 2026-09-10). There now (2026-10-21): 1.0d20 on
  Stroika 3.0d23, installed from its .deb (systemd), `WebServerPort` 8080; its API answers normally (up 7 days, 150 MB),
  and its database holds 6180 devices. Still to find out: what is broken, and whether that database is to be kept. Then
  build Release and its .deb in WTF-Dev-2604 (Debug is built and run-checked there - `make run-test-container`), check
  it with `make test-deb-install`, and install it on medusa.

- **SSDP discovery, `RecieveSSDPAdvertisement_`** (Discovery/Devices.cpp:929-1085), the callback of both the SSDP
  Listener and the Search, runs on Stroika's shared SSDP thread - which, since Stroika 3.0d25, a network change and
  shutdown (`Stop ()`, destruction) wait for. It holds that thread far too long (protagoras traces, 2026-10-21):
  - Its lock-retry loop (Devices.cpp:999-1083) has no cap - each round waits up to 5 s to upgrade the lock, then sleeps
    1 s, 2 s, ... - so near startup, callbacks took 24-41 s.
  - The description GET has a 5 s timeout, but on Windows Stroika's WinHTTP backend applies it only to reading the body
    (https://github.com/SophistSolutions/Stroika/issues/1246). There, a LOCATION that never accepts the connection takes
    Windows' 21 s TCP give-up: Windows' own UPnP host advertises `http://[fe80::...]:2869/...` on the Tailscale
    interface, whose (Public) firewall profile drops it - and it is refetched on every NOTIFY, so the thread was busy
    21 s of every 22. The DNS lookup of a LOCATION host name has no timeout at all.
  - It drops every ssdp:byebye: a byebye has no LOCATION, so `locAddrs` is empty (a WeakAssert in Debug) and the merge,
    `fAlive` included, is skipped - https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/93.
  - Fix for all: Stroika 3.0d25's `SSDP::Client::CachingListener` (state per device, one call per change, removals
    included), with the description fetch and the merge on a thread of WTF's own - as Stroika's SSDPClient sample does
    (its fetch has no timeout; WTF's keeps one) - and not refetching soon a description that just failed.

- Discovery/Devices.cpp:660 tests `qDefaultTracingOn`, which Stroika never defines for readers, so that block (lock
  debugging, `qLOCK_DEBUGGING_`) has never compiled.

- Deprecated Stroika names WTF still uses (they go away at Stroika 3.0a1): `qDebug` ->
  `qStroika_Foundation_Debug_AssertionsChecked` (41 uses, 13 files); `qHasFeature_*` -> `qStroika_HasComponent_*`
  (WebServices/WSImpl.cpp, 8 uses; deprecated since Stroika 3.0d11).

- Before Stroika 3.0a1: WTF re-locks `Synchronized` objects it already holds (IntegratedModel/Private_/DBAccess.cpp:170-181
  and 190-201, Common/BLOBMgr.cpp:184-185). Fine with today's recursive default; a deadlock if
  https://github.com/SophistSolutions/Stroika/issues/1206 makes the default non-recursive.

- Visual Studio: `Workspaces/VisualStudio.net-2019` refers to Stroika's VS2019 projects, which no longer exist, and there
  is no VS2026 solution (Skel now makes 2022 and 2026 ones). Add the 2026 one, drop 2019 - and probably 2022 too, as
  WTF targets the newest Visual Studio only.

## Watching

Not work items: checks to run now and then, raised only when one finds something.
