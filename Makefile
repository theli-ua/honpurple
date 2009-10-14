TARGET := src/libhon.so
VERSION := $(shell cat src/honprpl.h | sed -n 's/.*DISPLAY_VERSION.*"\(.*\)"/\1/p')

.PHONY: all

all: 
	@make -C src all
clean:
	@make -C src clean
	rm -rf deb dist

install: all
	mkdir -p ~/.purple/plugins/
	cp $(TARGET) ~/.purple/plugins/

sdist:
	mkdir -p dist
	tar -cvjpf --exclude .svn dist/honpurple-$(VERSION).tar.bz2 data src Makefile*

deb_common: all
	mkdir -p dist
	mkdir -p deb/usr/lib/pidgin/
	mkdir -p deb/usr/share/pixmaps/pidgin
	cp $(TARGET) deb/usr/lib/pidgin/
	rsync -r --exclude=.svn data/pixmaps deb/usr/share/
	rsync -r --exclude=.svn DEBIAN deb/

deb: deb_common
	sed -i "s:HONPURPLE_VERSION:$(VERSION):" deb/DEBIAN/control 
	dpkg-deb -b deb dist/honpurple-$(VERSION).deb
deb64: deb_common
	sed -i "s:HONPURPLE_VERSION:$(VERSION):" deb/DEBIAN/control
	sed -i "s:i386:amd64:" deb/DEBIAN/control
	dpkg-deb -b deb dist/honpurple_amd64-$(VERSION).deb
