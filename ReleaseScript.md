# Making a release

The installers come from the release commit's CI run (build-N-test, on `v1-Release`), attached to a GitHub release
unzipped, one file per platform, named for it - so a tester downloads one file and runs it.

1. **Release notes**: a `### <VER> {<date>}` section at the top of Release-Notes.md's History - from
   `git log --no-merges <previous release's tag>..v1-Dev` and the issues closed since. Its TLDR becomes the GitHub
   release's notes (below), so it is for people trying WTF, and says anything an upgrade loses or changes (the database,
   a setting): Getting-Started.md's Upgrade section promises that. Then Change-Details.

2. **VERSION**: drop the `x` (`1.0d21x` -> `1.0d21`), in the notes' commit ("release 1.0d21"). The build puts it in the
   executable and the installers' names, and CI tags the Docker image with it - and `:latest` for a version without an
   `x` - so it must be right before the push to `v1-Release`.

3. **Push**: push `v1-Dev`, then fast-forward `v1-Release` to it (`git push origin v1-Dev:v1-Release` - `v1-Release`
   has no commits of its own). Wait for that build-N-test run to go green.

4. Take the green run's ID (`gh run list -R SophistSolutions/WhyTheFuckIsMyNetworkSoSlow -b v1-Release -L 3`), and in an
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

5. Write the release's notes (below) to `notes.md`, then
   `gh release create $VER -R $R --target v1-Release --prerelease --title "$VER {<date>}" --notes-file notes.md *.msi *.deb`

6. The Docker image: CI pushed it with the release commit - check that run's `docker-image` job, and that Docker Hub has
   `sophistsolutionsinc/whythefuckismynetworksoslow:$VER` (and `:latest`). By hand, if need be:
   [Installers/Docker/ReadMe.md](Installers/Docker/ReadMe.md).

7. **Start the next version** on `v1-Dev`: VERSION `1.0d22x` ("start 1.0d22x").

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
