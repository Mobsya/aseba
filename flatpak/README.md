# Install the Thymio Suite on Linux

## Install Flatpak

Please refer to the installation instructions on [Flathub](https://flatpak.org/setup/).

## Set-up `udev` rules for thymio

Before being able to use a Thymio, you need to set up the
appropriate udev rules to grant your user account permissions
to use the device? This only needs to be done once per computer and doesn't have to be
done again if you reinstall the Thymio Suite.

The instructs are given for a `Ubuntu` system.
If you use another distribution, the steps to follow might be
slightly different. Please refer to the documentation of your distribution concerning **udev rules**.


⚠️ This step needs to be executed before you launch Thymio Suite.

⚠️ You will need root access - if you don't own the computer, ask your network administrator.

Using `nano` or `vi`, edit `/etc/udev/rules.d/99-mobsya.rules`.
For example

`sudo nano /etc/udev/rules.d/99-mobsya.rules`

Add the following line into the file:

```
SUBSYSTEM=="usb", ATTRS{idVendor}=="0617", ATTRS{idProduct}=="000a", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0617", ATTRS{idProduct}=="000c", MODE="0666"
```

Save the file (`Ctrl+x` if you use `nano`), then execute

```
sudo udevadm control --reload-rules
```

## Install Thymio Suite from Flathub

You can [Install Thymio Suite from Flathub](https://flathub.org/apps/details/org.mobsya.ThymioSuite). **This is the recommanded installation method**.

## Install the development version of Thymio Suite


1. [Download the Thymio Suite Flatpak bundle 📦 ](https://github.com/Mobsya/aseba/releases/download/2.0.0-RC3/ThymioSuite-2.0.0-RC3-linux.flatpak)
2. In a terminal, execute

```
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo  
flatpak install flathub org.kde.Platform/x86_64/5.12 --user
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

⚠️ When installing Thymio Suite from a bundle, you will have to install
new versions manually as they will not update automatically.

⚠️ Development versions can be unstable.

## Build the flatpak version from source

This section is reserved for developers wishing to manually
build the Flatpak version from source.

### install `flatpack-builder`

Please refer to the installation instructions on [Flathub](https://flatpak.org/setup/).

### Build the Thymio Suite Flatpak

* Building the application:

In the `flatpack` repository, run :
```
flatpak-builder <build-dir> org.mobsya.ThymioSuite.json --ccache --force-clean --keep-build-dirs \
-v --install-deps-from=flathub --user --repo=mobsya-repo
```

*  `<build-dir>` is the tempory build directory for the Flatpak

* Building the bundle:

```
flatpak build-bundle  --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo mobsya-repo \
thymio.flatpak org.mobsya.ThymioSuite
```
