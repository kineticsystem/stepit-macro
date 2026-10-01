# The Package Cache

Building the rig installs many Debian packages: those of each image's
`Dockerfile`, and those that rosdep installs for the code of each module
(`update.sh`). Without a cache, every one of them comes from the internet, again
and again. Two caches keep them instead, one on your machine and one on GitHub,
for CI:

| Where | What keeps the packages | Speeds up |
|---|---|---|
| Your machine | `stepit-apt-cache`, an apt-cacher-ng proxy | `./docker/dock.sh build`, and the containers' rosdep |
| GitHub Actions | The Actions cache | The `plugins`, `stepit-macro` and `objectives` jobs of `ci.yml` |

They are independent: CI never sees your machine's cache, and the other way
round.

## How apt downloads a package

apt, which rosdep calls, installs a package in two steps: it downloads the
package's file, a `.deb`, from a repository, e.g. `http://archive.ubuntu.com` or
`http://packages.ros.org`, into `/var/cache/apt/archives`, then unpacks it. If
the right `.deb` is already in that folder, apt skips the download.

Docker images of Ubuntu, and of ROS on top of it, delete every `.deb` right after
installing it, to keep images small: the setting is the file
`/etc/apt/apt.conf.d/docker-clean`. So inside a container, nothing is kept.

## On your machine: apt-cacher-ng

[apt-cacher-ng](https://www.unix-ag.uni-kl.de/~bloch/acng/) is a caching
proxy made for Debian packages. A proxy is a server that apt asks instead of the
repository: apt says "give me this file of `packages.ros.org`", and the proxy
downloads it from there and passes it on. A caching proxy also keeps a copy, so
the next time anyone asks for the same file, it answers from its disk without
going to the internet. The "ng" stands for "next generation": it replaced an
older tool, apt-cacher.

It is the usual way to share downloads between many machines of a network; here
the "machines" are the containers of the rig and the image builds. It suits
them better than sharing `/var/cache/apt/archives` directly: apt locks that
folder while it works, so containers starting together would fail on each
other's lock, while the proxy serves many of them at once.

```
 image builds ─────┐
 stepit-driver ────┤
 stepit-commander ─┤  127.0.0.1:3142   ┌──────────────────┐  only on a miss  ┌────────────────────┐
 stepit-macro ─────┼─────────────────▶ │ stepit-apt-cache │ ───────────────▶ │ archive.ubuntu.com │
 stepit-camera ────┘                   │  (apt-cacher-ng) │                  │ packages.ros.org   │
                                       └────────┬─────────┘                  └────────────────────┘
                                                │
                                   volume stepit-macro_apt-cache
```

How it is wired:

- **The proxy** is the `apt-cache` service of
  [`docker-compose.yml`](../docker/docker-compose.yml), built from
  [`docker/apt-cache/Dockerfile`](../docker/apt-cache/Dockerfile). It runs on the
  host network and listens on `127.0.0.1:3142`. Its packages live in the Docker
  volume `stepit-macro_apt-cache`, so they outlive the container.
- **The image builds**: `./docker/dock.sh build` starts the proxy first, then
  builds the images with `http_proxy=http://127.0.0.1:3142`, on the host network
  so that they reach it. Docker does not count the proxy setting when deciding
  whether an image layer can be reused, so it does not cause rebuilds.
- **The containers**: the ROS containers mount
  [`apt-proxy.conf`](../docker/apt-cache/apt-proxy.conf) into
  `/etc/apt/apt.conf.d`. It tells apt to ask
  [`apt-proxy-detect`](../docker/apt-cache/apt-proxy-detect) which proxy to use:
  the cache when it is listening, none otherwise. So apt in a container works
  with or without the proxy running.

Only plain HTTP downloads go through it, which is what the Ubuntu and ROS
repositories use. HTTPS downloads, e.g. Node.js, pip and pnpm packages, go
directly to the internet and are not cached.

### rosdep installs once, into the image

The proxy saves the download, but apt still has to unpack every package in each
new container. `dock.sh build` therefore installs rosdep's packages once, in the
container it compiles each module in, and then saves that container as the
service's image (`docker commit`). The image then holds the packages and the
marker file `~/.dependencies`, so the service's own container, on its first
start, finds the marker and skips `update.sh`.

A container started from an image that `dock.sh build` did not compile, e.g.
after a plain `docker compose build`, still runs `update.sh` on its first start,
as before.

### Day to day

Nothing to do: `./docker/dock.sh build` and `./docker/dock.sh start` start the
proxy, which then restarts with Docker until `./docker/dock.sh stop`. Useful commands:

```bash
docker logs stepit-apt-cache                       # what it served
docker exec stepit-apt-cache du -sh /var/cache/apt-cacher-ng   # how big it is
docker volume rm stepit-macro_apt-cache            # empty it (stop the proxy first)
```

Its statistics page is on <http://localhost:3142/acng-report.html>.

`./docker/dock.sh clean` removes the containers and images, the proxy's
included, but keeps the volume: the next build starts with the packages already
there.

## On GitHub: the Actions cache

CI runs on a fresh machine every time, so it cannot use the proxy. It uses the
[Actions cache](https://docs.github.com/en/actions/using-workflows/caching-dependencies-to-speed-up-workflows)
instead, which stores a folder between runs. Each job of
[`ci.yml`](../.github/workflows/ci.yml) that installs packages:

1. deletes `docker-clean`, so that apt keeps the `.deb` files;
2. restores its newest cache, `apt-<job>-…`, into `/var/cache/apt/archives`;
3. installs, downloading only what is missing;
4. runs `apt-get autoclean`, which drops versions the repositories no longer
   serve, and saves the folder as a new cache named after a hash of the files
   it holds. When nothing new was downloaded, that cache exists already and
   nothing is saved.

When ROS publishes new versions of packages, the next run downloads them and
saves one new cache, which the runs after it restore. GitHub deletes caches
unused for 7 days, and the oldest ones when a repository exceeds its space.
Pull requests from forks can read the cache but not write to it.

## Troubleshooting

**`dock.sh build` says `The package cache is not listening`.** The proxy did not
start, e.g. because port 3142 is taken: see `docker logs stepit-apt-cache`. The
build goes on, downloading directly.

**apt fails with a 503 or a hash mismatch from `127.0.0.1:3142`.** The cache
holds a broken file, e.g. from an interrupted download. Empty it:
`./docker/dock.sh stop`, `docker rm stepit-apt-cache`,
`docker volume rm stepit-macro_apt-cache`, then build again.
