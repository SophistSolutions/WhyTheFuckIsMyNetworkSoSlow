# Making a release

The installers come from the release commit's CI run (build-N-test, on `v1-Release`), attached to a GitHub release
unzipped, one file per platform, named for it - so a tester downloads one file and runs it.

1. Take the green run's ID (`gh run list -R SophistSolutions/WhyTheFuckIsMyNetworkSoSlow -b v1-Release -L 3`), and in an
   empty directory:

   ```bash
   RUN=<run id>; VER=<version, e.g. 1.0d21>; R=SophistSolutions/WhyTheFuckIsMyNetworkSoSlow
   gh run download $RUN -R $R -n "Installer (windows-vs2k26-x86_64-Release)" -D win
   mv win/*.msi WhyTheFuckIsMyNetworkSoSlow-Windows-$VER-x86_64.msi
   for u in 22.04 24.04 26.04; do
       gh run download $RUN -R $R -n "ubuntu-$u (Release) Installers" -D u$u
       mv u$u/*.deb whythefuckismynetworksoslow-$VER-ubuntu-${u}_x86_64.deb
   done
   ```

2. Write the release's notes (below) to `notes.md`, then
   `gh release create $VER -R $R --target v1-Release --prerelease --title "$VER {<date>}" --notes-file notes.md *.msi *.deb`

3. The Docker image: in WTF-Dev-2604, build Release, then `make -C Installers/Docker image push`
   ([Installers/Docker/ReadMe.md](Installers/Docker/ReadMe.md)).

## The release's notes

The TLDR from Release-Notes.md, then:

```markdown
## Install

[Getting started](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/blob/<VER>/Documentation/Getting-Started.md)
has the details: settings, logs, upgrading, uninstalling.

- **Windows 10/11**: download the `.msi` and run it (it is not signed: if Windows says "Windows protected your PC", click
  More info, then Run anyway). Then open http://localhost/
- **Ubuntu 22.04, 24.04, 26.04**: download the `.deb` for your version, then
  `sudo apt install ./whythefuckismynetworksoslow-<VER>-ubuntu-<version>_x86_64.deb` - and open http://localhost/
- **Docker (Linux)**:
  `docker run -d --name wtf --network host -e WTF_PORT=8080 -v wtf-data:/var/opt/WhyTheFuckIsMyNetworkSoSlow sophistsolutionsinc/whythefuckismynetworksoslow:<VER>`
  - and open http://<host>:8080/

WTF serves its page on port 80 by default - `WebServerPort` in AppSettings.json changes it (see Getting started).

## Details

[Release-Notes.md](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/blob/<VER>/Release-Notes.md)
```
