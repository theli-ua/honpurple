TARGET := src/libhon.so
VERSION := $(shell cat src/honprpl.h | sed -n 's/.*DISPLAY_VERSION.*"\(.*\)"/\1/p')

.PHONY: all
all: $(TARGET)

$(TARGET): 
	@make -C src

clean:
	@make -C src clean

install: $(TARGET)
	cp $(TARGET) ~/.purple/plugins/

sdist:
	tar -cvjpf honpurple-$(VERSION).tar.bz2 data src Makefile*
