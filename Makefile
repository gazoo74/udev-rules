#
# Copyright 2017 Gaël PORTAY <gael.portay@gmail.com>
#
# Licensed under the MIT license.
#

CFLAGS	+= -std=c99 -Werror -Wall -Wextra

PREFIX	?= /usr/local
DPDEV	?= HDMI1

.PHONY: all
all: edidcat

hotplug-monitor.conf:
	( cd /sys/class/drm/card0/ && ls -1d card0-* ) | \
	sed -e 's,card0-,,' -e 's,-[A-Z],,' -e 's,-,,' | \
	sed -e 's,^,[,' -e 's,$$,]\nposition=right\n,' >$@

.PHONY: conf
conf: hotplug-monitor.conf

.PHONY: install
install:
	install -m 0755 -d $(DESTDIR)/etc/udev/rules.d/
	install -m 0644 52-drm.rules $(DESTDIR)/etc/udev/rules.d/
	sed -e 's,/usr,$(PREFIX),' \
	    -i $(DESTDIR)/etc/udev/rules.d/52-drm.rules
	install -m 0644 53-drm-connector.rules $(DESTDIR)/etc/udev/rules.d/
	sed -e 's,/usr,$(PREFIX),' \
	    -e '/SUBSYSTEM=="drm"/s#$$#, ENV{DISPLAY}="$(DISPLAY)"#' \
	    -e '/SUBSYSTEM=="drm"/s#$$#, ENV{XAUTHORITY}="$(XAUTHORITY)"#' \
	    -i $(DESTDIR)/etc/udev/rules.d/53-drm-connector.rules
	install -m 0755 -d $(DESTDIR)$(PREFIX)/bin/
	install -m 0755 edidcat $(DESTDIR)$(PREFIX)/bin/
	install -m 0755 hotplug-drm $(DESTDIR)$(PREFIX)/bin/
	install -m 0755 hotplug-monitor $(DESTDIR)$(PREFIX)/bin/
	sed -e 's,/usr,$(PREFIX),' \
	    -i $(DESTDIR)$(PREFIX)/bin/hotplug-monitor
	if [ -e "hotplug-monitor.conf" ]; then \
		install -m 0755 -d $(DESTDIR)$(PREFIX)/etc/; \
		install -m 0644 hotplug-monitor.conf $(DESTDIR)$(PREFIX)/etc/; \
	fi

.PHONY: reload
reload:
	udevadm control --reload-rules

.PHONY: monitor
monitor:
	udevadm monitor --property

.PHONY: test
test:
	devpath="$$(udevadm info -q path -n /dev/dri/card0)"; \
	udevadm test --action=change "$$devpath"

.PHONY: uevent-add uevent-remove uevent-change
uevent-add uevent-remove uevent-change:
uevent-%:
	@devpath="$$(udevadm info -q path -n /dev/dri/card0)"; \
	echo "$*" >"/sys/$$devpath/uevent"

.PHONY: tests
tests:
	@echo "Switch off $(DPDEV) and wait 3 seconds..."
	xrandr --output "$(DPDEV)" --off && sleep 3
	@echo "Simulate connection of $(DPDEV)..."
	sudo $(MAKE) -f Makefile uevent-change

.PHONY: clean
clean:
	rm -f edidcat

