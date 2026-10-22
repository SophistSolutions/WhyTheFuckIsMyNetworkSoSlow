# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: long-range ideas
are in `TODO.txt` and https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues. Prune entries as they are
done.

## Open

- **Device rollup: survey and analyze it before fixing** (LGP 2026-10-22: "That logic is crap" - medusa's UI lists over
  1000 devices, and counting). medusa's `/api/v1/devices` on 2026-10-22 (1.0d21x, on its kept database): 6180 devices -
  - 5734 empty (no network, no MAC, never seen). `MergeInNew_` (IntegratedModel/Private_/RolledUpDevices.cpp:234-258)
    gives each new rollup a new ID, and `GenNewDeviceID` (Private_/DBAccess.cpp:128-140) stores it as a user setting
    holding its MAC, so the ID survives restarts. Nothing removes one, and one whose MAC is no longer seen is still
    listed (#90). Some MACs have two (26:a2:62:0b:df:df).
  - 176 docker containers (docker0 133, the second bridge 43): a new random MAC per container start, so a new device.
  - 274 on the LAN: 83 with a global MAC (83 distinct - fine); 191 with only randomized ("locally administered") MACs -
    phones, laptops, watches with private Wi-Fi addresses: "Mac.lan" is 8 devices, "iPhone.lan" 5, "Watch.lan" 4.
  - The cause: `ShouldRollup_` (RolledUpDevices.cpp:282-312) identifies a device by its MAC alone (by IP when it has
    none). And it tests `fAggregatesIrreversibly` twice (:284-285) - the first was meant to be `fAggregatesReversibly`.
  - Ideas, biggest payoff first: (1) list no device that has no data, expire the automatic records (those holding only
    the MAC) not seen for N days, and find the duplicate-record bug - 6180 -> about 450; (2) for a randomized MAC, match
    on what is stable - host name with type, OS and open ports, the SSDP UDN, a new MAC on the IP a same-named device
    just left - and let the user merge and split (#59); (3) fold devices on a host-internal network (docker0) into the
    host, once Stroika #1248 says which networks those are (as #94); (4) by default, show the devices seen recently
    (#57, #61). Related: #58 (design doc for IDs), #67, #53.

- **The UI fires one request per device, all at once** (2026-10-22: Chrome on Linux showed medusa's WTF, then went
  blank - ERR_INSUFFICIENT_RESOURCES): `fetchActiveDevices` (html/src/stores/Net-State-store.ts:165-211), on every
  HomePage and DevicesPage poll, fetches the device IDs, then each device separately - 6180 at once on medusa (a fresh
  headless Chrome logged 12874 failed fetches). Probably what looked broken before the upgrade too (1.0d20 listed the
  same 6180). Fix: one `/api/v1/devices?recurse=true` (0.2 s for all 6180), or a small concurrency cap; the rollup
  entry above would also cut the count to about 450.

- HTTP caching (checked 2026-10-22 - not the cause of the blank page): hashed /assets/ files are immutable - right; but
  index.html is `public, max-age=300` - make it `no-cache` (304s work), so an upgrade is seen at once. A missing file is
  answered with index.html (200, text/html) - under /assets/ marked immutable for 68 years, as Stroika's
  FileSystemRequestHandler picks the cache rule by the URL, not by the file it serves (a Stroika bug) - and unknown
  /api/ paths get index.html too. That fallback (WebServices/WebServer.cpp:165-168) is "for createWebHistory router
  mode", but the UI uses hash routing (html/quasar.config.ts:62): drop it, and a missing file is a 404. Single-object
  API GETs are cached 10 s (active) or 24 hours (historical - IntegratedModel/Mgr.cpp:154-155).

- For Stroika (pass on to its session; found 2026-10-22): FileSystemRequestHandler's fallback file takes the cache rule
  of the URL asked for, not of the file it serves (FileSystemRequestHandler.cpp:66-86 - above); and its Samples' unit
  files (Samples/HTMLUI/Installers, Samples/Service/Installers) say `PIDFile=/var/run/...`, which systemd warns about
  on every reload (WTF's moved to /run).

- **Windows: move from VS2k22 to VS2k26** (LGP 2026-10-21): DockerBuildContainers has only Windows-VS2k22 (on Stroika's
  `stroika-buildvm-windows-msys-vs2k22:v3`), and CI builds Windows only in it. Stroika publishes
  `stroika-buildvm-windows-msys-vs2k26` (and `-cygwin-vs2k26`). Add Windows-VS2k26 - Dockerfile, Makefile target, the
  build-Dev-Docker-Containers matrix - and once the image is published, move build-N-test-Matrix.json's Windows entries
  to it; then retire VS2k22 (WTF is an app, so it targets the newest Visual Studio only - see AGENTS.md). Windows
  images build only on medusa-windows-dev, and only at night until Stroika's 3.0d25 release is done. (Goes with the VS
  2026 solution, below.)

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
    `fAlive` included, is skipped - https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/93. (A
    Debug run on medusa logged it 2224 times in 14 hours.)
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

- **A Debug build's .deb loads every core for over a minute** (2026-10-22, medusa's load 24 -> 51): dpkg-deb compresses
  the 2 GB unstripped (ASan) Debug binary to 216 MB on 33 threads, about 80 s. Cap it (`--threads-max`, in
  Installers/Makefile), or make no .deb for Debug. That run's DEB-Installer-Build.Out also said "dpkg-deb: error: tar
  subprocess was killed by signal (Broken pipe)", though make succeeded and the .deb was complete; not reproduced.

- Linux: the top Makefile's default-configurations try only g++, g++-12..14 and armhf g++-12..14 - no g++-15/16
  (Ubuntu 26.04 has g++-15), and no raspberrypi with armhf g++-15. build-N-test-Matrix.json has no Ubuntu 26.04 entry
  yet (add one once build-Dev-Docker-Containers has published the image).

- Every Debug start logs a WeakAssert: WebServices/Model.cpp:305-332 maps `NetworkInterface::fBindings` twice on purpose
  (`boundAddressRanges`, `boundAddresses`), which Stroika's duplicate-field check flags. A fix with no JSON change:
  write and read those two keys in the class's `fAfterFrom`/`fAfterTo` options, as the fingerprint does
  (Model.cpp:343-349).

- Small bugs found reading the code (2026-10-20):
  - WebServices/WebServer.cpp:483 records `fNumberOfActiveConnections` with `RecordActiveRunningTasksCount`, mixing two
    statistics; `RecordProcessingConnectionCount`, never called, looks intended.
  - Discovery/Networks.cpp `genCIDRsFromBindings`: the loop dropping subsumed CIDRs never fires (a CIDR with more
    significant bits cannot contain one with fewer) - and, made to fire, it removes from the Set it is iterating.
  - html/src/pages/AboutPage.vue imports `pretty-bytes`, which html/package.json does not list.
  - Every Backend Makefile's `SrcDir` names `BackendApp/...`, and two name the wrong module - unused, but misleading.
  - .vscode/launch.json: the natvis path says `ThirdPartyComponets`; the lldb entry runs `phred123/phred123`.
  - README.md puts the API on port 8080 (UI and API share one port, default 80).

## Watching

Not work items: checks to run now and then, raised only when one finds something.
