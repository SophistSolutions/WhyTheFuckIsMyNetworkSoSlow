# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: long-range ideas
are in `TODO.txt` and https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues. Prune entries as they are
done.

## Open

- **Device rollup: survey and analyze it before fixing** (LGP 2026-10-22: "That logic is crap" - medusa's UI lists over
  1000 devices, and counting). medusa's `/api/v1/devices` on 2026-10-22 (1.0d21x, on its kept database): 6180 devices -
  - **First (LGP 2026-10-23): the 5734 empty records.** Each is 173 bytes of JSON - an ID and a user setting holding
    one MAC (`userOverrides.aggregateDeviceHardwareAddresses`); no network, no ports, never seen. Between them they hold
    only 24 distinct MACs (21 randomized): 26:a2:62:0b:df:df and e2:dc:30:1b:25:23 have 1468 records each,
    92:e7:c9:41:39:66 1460, four more 305-372; 17 of the 24 are also on a real device (counted 2026-10-23 - still 6180
    devices). So a known MAC keeps getting a new ID: `MergeInNew_` (IntegratedModel/Private_/RolledUpDevices.cpp:234-258)
    gives each new rollup a new ID, and `GenNewDeviceID` (Private_/DBAccess.cpp:128-140) stores it as a user setting
    holding its MAC, so the ID survives restarts - and nothing finds the one already stored for that MAC, or removes
    one. One whose MAC is no longer seen is still listed (#90).
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

- For Stroika (pass on to its session; found 2026-10-22): its Samples' unit files (Samples/HTMLUI/Installers,
  Samples/Service/Installers) say `PIDFile=/var/run/...`, which systemd warns about on every reload (WTF's moved to
  /run). And (2026-10-23) Build/Scripts/FormatCode expands its `EXCLUDES` unquoted, so bash globs `*/node_modules/*`
  against the current directory: run from WTF's root it matches html/node_modules/... and find fails ("paths must
  precede expression"); `-name $filePattern` is unquoted the same way. FileSystemRequestHandler's fallback cache rule,
  and the HTMLUI sample's caching and routing docs: https://github.com/SophistSolutions/Stroika/issues/1253.

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

- A Debug .deb build on 2026-10-22 logged "dpkg-deb: error: tar subprocess was killed by signal (Broken pipe)" in
  DEB-Installer-Build.Out, though make succeeded and the .deb was complete; not reproduced (three clean builds 10/23).

- Small bugs found reading the code (2026-10-20):
  - html/src/pages/AboutPage.vue imports `pretty-bytes`, which html/package.json does not list.
  - Every Backend Makefile's `SrcDir` names `BackendApp/...`, and two name the wrong module - unused, but misleading.
  - .vscode/launch.json: the lldb entry runs `phred123/phred123`.
  - README.md puts the API on port 8080 (UI and API share one port, default 80).

## Watching

Not work items: checks to run now and then, raised only when one finds something.
