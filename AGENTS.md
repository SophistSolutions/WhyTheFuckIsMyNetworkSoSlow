# AGENTS.md

This file provides guidance to agents (and people) working in this repository.

@ThirdPartyComponents/Stroika/StroikaRoot/AGENTS.md

WTF is an application built on Stroika (scaffolded with Stroika's `Skel`). Stroika's AGENTS.md, imported above, applies
here - agents that do not follow `@` imports: read `ThirdPartyComponents/Stroika/StroikaRoot/AGENTS.md` first. Its build
system, C++ conventions, Commits, Issues and "Working with LGP" sections are WTF's too. These parts of it do not apply:
the regression tests (`Tests/NN`, `run-tests` - WTF has no automated tests), its CI matrix and supported-platforms
tables, its release milestones (`3.0dNN`), `UPGRADE NOTE:`s in commit messages (nothing is built on WTF), and its
"Downstream projects" list. Where the two files disagree, this one wins.

## What this is

"Why The Fuck Is My Network So Slow" (WTF): a service that finds the networks and devices it can see, keeps their
history in a local database, and serves a web UI and a JSON API about them (`http://<host>/`, `http://<host>/api`). It
installs as a Windows service or a systemd unit. Version in `VERSION`; branches `v1-Dev` (work) and `v1-Release`. The
README is out of date in places (it puts the API on port 8080) - trust the code.

**Platforms** (LGP, 2026-10-21): on Linux, keep building for the older Ubuntu LTS releases as well as the newest - that
is platform support, the reach of the .deb. On Windows, target the newest Visual Studio only: WTF is an application, not
a library, so once it builds with VS2k26, VS2k22 goes.

## Layout

- `Backend/Sources/` - the service: one executable, `WhyTheFuckIsMyNetworkSoSlow-Backend`, in namespace
  `WhyTheFuckIsMyNetworkSoSlow::BackendApp`. Each module is a singleton (`sThe`) made live by an `Activator`;
  `Service.cpp`'s `MainLoop` constructs them in dependency order, and their destruction shuts them down in reverse.
  - `Main.cpp`, `Service.cpp` - command line, logging, the service framework (`Frameworks::Service`), and on Windows
    the firewall rule SSDP listening needs.
  - `Common/` - `AppConfiguration` (the `AppSettings` options file), `DB` (the one database connection: a LocalDocumentDB
    JSON directory or file, or SQLite), `BLOBMgr` (device icons and other fetched files, kept in the database), the
    MAC-prefix and GEO/ISP lookups, `OperationalStatistics`.
  - `Discovery/` - what is on the network now: `NetworkInterfaces`, `Networks` (made from the interfaces), `Devices`
    (SSDP, this machine, ARP neighbors, a random walk of the subnet, rescans of known devices), `PortScanner`. IDs here
    last only as long as the process.
  - `IntegratedModel/` - the rollup layer: merges discovery with the stored history into the Devices, Networks and
    NetworkInterfaces the API returns, with IDs that persist, steered by user settings (names, tags, notes, aggregation
    rules). `Private_/DBAccess` does all database I/O; a background thread stores snapshots every 30 seconds.
  - `WebServices/` - `Model.h` (the data model), `IWSAPI`/`WSImpl` (the API), `WebServer` (routes: `/api/v1/...`,
    `/config.json`, and the html files).
- `html/` - the web UI: Vue 3, Quasar (app-vite), pinia, TypeScript. `src/proxy/API.ts` calls the API; `src/models/`
  mirrors `Model.h`.
- `Installers/` - .deb and .rpm (each with a systemd unit), and the WiX .msi.
- `ThirdPartyComponents/` - Stroika (git submodule, at `StroikaRoot`) and `EthernetMACAddressOUIPrefixes` (nmap's
  mac-prefixes file, downloaded by the build).
- `DockerBuildContainers/` - WTF's build images (built on Stroika's), published as
  `sophistsolutionsinc/whythefuckismynetworksoslow-*:v1` by the `build-Dev-Docker-Containers` workflow (run by hand).
- `.github/workflows/build-N-test.yml` - CI on every push: builds on Ubuntu 22.04 and 24.04, macOS and Windows, and
  keeps the .deb and .msi installers; it runs nothing. As in Stroika, `v1-Dev` runs only the matrix entries marked
  `"always"`.
- `Workspaces/` - the VS Code workspace and Visual Studio solutions (Visual Studio is for debugging; make builds).
- `Release-Notes.md` - WTF's release notes. `TODO.txt` - LGP's long-range notes and ideas. `TODO.md` - short-term.

## Model.h is a contract

`WebServices/Model.h`'s types are, all at once, the API's JSON, the documents stored in the database, and what the
rollup fingerprints hash (`GenerateFingerprintFromProperties`). So renaming a mapper's field name (`"platformInterfaceID"`,
`"aggregatesReversibly"`, ...) breaks the UI and every existing database; and changing what a fingerprint hashes changes
rollup IDs, orphaning the user settings stored under the old ones. Agree a migration first.

## Build

Stroika's build section applies. The WTF-specific parts:
- One build root: the first `make` (`make build-root`) points StroikaRoot's `Builds`, `ConfigurationFiles` and
  `IntermediateFiles` at WTF's own, so Stroika and WTF build into the same `Builds/<CONFIG>/`.
- `make default-configurations` makes WTF's list, from the top-level `Makefile`: Debug and Release everywhere;
  Release-Logging (Release with a trace file) on Linux and Windows; x86/x86_64 variants and Profile on Windows;
  raspberrypi-* where the cross compiler is installed. Then `make CONFIGURATION=Debug all -j8`. `make all` alone builds
  every default configuration - slow.
- For one configuration, `make all` builds the Stroika libraries (with `QUICK_BUILD=1`, so libraries already there are
  not rebuilt), the OUI file, `html` (`npm i` and `npm run build`, logged to
  `IntermediateFiles/<CONFIG>/WhyTheFuckIsMyNetworkSoSlow/html/`), `Backend`, and whatever installers the host can make.
- Output: `Builds/<CONFIG>/WhyTheFuckIsMyNetworkSoSlow/` - the executable, `html/`, `data/OSI-MAC-PREFIXES.txt`, the
  installers.
- **A header edit rebuilds nothing, in WTF as in Stroika.** After editing a WTF `.h`/`.inl`, delete WTF's objects
  (`rm -rf IntermediateFiles/<CONFIG>/WhyTheFuckIsMyNetworkSoSlow-Backend`) before trusting a build. (A top-level
  `make CONFIGURATION=X clean` also redoes the html build and the OUI download.) After moving the Stroika submodule, see
  "Upgrading Stroika".
- Formatting: `make format-code` (Backend, with Stroika's clang-format rules); in `html/`, `npm run lint` and
  `npm run format`.
- `html/.env` is local (untracked), and the build copies it in: a `VUE_APP_ROOT_API` there is baked into the built UI,
  and overrides what the backend's `/config.json` says. Keep it unset for a build that is installed anywhere.

## Run and check

- `Builds/<CONFIG>/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Backend --run-directly` runs it in the
  foreground (`--help` lists the service commands). UI and API share one port, `WebServerPort` in `AppSettings`
  (default 80 - on Linux that needs root, unless it is set above 1024; on Windows any user can listen on it, and
  without admin only the startup's firewall-rule setup fails, with a warning).
- Its state is in `C:\ProgramData\WhyTheFuckIsMyNetworkSoSlow\` or `/var/opt/WhyTheFuckIsMyNetworkSoSlow/`:
  `AppSettings.json`, and the database (by default the directory `db-localDBDir-v1`). Until the database has loaded,
  the API answers 503.
- There are no automated tests. A check is the build (with its warnings, as in Stroika), then a run: `curl` `/api/v1/about`,
  `/api/v1/healthcheck`, `/api/v1/devices?recurse=true`, `/api/v1/networks?recurse=true` and
  `/api/v1/network-interfaces?recurse=true`, and the UI's Devices and Networks pages. Discovery needs a real network
  and a few minutes. Say what you checked, and what you could not.
- Logs go to syslog or the Windows event log (`Logging` in `AppSettings`). DbgTrace goes to a trace file in Debug and
  Release-Logging builds; its name is logged at startup.

## Upgrading Stroika

Between Stroika releases the submodule tracks Stroika's `v3-Dev`. To upgrade:
1. `git status` - note what is local, the submodule pointer especially.
2. Move the submodule, from WTF's root: `make latest-submodules STROIKA_COMMIT=v3-Dev` (a checkout and pull in
   StroikaRoot; `STROIKA_COMMIT=<tag>` for a release).
3. Read Stroika's `Release-Notes.md`, "Upgrade Notes (X to Y)", for every release passed (`git describe` in the
   submodule, before and after). For each note, search WTF (excluding `ThirdPartyComponents/`); and compare WTF's
   Skel-derived files - the top `Makefile`, `Workspaces/`, `Installers/Makefile`, `html/` - with Stroika's
   `Build/Shared/Skel-Templates/`.
4. Rebuild from clean - `make distclean`, then `make all CONFIGURATION=Debug` (it makes the default configurations
   again): with `QUICK_BUILD=1` and no header dependencies, an incremental build compiles WTF against the new headers
   and links the old libraries.
5. Fix the errors, and the deprecation warnings too: deprecated Stroika names go away at v3.0a1.
6. Build Debug and Release, with MSVC and g++ at least; run and check as above.
7. Report every change WTF needed that the upgrade notes do not mention, and every note that was wrong or unclear, so
   Stroika's Release-Notes can be fixed.
8. The commit's headline names the Stroika version. (WTF's `Release-Notes.md` is written at release, not per commit.)
