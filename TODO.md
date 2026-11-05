# TODO

**Note:** This file is for **short-term** TODOs only - a scratch list to hand context between work sessions and machines,
and to brief agents (human or AI) picking up a task. It is not a roadmap, issue tracker, or design doc: long-range ideas
are in `TODO.txt` and https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues. Prune entries as they are
done.

## Open

- **Before the 1.0d21 release: the Interface.cpp:961 WeakAssert on Windows** (LGP 2026-11-05). Stroika's
  `GetInterfaces_Windows_` (IO/Network/Interface.cpp:960-961) asserts that a Wi-Fi interface's transmit and receive
  speeds from GetAdaptersAddresses equal the WLAN API's. The receive one failed in 8 of 30 Debug traces on protagoras
  (10/24-10/29, 1-6 times each, at startup and later - 606 s, 753 s), not in the 15-minute run on 11/05. Likely: a Wi-Fi
  link's rate changes from moment to moment and the two are read at different times, so the assert is wrong, not the
  data - check, and if so a Stroika issue (drop the asserts, or say which source wins).

- **Device rollup: survey and analyze it before fixing** (LGP 2026-10-22: "That logic is crap" - medusa's UI lists over
  1000 devices, and counting). medusa's `/api/v1/devices` on 2026-10-22 (1.0d21x, on its kept database): 6180 devices -
  - **First (LGP 2026-10-23): the 5734 empty records.** Each is 173 bytes of JSON - an ID and a user setting holding
    one MAC (`userOverrides.aggregateDeviceHardwareAddresses`); no network, no ports, never seen. Between them they hold
    only 24 distinct MACs (21 randomized): 26:a2:62:0b:df:df and e2:dc:30:1b:25:23 have 1468 records each,
    92:e7:c9:41:39:66 1460, four more 305-372; 17 of the 24 are also on a real device (counted 2026-10-23 - still 6180
    devices). The cause, fixed 2026-10-29 (`FindOrCreateDeviceID`): the first rollups at startup run in parallel (#23's
    workaround), and each stored its own new ID for a MAC new to it - 6 records for one MAC in one protagoras run. The
    records already stored get no cleanup code (LGP 2026-10-29): a new default database, `db-localDBDir-v2`, also for an
    AppSettings naming 1.0d20's `-v1` (2026-11-05); a database named by hand (protagoras' `db-v19.db`, medusa's
    `db-v20`) is kept. Networks had the same bug (`FindOrCreateNetworkID`, 2026-10-29). One whose MAC is no
    longer seen is still listed (#90).
  - 176 docker containers (docker0 133, the second bridge 43): a new random MAC per container start, so a new device.
  - 274 on the LAN: 83 with a global MAC (83 distinct - fine); 191 with only randomized ("locally administered") MACs -
    phones, laptops, watches with private Wi-Fi addresses: "Mac.lan" is 8 devices, "iPhone.lan" 5, "Watch.lan" 4.
  - The cause: `ShouldRollup_` (RolledUpDevices.cpp:282-312) identifies a device by its MAC alone (by IP when it has
    none).
  - Ideas, biggest payoff first: (1) list no device that has no data, expire the automatic records (those holding only
    the MAC) not seen for N days - 6180 -> about 450; (2) for a randomized MAC, match
    on what is stable - host name with type, OS and open ports, the SSDP UDN, a new MAC on the IP a same-named device
    just left - and let the user merge and split (#59); (3) fold devices on a host-internal network (docker0) into the
    host, once Stroika #1248 says which networks those are (as #94); (4) by default, show the devices seen recently
    (#57, #61). Related: #58 (design doc for IDs), #67, #53.
  - **The plan (2026-11-02): Documentation/Design.md**, its Migration section - its review postponed till after the
    next release (LGP 2026-11-05: "complicated, and important"). Decided 2026-11-03, for Design.md's Matching: a
    network's strong attribute by link type - tunnel, PPP, cellular: address range + gateway IP + interface at once;
    Ethernet and Wi-Fi: the gateway MAC, falling back after a few minutes to gateway IP + range + external address, a
    network so identified marked as such in the API and UI. Proposed: ties go to the oldest anchor; label each open
    question with the step that needs it settled.

- **SSDP "last seen"**: once https://github.com/SophistSolutions/Stroika/issues/1256 gives `SSDP::Advertisement` the
  time it was heard, `SSDPDeviceDiscoverer_::Refresh_` extends seen.UDP to when each advertisement was last heard, not to
  now - today a device that leaves without a byebye looks seen until its max-age runs out (often 30 minutes).

- A Debug .deb build on 2026-10-22 logged "dpkg-deb: error: tar subprocess was killed by signal (Broken pipe)" in
  DEB-Installer-Build.Out, though make succeeded and the .deb was complete; not reproduced (three clean builds 10/23).

## Watching

Not work items: checks to run now and then, raised only when one finds something.
