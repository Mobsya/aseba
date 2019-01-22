Thymio Device Manager
#####################

The Thymio Device Manager is a bridge between the Thymio robots and the various applications (Aseba Studio, VPL, Blockly, Scratch, etc).

Starting with version 1.7, it is not possible to use the applications while a Thymio Device Manager is not running.



Usage
=====

Launching the Device Manager
----------------------------

The Device Manager will be automatically started when you start the Thymio Launcher and it is therefore not necessary to launch it manually.
However, it can be launched manually by executing the ``thymio-device-manager`` command.

Note that only one single ``Thymio Device Manager`` instance can be launched on a computer.

.. warning:: It is not recommended to launch ``thymio-device-manager`` as ``root`` or administrator


Connecting a Thymio through USB
-------------------------------

When plugging a Thymio in your computer, it will automatically be detected and made available by the Thymio Device Manager so that it can be used
by the various applications of the Thymio Software Suite.
Please note that you will need to launch the various applications through the launcher.

If the Thymio Device Manager does not detect a device, make sure it is properly plugged in.

On Linux, you might need to create the following `udev rule <https://en.wikipedia.org/wiki/Udev>`_ to give non-root users access to the USB devices.

::

    SUBSYSTEM=="usb", ATTRS{idVendor}=="0617", MODE="0666"


Please refer to the documentation of your distribution for more information about ``udev`` (`Arch <https://wiki.archlinux.org/index.php/udev#About_udev_rules>`_, `Debian <https://wiki.debian.org/udev>`_, `Ubuntu <http://manpages.ubuntu.com/manpages/xenial/man7/udev.7.html>`_)

.. important::  The Thymio Device Manager does not support Thymios with a firmware 9 or older. Make sure to update your Thymio to the latest firmware available. Some features are only available with a firmware 13 or newer.

.. note::  The Thymio Device Manager does not support non-Thymios robots.

Connecting a Wireless Thymio
----------------------------

See `Connecting a Thymio through USB`_. You can pair multiple Thymios to the same dongle, and they will all be detected by the Thymio Device Manager.
However, because of physical limitations, we advise against connecting more than 6 Thymios to the same dongle.

.. note::  When connecting through a wireless dongle, it is important that the Thymios have an up to date firmware (at least version 13 or later).

.. note::  Thymios connected to the same dongle always form an implicit group ( they share the same events and shared variables).


Connecting a Simulated Thymio
-----------------------------

When launching a Playground scenario, the associated simulated Thymios will automatically connect to the device manager and become visible in the launcher.
On windows, the ``Bonjour`` Service bust be running. On Linux, the  ``avahi-deamon`` service must be installed and running.

.. note::  The Thymio Device Manager does not support non-Thymios simulated robots

Connecting to a Thymio Device Manager
-------------------------------------

The Thymio Device Manager is accessed by applications (launcher, studio) through a randomly assigned port which is discovered by zeroconf.
In order to be able to use the Thymio Device Manager and any other Thymio Software, make sure your computer or system administrator does not block zeroconf (avahi, or bonjour).

Applications are able to see and interact with Thymios connected on other computers from the same network because Thymio Device Manager makes itself visible
on the network through zeroconf.

Web-based applications such as ``Scratch`` and ``Blockly`` access the Device Manager through the port ``8597``. Make sure this port is not blocked by your system administrator.


Features
========

The Thymio Device Manager introduces a few notions and concepts used by other applications.

Thymio Status
-------------

Since the Thymio Device Manager allow the use of Thymios connected to other computers as well as launching multiple applications at the same time, a notion of status is
introduced to prevent concurrent access to the Thymios.
Thymios may be in any of these statuses:

 * ``Connected`` : The Thymio was detected by the Thymio Device Manager but can not be used yet as its state needs to be synchronized. A Thymio should never be in this state more than a few seconds
 * ``Available`` : The Thymio can be used
 * ``Ready``     : You are using this Thymio, no one can send code or other data to it until you are done using it.
 * ``Busy``      : Someone else is using the Thymio; You can see the state of its variables, events, etc but not send code or set variables to it.
 * ``Disconnected`` : The Thymio was physically disconnected or is too far from the dongle. A disconnected Thymio is not usable.

Groups
------

A Group is a set of Thymios that share the same definitions of events as well as the same shared variables.
Thymios in the same group can also send each other events.

Events send by applications are sent to all Thymios in the same group.

Thymios connected to the same dongle implicitly share the same group.
In the future, it will be possible to create groups from multiple dongle or usb-connected Thymios as well as simulated Thymios, however, Thymios connected to the same dongle
can never be in different groups.


.. note::  Some applications such as ``VPL`` do not support groups.
.. note::  Datas associated with groups ( events and shared variables ) are lost when the Device Manager is closed.

Reconnection
------------

Thymios are uniquely identified and so will seamlessly reconnect after being unplugged or rebooted.

The Device Manager will try to put the Thymios in the same group it was in before the disconnection
and restore the events and shared variables.
In some cases, this is not possible and so some events and shared variables may be lost.


.. note::  This feature requires a firmware version 13 or greater


Permissions
-----------

While the Device Manager does not have user accounts, it offers a rudimentary permission system.
Notably:

 * Renaming a Thymio is only possible from the same machine the Thymio is connected to
 * Force Stopping(stopping an otherwise busy Thymio) is only possible from the same machine the Thymio is connected to.


Writing Applications compatible with the Thymio Device Manager
==============================================================

Applications interact with the Device Manager through a stateful endpoint which can either be:
 * A tcp socket
 * A WebSocket

Both endpoints expect a `flexbuffer-based protocol <https://github.com/Mobsya/aseba/blob/master/aseba/flatbuffers/thymio.fbs>`_.
For convenience, Mobsya provides 2 APIs: One JS based compatible with NPM and Web-based applications, as well as a Qt-based API  which we usee for the luncher, Studio and VPL classic
