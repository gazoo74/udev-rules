#
# Copyright 2017 Gaël PORTAY <gael.portay@gmail.com>
#
# Licensed under the MIT license.
#

CFLAGS	+= -std=c99 -Werror -Wall -Wextra

PREFIX	?= /usr/local

.PHONY: all
all: edidcat doc

doc: edidcat.1.gz hotplug-drm.7.gz hotplug-monitor.7.gz \
     hotplug-monitor.conf.5.gz

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
	install -m 0755 -d $(DESTDIR)$(PREFIX)/libexec/hotplug-monitor.d/
	install -d $(DESTDIR)$(PREFIX)/share/man/man1/
	install -m 644 edidcat.1.gz \
	           $(DESTDIR)$(PREFIX)/share/man/man1/
	install -d $(DESTDIR)$(PREFIX)/share/man/man5/
	install -m 644 hotplug-monitor.conf.5.gz \
	           $(DESTDIR)$(PREFIX)/share/man/man5/
	install -d $(DESTDIR)$(PREFIX)/share/man/man7/
	install -m 644 hotplug-drm.7.gz hotplug-monitor.7.gz \
	           $(DESTDIR)$(PREFIX)/share/man/man7/
	if [ -e "hotplug-monitor.conf.sample" ]; then \
		install -m 0755 -d $(DESTDIR)$(PREFIX)/share/hotplug-monitor/; \
		install -m 0644 hotplug-monitor.conf.sample \
		                $(DESTDIR)$(PREFIX)/share/hotplug-monitor/; \
	fi
	if [ -e "hotplug-monitor.conf" ]; then \
		install -m 0755 -d $(DESTDIR)$(PREFIX)/etc/; \
		install -m 0644 hotplug-monitor.conf $(DESTDIR)$(PREFIX)/etc/; \
	fi

.PHONY: conf
conf:
	@for dir in /sys/class/drm/card0/card0-*; do \
		if [ "$$(cat $$dir/status)" = "connected" ]; then \
			reference="$${dir##*/card0-}"; \
			break; \
		fi; \
	done; \
	product_uuid="$$(cat /sys/devices/virtual/dmi/id/product_uuid)"; \
	product_uuid="$${product_uuid// /_}"; \
	product_serial="$$(cat /sys/devices/virtual/dmi/id/product_serial)"; \
	product_serial="$${product_serial// /_}"; \
	product_name="$$(cat /sys/devices/virtual/dmi/id/product_name)"; \
	product_name="$${product_name// /_}"; \
	product_version="$$(cat /sys/devices/virtual/dmi/id/product_version)"; \
	product_version="$${product_version// /_}"; \
	sys_vendor="$$(cat /sys/devices/virtual/dmi/id/sys_vendor)"; \
	sys_vendor="$${sys_vendor// /_}"; \
	echo "#"; \
	echo "# Automatically generated file; EDIT TO YOUR CONVENIENCE."; \
	echo "# hotplug-monitor $$(git describe --always) Configuration"; \
	echo "#"; \
	echo "# Note this ini file use \`#' for comments instead of \`;'"; \
	echo "#"; \
	echo ""; \
	echo "# Sections override either machine or monitors preferences."; \
	echo "#"; \
	echo "# The machine section override the script default settings:"; \
	echo "#  - the reference monitor (here: $$reference):"; \
	echo "#    it is the one which serves as reference for position."; \
	echo "#  - the position of the monitor relative to the reference:"; \
	echo "#    left or right of $$reference."; \
	echo ""; \
	echo "# Override the default script preferences."; \
	echo "# This setup is preferred if no machine preferences"; \
	echo "[default]"; \
	echo "reference=$$reference"; \
	echo "position=right"; \
	echo ""; \
	echo "# This machine may override the default preferences."; \
	echo "# It can be identified in 5 ways"; \
	echo "#"; \
	echo "# Either using its UUID:"; \
	echo "# (read from /sys/devices/virtual/dmi/id/product_uuid)"; \
	echo "[$$product_uuid]"; \
	echo "reference=$$reference"; \
	echo "position=right"; \
	echo ""; \
	echo "# Or using its serial:"; \
	echo "# (read from /sys/devices/virtual/dmi/id/product_serial)"; \
	echo "# [$$product_serial]"; \
	echo "# reference=$$reference"; \
	echo "# position=right"; \
	echo "#"; \
	echo "# Or using the vendor and the product name:"; \
	echo "# (read from /sys/devices/virtual/dmi/id/sys_vendor"; \
	echo "#   and from /sys/devices/virtual/dmi/id/product_name)"; \
	echo "# [$${sys_vendor}_$$product_name]"; \
	echo "# reference=$$reference"; \
	echo "# position=right"; \
	echo "#"; \
	echo "# Or using the vendor and the product version:"; \
	echo "# (read from /sys/devices/virtual/dmi/id/sys_vendor"; \
	echo "#   and from /sys/devices/virtual/dmi/id/product_version)"; \
	echo "# [$${sys_vendor}_$$product_version]"; \
	echo "# reference=$$reference"; \
	echo "# position=right"; \
	echo "#"; \
	echo "# Or using the vendor only:"; \
	echo "# (read from /sys/devices/virtual/dmi/id/sys_vendor)"; \
	echo "# [$$sys_vendor]"; \
	echo "# reference=$$reference"; \
	echo "# position=right"; \
	echo "#"; \
	echo "# Pick up one of the setup above (many if you handle a park)."; \
	echo "# Only the first one will apply to this single machine."; \
	echo "# The four others will apply either to same products machines,"; \
	echo "# or to the same manufacturer machines (the last)."; \
	echo ""; \
	echo "# The following sections defines per monitor settings:"; \
	for dir in /sys/class/drm/card0/card0-*; do \
		if [ "$${dir##*/card0-}" = "$$reference" ]; then \
			echo "# Set reference monitor is a non-sense."; \
			echo "# [$${dir##*/card0-}]"; \
			echo "# reference=$$reference"; \
			echo "# position=right"; \
			echo ""; \
			continue; \
		fi; \
		echo "[$${dir##*/card0-}]"; \
		echo "reference=$$reference"; \
		echo "position=right"; \
		echo ""; \
	done; \
	echo "" >$@

.PHONY: hotplug-monitor.conf.sample
hotplug-monitor.conf.sample:
	@$(MAKE) -s conf >$@

.PHONY: check
check:
	shellcheck hotplug-drm hotplug-monitor

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

uevent-add uevent-remove uevent-change:
uevent-%:
	@devpath="$$(udevadm info -q path -n /dev/dri/card0)"; \
	echo "$*" >"/sys/$$devpath/uevent"

show-connectors:
	@for dir in /sys/class/drm/card0/card0-*; do \
		echo "$${dir##*/card0-}"; \
	done

define do_run =
run-$(1):
endef

connectors := $(shell ls -1d /sys/class/drm/card0/card0-* | sed -e 's,^/sys/class/drm/card0/card0-,,')
$(foreach connector,$(connectors),$(eval $(call do_run,$(connector))))

run-%:
	rm /tmp/hotplug-drm/$*/ -rf
	@echo "Switch off $* and wait 3 seconds..."
	xrandr --output "$$(echo "$*" | sed -e 's,-[A-Z],,' -e 's,-,,')" --off && sleep 3
	@echo "Simulate connection of $*..."
	devpath="/sys/class/drm/card0/card0-$*"; \
	echo "change" >"$$devpath/uevent"

.PHONY: clean
clean:
	rm -f edidcat edidcat.1.gz hotplug-drm.7.gz hotplug-monitor.7.gz \
	      hotplug-monitor.conf.5.gz

%.1: %.1.adoc
	asciidoctor -b manpage -o $@ $<

%.5: %.5.adoc
	asciidoctor -b manpage -o $@ $<

%.7: %.7.adoc
	asciidoctor -b manpage -o $@ $<

%.gz: %
	gzip -c $^ >$@

