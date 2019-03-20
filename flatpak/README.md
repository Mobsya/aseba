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
flatpak-builder <build-dir> org.mobsya.ThymioSuite.json --ccache --force-clean --keep-build-dirs -v --install-deps-from=flathub --user
```

*  `<build-dir>` is the tempory build directory for the flatpak