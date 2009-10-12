TARGET := src/libhon.so
VERSION := $(shell cat src/honprpl.h | sed -n 's/.*DISPLAY_VERSION.*"\(.*\)"/\1/p')

.PHONY: all

all: 
	@make -C src all
$(TARGET): 
	@make -C src $(TARGET)

clean:
	@make -C src clean
	rm -rf deb dist

install: $(TARGET)
	cp $(TARGET) ~/.purple/plugins/

sdist:
	mkdir -p dist
	tar -cvjpf --exclude .svn dist/honpurple-$(VERSION).tar.bz2 data src Makefile*

deb: all
	mkdir -p dist
	mkdir -p deb/usr/lib/pidgin/
	mkdir -p deb/usr/share/pixmaps/pidgin
	cp $(TARGET) deb/usr/lib/pidgin/
	rsync -r --exclude=.svn data/pixmaps deb/usr/share/
	cp -R DEBIAN deb/
	sed -i "s:HONPURPLE_VERSION:$(VERSION):" deb/DEBIAN/control 
	dpkg-deb -b deb dist/honpurple-$(VERSION).deb
