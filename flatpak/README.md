# Run

## Install flatpak

Please refer to the installation instructions on [Flathub](https://flatpak.org/setup/).

## Install the development flatpak bundle

**This method is not recommended.**

1. [Download the bundle](https://github.com/Mobsya/aseba/releases/download/nightly/thymio.flatpak)
2. In a terminal, execute

```
flatpak install thymio.flatpak --user -y
```

This will install Thymio and its dependencies (~500MB)

3. To launch the Thymio Suite, simply execute

```
flatpak run org.mobsya.ThymioSuite
```

To launch only the device manager instead of the launcher,
execute

```
flatpak run --command=thymio-device-manager org.mobsya.ThymioSuite
```

# Build

## install `flatpack-builder`

## Ubuntu
```
sudo add-apt-repository ppa:alexlarsson/flatpak
sudo apt update
sudo apt install flatpak flatpak-builder
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

## Other distributions

Please refer to the installation instructions on [Flathub](https://flatpak.org/setup/).

## Build the Thymio Suite Flatpak

In the `flatpack` repository, run :
```
flatpak-builder <build-dir> org.mobsya.ThymioSuite.json --ccache --force-clean --keep-build-dirs -v --install-deps-from=flathub --user --repo=mobsya-repo
```

*  `<build-dir>` is the tempory build directory for the flatpak

Building the bundle:


```
flatpak build-bundle  --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo mobsya-repo thymio.flatpak org.mobsya.ThymioSuite
```