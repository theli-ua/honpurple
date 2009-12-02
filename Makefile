TARGET := src/libhon.so
VERSION := $(shell cat src/honprpl.h | sed -n 's/.*DISPLAY_VERSION.*"\(.*\)"/\1/p')
PREFIX = /usr
DESTDIR =
ARCH := $(shell uname -m)
PIXMAPS:=$(shell cd data ; find . -type f -iname *.png)

.PHONY: all

all: 
	@make -C src all
clean:
	@make -C src clean
	rm -rf deb dist

install: all
	@install -m 0755 -D  ${TARGET} ${DESTDIR}/${PREFIX}/lib/purple-2/libhon.so
	@for file in ${PIXMAPS}; do \
		install -D -m 644 data/$$file ${DESTDIR}/${PREFIX}/share/$$file; \
	done
sdist:
	mkdir -p dist
	tar -cvjp --exclude .svn -f dist/honpurple-$(VERSION).tar.bz2 data src Makefile*
deb: all
	mkdir -p dist
	mkdir -p deb/usr/lib/purple-2
	mkdir -p deb/usr/share/pixmaps/pidgin
	cp $(TARGET) deb/usr/lib/purple-2/
	rsync -r --exclude=.svn data/pixmaps deb/usr/share/
	rsync -r --exclude=.svn DEBIAN deb/
	sed -i "s:HONPURPLE_VERSION:$(VERSION):" deb/DEBIAN/control
	sed -i "s:i386:${ARCH}:" deb/DEBIAN/control
	dpkg-deb -b deb dist/honpurple_${ARCH}-$(VERSION).deb
