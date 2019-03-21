# Install the Thymio Suite on Linux

## Install flatpak

Please refer to the installation instructions on [Flathub](https://flatpak.org/setup/).

## Instal Thymio Suite from Flathub

[TODO]

## Install the development version of Thymio Suite

**This method is not recommended.**

1. [Download the Thymio Suite flatpak bundle](https://github.com/Mobsya/aseba/releases/download/nightly/thymio.flatpak)
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
you can execute

```
flatpak run --command=thymio-device-manager org.mobsya.ThymioSuite
```

## Build the flatpak version from source

This section is reserved for developers whishing to manually
build the flatpak version from source.

### install `flatpack-builder`

Please refer to the installation instructions on [Flathub](https://flatpak.org/setup/).

### Build the Thymio Suite Flatpak

* Building the application:

In the `flatpack` repository, run :
```
flatpak-builder <build-dir> org.mobsya.ThymioSuite.json --ccache --force-clean --keep-build-dirs \
-v --install-deps-from=flathub --user --repo=mobsya-repo
```

*  `<build-dir>` is the tempory build directory for the flatpak

* Building the bundle:

```
flatpak build-bundle  --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo mobsya-repo \
thymio.flatpak org.mobsya.ThymioSuite
```

