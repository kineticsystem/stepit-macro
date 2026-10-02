# The rig's local package cache: an apt-cacher-ng proxy that keeps every Debian
# package the containers download, from Ubuntu and from packages.ros.org, so
# that the next build or rosdep install takes it from the disk instead of the
# internet. See docker-compose.yml.
FROM ubuntu:24.04

LABEL name="develop:stepit-apt-cache"

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get -y install --no-install-recommends apt-cacher-ng \
 && rm -rf /var/lib/apt/lists/* \
 && install -d -o apt-cacher-ng -g apt-cacher-ng /run/apt-cacher-ng   # Its local socket, made by systemd elsewhere.

# The cache, a volume: it outlives the container and `dock.sh clean`.
VOLUME /var/cache/apt-cacher-ng

EXPOSE 3142

USER apt-cacher-ng
CMD ["/usr/sbin/apt-cacher-ng", "-c", "/etc/apt-cacher-ng", "ForeGround=1", "Port=3142", \
     "BindAddress=127.0.0.1"]
