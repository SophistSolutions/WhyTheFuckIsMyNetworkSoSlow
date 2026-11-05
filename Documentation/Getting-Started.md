# Getting started with WTF

WTF ("Why The Fuck Is My Network So Slow") runs as a service on one computer on your network. It finds the networks
and devices that computer can see, keeps their history, and shows them in a web page. It is early work: today it is a
good map of what is on your network, but it does little monitoring yet.

## 1. Install

Get the installer for your system from the
[Releases page](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/releases) - the newest release is at the
top.

### Windows 10 or 11 (64-bit)

1. Download `WhyTheFuckIsMyNetworkSoSlow-Windows-<version>-x86_64.msi`, and run it.
2. Windows may say "Windows protected your PC": the installer is not signed. Click **More info**, then **Run anyway**.
3. It installs a Windows service, which starts at once and with Windows. Open <http://localhost/>.

### Ubuntu 22.04, 24.04 or 26.04 (64-bit Intel/AMD)

1. Download the `.deb` for your Ubuntu version.
2. Install it: `sudo apt install ./whythefuckismynetworksoslow-<...>.deb`
3. It installs a systemd service, which starts at once and at boot. Open <http://localhost/>, or
   `http://<that computer's name or address>/` from another computer.

### Docker (Linux)

On a Linux computer with Docker Engine (a home server or a NAS, say):

```bash
docker run -d --name wtf --network host --restart unless-stopped \
    -e WTF_PORT=8080 -v wtf-data:/var/opt/WhyTheFuckIsMyNetworkSoSlow \
    sophistsolutionsinc/whythefuckismynetworksoslow:<version>
```

Then open `http://<that computer>:8080/`.

- `--network host` is required: without it, WTF sees only Docker's own private network, not your LAN.
- `WTF_PORT` sets the web page's port (WTF's own default is 80).
- `wtf-data` is a Docker volume that holds the settings and the database across upgrades. Name a different volume for
  each WTF you run.
- **Not with Docker Desktop** (Windows, macOS, or Linux): it runs containers inside a virtual machine, so WTF would
  only see that machine's network. On Windows, use the installer.

### macOS

No installer yet ([#60](https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/60)).

## 2. The first few minutes

- Right after it starts, the page may say the service is unavailable while WTF loads its database - give it a few
  seconds, and reload.
- The **Networks** page shows your network almost at once. Devices appear on the **Devices** page over the next few
  minutes, as WTF hears from them and scans your network; open ports and other details follow later.
- Click a device or network for its details. You can name devices, tag them, and add notes.
- The **About** page shows the version, and how WTF itself is doing.
- The JSON API that the web page uses is at `http://<host>/api`.

## 3. Settings

The settings file is `AppSettings.json`, in:

| System  | Folder                                                    |
| :------ | :-------------------------------------------------------- |
| Windows | `C:\ProgramData\WhyTheFuckIsMyNetworkSoSlow\`             |
| Linux   | `/var/opt/WhyTheFuckIsMyNetworkSoSlow/`                   |
| Docker  | the data volume (`wtf-data` above)                        |

The database is in the same folder. To change a setting, edit the file, then restart WTF:

- Windows: `Restart-Service WhyTheFuckIsMyNetworkSoSlow` in an administrator PowerShell, or restart "Why the Fuck is my
  Network So Slow" in the Services app.
- Linux: `sudo systemctl restart whythefuckismynetworksoslow`
- Docker: `docker restart wtf` (or run it again with a new `WTF_PORT`).

### The port

WTF serves its web page and API on one port, 80 unless `WebServerPort` says otherwise:

```json
{
    "WebServerPort": 8080
}
```

(keep the other settings in the file as they are). Pick a port nothing else on that computer uses - a web server, say.
To see what is using a port: `netstat -ano | findstr :80` on Windows, `sudo ss -ltnp | grep :80` on Linux.

If the port is taken, WTF logs an error ending "while binding to INADDR_ANY:80, while constructing webserver" - "Address
already in use" on Linux, "Only one usage of each socket address ..." on Windows - and stops, within a minute; on Linux,
systemd tries again every few seconds.

## 4. Logs

- Windows: Event Viewer, under **Windows Logs** > **Application**, source `WhyTheFuckIsMyNetworkSoSlow`.
- Linux: `journalctl -t WhyTheFuckIsMyNetworkSoSlow`
- Docker: `docker logs wtf`

## 5. Upgrade

Install the new version over the old one - the settings and the database are kept, unless the release notes say
otherwise. With Docker, stop and remove the container (`docker stop wtf`, then `docker rm wtf`), and run the new
version's image with the same volume.

## 6. Uninstall

- Windows: Settings > Apps > Installed apps > "Why the Fuck is my Network So Slow" > Uninstall.
- Ubuntu: `sudo apt remove whythefuckismynetworksoslow`
- Docker: `docker stop wtf`, then `docker rm wtf` - and `docker volume rm wtf-data` to delete its data too.

Uninstalling leaves the settings and database folder (above); delete it to remove everything. On Windows it also leaves
the firewall rule "WhyTheFuckIsMyNetworkSoSlow Receive SSDP Notify UDP Access Allowed".

## 7. Report a problem

Open an issue at <https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues>. Say what you saw and what
you expected; include the version (on the About page), your system, and the log lines from around that time.
