# udev-rules

This repository provides few *udev(7)* rules and some extra *shell* scripts.
Those scripts are triggered by netlink *uevents* via *udev(7)*.

Scripts are *device-manager* agnostics. They can be used by the *busybox(1)*
*mdev* applet or within an home-made helper script (*/sbin/hotplug*).

## NAME

[hotplug-drm(7)](hotplug-drm.7.adoc) - splits drm [uevent](52-drm.rules#L15)
into card's connectors uevents

[hotplug-monitor(7)](hotplug-monitor.7.adoc) - setups monitor upon card's
connector [uevent](53-drm-connector.rules#L7)

## DESCRIPTION

[hotplug-drm](hotplug-drm) and [hotplug-monitor](hotplug-monitor) are both
shell scripts triggered by *udev(7)*.

Because there is an *uevent* on card change but not on its connector, a script
has to trigger it manually.

The goal of [hotplug-drm](hotplug-drm#L29-L50) is to split the *drm uevent* on
card change into a *sub-uevent* on the card's connector. The script saves all
connector's states under the */tmp/hotplug-drm* directory to trigger a connector
*uevent* only if its *status* has changed.

[hotplug-monitor](hotplug-monitor) is triggered by the *connector sub-uevent*
mentioned above. It setups the monitors layout using *xrandr(1)* according to a
very simple *ini* style configuration file. This file is located either under
[/usr/etc/hotplug-monitor.conf](hotplug-monitor.conf.sample) or
*/etc/hotplug-monitor.conf*.

## FILES

[/usr/etc/hotplug-monitor.conf](hotplug-monitor.conf.sample)
	Configuration file for [hotplug-monitor(7)](hotplug-monitor.conf.5.adoc).

/tmp/hotplug-drm
	Temporary directory used by *hotplug-drm(7)* to save connectors states.

## REQUIREMENT

[hotplug-monitor](hotplug-monitor#L28-L52) needs *bash*. It uses _arrays_ to
handle the conffile content line-by-line, and _replacement substitution_.

## BUGS

Report bugs at *https://github.com/gazoo74/udev-rules/issues*

## AUTHOR

Written by Gaël PORTAY *gael.portay@savoirfairelinux.com*

## SEE ALSO

udev(7), netlink(7), xrandr(1)

## COPYRIGHT

Copyright (c) 2017 Gaël PORTAY

This program is free software: you can redistribute it and/or modify it under
the terms of the MIT License.
