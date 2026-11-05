# WTF in a container, made from its .deb - Linux only: it must run on the host network to see the LAN (ReadMe.md)
ARG BASE_IMAGE=ubuntu:26.04
FROM ${BASE_IMAGE}

ARG DEB
COPY ${DEB} /tmp/
RUN apt-get update \
    && apt-get install -y --no-install-recommends jq /tmp/${DEB} \
    && rm -rf /var/lib/apt/lists/* /tmp/${DEB}

COPY --chmod=755 entrypoint.sh /usr/local/bin/wtf-entrypoint.sh

# AppSettings.json and the database
VOLUME /var/opt/WhyTheFuckIsMyNetworkSoSlow

ENTRYPOINT ["/usr/local/bin/wtf-entrypoint.sh"]
