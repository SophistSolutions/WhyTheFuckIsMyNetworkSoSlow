# WTF design: snapshots, identity, aging, measurements, peers

**Status: DRAFT (2026-10-30; Migration added 2026-11-02)** - the direction, from design discussions; not a description
of the code. "Where the code stands" near the end says what exists today, and "Migration" how to get from there to here.

## Goal

WTF finds the networks and devices it can see and keeps their history. The direction: collect enough measurements to
tell, when a network goes bad, what is wrong and where.

## Principles

- **Record what was seen; derive everything else.** Rules - identity, aging - can then change without losing information.
- **Every object needs an identity** - networks, devices, network interfaces. Automate it as far as possible. It will
  sometimes be wrong, so users can override it.
- **WTF stops and restarts.** What it saw must survive that: that is what snapshots are for.
- **Old data is worth less than new, and is used differently.** Keep detail for recent data, summaries for old.

## Three layers

```
 1. SNAPSHOTS   what was seen, by which observer, when - no identity
        |
 2. ROLLUP      gives identity to the objects in the snapshots: which are the same thing, and its ID
        |       (automatic rules, overridden by the user's)
 3. VIEWS       what the UI and API show about each identified object: its merged properties and stats
                (computed, never stored)
```

## Example: a laptop, its router, one network

WTF runs on a laptop on home Wi-Fi. (Documentation addresses: 203.0.113.x, and MACs from 00:00:5e:00:53:xx.) Its
snapshot for one 5-minute bucket (bucket sizes are open - see Aging):

```
observer  instance I1 (the laptop), through its interface "Wi-Fi"
bucket    2026-10-31 10:00-10:05 UTC

network   range 192.168.0.0/24, gateway 192.168.0.1, gateway MAC 00:00:5e:00:53:01, external 203.0.113.7
          seen 10:00:12-10:04:42, 10 times

device    MAC 00:00:5e:00:53:22, IP 192.168.0.23, name "laptop", seen as: this host
          on network (192.168.0.0/24, gateway 192.168.0.1, gateway MAC 00:00:5e:00:53:01)
device    MAC 00:00:5e:00:53:01, IP 192.168.0.1, name "router.lan", seen by: ARP, SSDP
          on network (192.168.0.0/24, gateway 192.168.0.1, gateway MAC 00:00:5e:00:53:01)

measure   ping RTT, Wi-Fi -> 192.168.0.1 (00:00:5e:00:53:01): min 1.8 ms, avg 3.1 ms, max 9.4 ms, 60 sent, 0 lost
measure   Wi-Fi signal: avg -58 dBm, min -63 dBm
```

No IDs anywhere: a device says which network it was on by that network's attributes.

Rollup makes three objects from it:

| Object | ID | Anchored on |
|---|---|---|
| network "Home" | N1 | 192.168.0.0/24, gateway 192.168.0.1, gateway MAC 00:00:5e:00:53:01, external 203.0.113.7 |
| device "laptop" | D1 | MAC 00:00:5e:00:53:22 |
| device "router.lan" | D2 | MAC 00:00:5e:00:53:01 - also N1's gateway |

What happens next:
1. **10:07, the external-address lookup fails.** The network row has no external address - a different key, so its own
   row. Rollup: nothing known on both sides differs, and the gateway MAC matches, so it is N1.
2. **11:00, WTF restarts.** The same attributes give the same keys; the 11:00 bucket's rows are just the next rows.
   Nothing about the restart needs storing.
3. **A second instance, I2, on a wired desktop.** Its rows say observer I2, interface "Ethernet". The same network
   attributes make it N1; the laptop (MAC ...:22, seen by ARP) is D1, the router D2. When the laptop's ping to the
   router climbs and the desktop's stays flat, the fault is the laptop's Wi-Fi, not the router.
4. **Next week the ISP changes the address to 203.0.113.99.** A known difference, so a new network, N2 - rollup errs
   toward splitting. If it is the same network, the user merges N2 into N1.
5. **Two weeks on, aging** turns these 5-minute rows into hourly ones - row by row, never combining two keys, so every
   rule can still be re-run over them.

## Snapshots

- A snapshot records the attributes of something as seen, the observer (which WTF instance, through which of its
  interfaces), and when. For example:
  - a device: MAC, IP, the network it was on, host name, how it was seen (ARP, SSDP, ping, ...);
  - a network: address ranges, gateway IPs, gateway MACs, external address.
  
  Attributes not known at the time are left empty.
- **A snapshot carries no identity.** Whether two sightings are the same thing is rollup's decision, not the
  collector's. Rollup can merge later; nothing can split a merge already stored. And snapshots from several observers
  then combine without conflict (see Peers).
- **Stored by its attributes (its key), not by an ID.** A sighting with the same key in the same time bucket extends
  the same row - after a restart too. A changed attribute makes a new key; a stored row is never overwritten with
  different attributes.

## Rollup: identity

### Identity is the attributes, not a hash

WTF has always decided identity by comparing attributes, and their choice has changed over time. It did the comparison
through a hash - a fingerprint - and that was the flaw: a hash says only equal or not equal. It cannot say "unknown", so
a missing value (a failed external-address lookup) looked like a different network. Identity needs looser matching,
with wildcards, where identities that are not identical still compare equal. So the inputs to the hash, not the hash,
are the identity.

This applies wherever a fingerprint is used: networks and network interfaces. Devices already compare attributes.

### Matching

A snapshot belongs to an existing object when:
1. **no attribute known on both sides differs** - unknown matches anything; a known difference means a different object;
2. **they share a strong attribute:**
   - a network: a gateway MAC; for one with no gateway (Hyper-V, WSL, Tailscale), its address range plus its interface;
   - a device: a MAC; for a device with only randomized MACs, stable attributes instead - host name with type, OS and
     open ports, SSDP UDN (open).
3. **several objects qualify:** the one agreeing on the most attributes. Ties: open.
4. **missing values are filled in** on the object as they become known.

**The automatic rules err toward splitting**: a wrong split costs a user merge; a wrong merge is much harder to see.

### IDs are anchors

- An ID is stored with the keys it was given for. When the rules change, each new group takes the ID of the anchor
  whose keys it holds, so an ID - and the user settings stored under it - survives the change.
- Two anchors for one thing (after a rule change, or made by two peers at once): the older ID wins, the other becomes
  its alias. Every instance applies the same rule, so all converge.
- Automatic anchors are kept apart from the user's own settings.

### User overrides

- Names, notes, tags: stored by ID.
- **Merge** - "these are the same thing".
- **Split** - "these are not the same thing". Possible only because snapshots carry no identity. Whether, and how, the
  UI offers it: open.
- User rules beat automatic ones.

### Accepted risk: one router, two internet connections

A travel router or a phone hotspot keeps its LAN - address range, gateway IP and MAC - wherever it goes; only the
external address differs. While that address is unknown (a hotel's captive portal blocks the lookup), its snapshots join
the home network. That corrects itself once the lookup works - unless home never learned its own external address, when
the fill-in writes the hotel's into home's record. Accepted: requiring a known external address would hide every network
without internet.

## Aging

- **Coarsen time, never keys.** Recent buckets are fine, older ones coarse; two keys are never combined, so any rule
  can be re-run over all of history.
- Example tiers, to settle: every sample for a day or two; per-minute summaries (min, average, max, p95, loss) for about
  two weeks; hourly after that; for presence alone, eventually first and last seen.
- **Incidents keep their detail.** When a measurement crosses a threshold, the minutes around it keep full detail long
  after routine detail is summarized - bad periods are what someone looks at a month later.
- Keys not seen for months, with no user settings, are dropped.
- **Deterministic:** buckets on UTC boundaries, summaries computed from the data alone, so every peer ages a copy the
  same way.

## Measurements

Two kinds of data, handled differently:

| | Inventory | Measurements |
|---|---|---|
| What | what exists: devices, networks, interfaces, their attributes | latencies, Wi-Fi signal strength, DNS times, traffic |
| Volume | low; changes rarely | high: a sample every few seconds per series |
| Needs | identity (rollup); kept long | aging; meaning comes from lining up in time |

- A measurement series is labeled by attributes as seen ("ping RTT, from interface I to 192.168.0.1, MAC m") - like a
  snapshot, no identity. Rollup decides which device or network a series belongs to, so merges and rule changes regroup
  measurements too.
- **Probes follow the path out of each network**, so a fault can be located:

  | Hop | Probe | If only this one is bad |
  |---|---|---|
  | Link | Wi-Fi signal strength, link rate | Wi-Fi: distance, interference |
  | Gateway | ping the router | LAN or router |
  | ISP | ping the first hop past the router | the ISP's access line |
  | Internet | ping a few well-known hosts | upstream, routing |
  | DNS | time lookups against each configured server | DNS, not the network |

- Traffic: per-interface byte and packet counters are cheap (the OS keeps them). Per-device or per-connection traffic
  needs packet capture or the router's help (SNMP, NetFlow) - a much larger step. Kept short-term, or as high-level
  summaries.

## Peers

- **N instances, decentralized**: any instance collects any peer's snapshots and runs its own rollup over all of them.
- **Finding peers:** UPnP (SSDP - multicast, so the same LAN only), and the UI ("look here"). Peers tell each other
  every peer they know, so a peer on another network becomes known through the others.
- **Trust:** everyone, for now.
- Each snapshot carries its observer and a sequence number, so "everything from observer O after N" can be fetched from
  any peer, and fetching one snapshot twice is harmless.
- Anchors and user settings are shared too - small - and converge by the older-ID-wins rule.
- Identity across perspectives is where rollup matters most: one phone seen by two instances may share no attribute (a
  different random MAC per Wi-Fi network) - hence wildcard matching and stable non-MAC attributes.
- Two vantage points separate "the network is bad" from "this host's Wi-Fi is bad".

## Where the code stands (2026-10-30)

| Design | Today |
|---|---|
| Snapshots carry no identity | Discovery gives each object a per-run ID before storing: devices sharing a MAC *or an IP* are one (`FindMatchingDevice_`), networks with the same gateway IPs and address ranges are one (`genProperNetworkIDAndSeen`) |
| Stored by key, never overwritten | One stored record per Discovery ID, written every 30 s: networks and interfaces replaced (`AddOrUpdate`), devices merged (`Device::Merge`) |
| Rollup recomputed from snapshots | Yes - `RolledUpDevices`, `RolledUpNetworks`, `RolledUpNetworkInterfaces` are rebuilt at each start from the stored records plus Discovery's current ones |
| Identity is the attributes | Networks and interfaces: a hash (`GenerateFingerprintFromProperties`). An interface's ID is its hash |
| IDs are anchors | Automatic records in the user-settings tables (`FindOrCreateDeviceID`: the exact MAC set; `FindOrCreateNetworkID`: the fingerprint) |
| User overrides | User settings - names, notes, tags, merges; network merges are lists of fingerprints. No splits |
| Aging | None: every run's records are kept. `PruneBadNetworks_` removes network settings with nothing rolled into them |
| Measurements | Interface link speeds and "seen" ranges (one per way of seeing); no latency or DNS data |
| Peers | None. Discovery searches for and listens to SSDP; WTF does not advertise itself |

## Migration

Today's behavior first, rebuilt on this design; new behavior after, each piece on its own. Every step is one or more
commits that each work alone.

1. **Identity by attributes; IDs as anchors.** Networks and interfaces stop using hashes; the automatic anchors move out
   of the user-settings tables; network merges are stated by attributes or ID, and the API's PATCH operations and help
   change with them. The automatic rule that folds a device's host-internal networks into it carries over. Needs the
   first open question settled: a network whose gateway never answers ARP must still get an ID.
2. **Snapshots with no identity.** Discovery stops merging devices on a shared IP alone, and stores rows keyed by
   attributes and observer: one row per key, per observer, per run. (One row per key for all time would lose the gaps
   between runs: a device away for two weeks would look seen throughout.) An adapter turns the rows into the per-key
   objects the rollup reads today, so most of the rollup code stays. Three commits - write the rows beside today's
   records, switch the rollup to read them, stop writing the old records - with old-versus-new comparison runs on two
   machines between them. Most of the risk is here: every discoverer in `Discovery/Devices.cpp` stores through
   `StoreMerged_`.
3. **The UI's "Aggregates" becomes a history list**, on the device, network and interface detail pages.
4. *Optional:* **carry names, notes and tags across the database reset** - devices by MAC, networks by fingerprints
   recomputed from their attributes.

The stored format changes, so the database starts fresh - the reset TODO.md already plans.

**Effort** (estimated 2026-10-31, in an AI agent's working days: about 8 hours of its own work, builds and test runs
included, waits for review not). Review rounds set the calendar more than the work does.

| Step | Agent days | Review rounds |
|---|---|---|
| 1. Identity by attributes | ~1 | 2-3 |
| 2. Snapshots with no identity | 2-3 | 4-5 |
| 3. UI history list | ~0.5 | 1 |
| 4. Carry user settings across (optional) | ~0.3 | 1 |
| **Today's behavior** | **3.5-4.5** | **7-10** |

| New behavior, after | Agent days |
|---|---|
| Aging: time buckets, incidents kept in detail | 1-1.5 |
| Measurements: path probes, DNS timing, Wi-Fi signal, time-series storage, API | 2-3 |
| Measurement charts in the UI | 1-2 |
| Peers: advertising over SSDP, a peer API, exchanging snapshots and anchors, a peers page | 3-5 |
| Merge and split controls in the UI | 1-2 |

**Lost:**
- Today's stored data - except names, notes and tags, with step 4.
- Opening one run's record. Today "Aggregates" links to each run's whole record (`/#/device/<id>`); it becomes a list of
  history rows - observer, attributes, when, and what was seen with it. The same information, organized by attributes
  rather than by run.
- The API's lookup of a per-run record by ID, and the `idIsPersistent` field. Only the UI's aggregate links use them.
- Some automatic merges that are right today: a phone that changes its random MAC but keeps its IP during one run is one
  device today, because Discovery merges on the IP. It becomes two until the stable-attribute rule exists (Matching).
- Merging networks by fingerprint through the API (`PATCH` of `/userOverrides/aggregateFingerprints`, in the API help's
  curl examples): scripts using it break.

**Kept:** every list, page, filter and sort; editing names, notes and tags; the device, network and interface details;
"seen" per way of seeing; ports, OS, icons and SSDP data; the API's shapes, but for `idIsPersistent`.

## Open questions

- **A network whose gateway MAC never arrives** (its gateway never answers ARP) has no strong attribute, so it would
  never get an ID. Proposed: after a few minutes, use gateway IP, address range and external address together. Related:
  wait a few minutes for the external-address lookup before anchoring a new network?
- **Ties:** a snapshot matching several objects equally (a hotspot seen with several carrier addresses, then with none)
  must not make a new object each time.
- **Devices with only randomized MACs:** which stable attributes, and how strong.
- **Aging:** bucket sizes and retention; what a device's history should show a month or a year later.
- **Storage:** measurements as time series (SQLite tables, say) beside today's document collections.
- **UI:** merges, splits.
- **Peer protocol:** transport and API.
