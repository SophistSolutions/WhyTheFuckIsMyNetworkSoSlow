#!/bin/sh
# Settings live in the data volume's AppSettings.json. On each start: WTF_PORT, if set, is written into it as
# WebServerPort; and logging goes to stdout (docker logs), not syslog - a container has none - unless it says otherwise.
set -e
F=/var/opt/WhyTheFuckIsMyNetworkSoSlow/AppSettings.json
test -s "$F" || echo '{}' > "$F"
jq --arg port "${WTF_PORT:-}" '
    (if $port != "" then .WebServerPort = ($port | tonumber) else . end)
    | .Logging.ToStdOut |= (if . == null then true else . end)
    | .Logging.ToSysLog |= (if . == null then false else . end)' "$F" > "$F.tmp"
mv "$F.tmp" "$F"
# --run-as-service, not --run-directly: only it shuts down cleanly on SIGTERM (docker stop). Its pid file, if a killed run
# left one, names PID 1 - this process - and it would refuse to start as "already running". Once
# https://github.com/SophistSolutions/Stroika/issues/1273 is fixed, this can run --run-directly, with no pid file to delete.
rm -f /run/WhyTheFuckIsMyNetworkSoSlow-Service.pid
exec /opt/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Backend --run-as-service "$@"
