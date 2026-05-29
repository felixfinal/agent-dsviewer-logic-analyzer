# dslogic-cli — DSLogic Native CLI
# Build: make && sudo make install

VERSION = 3.2.0
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

CC     = gcc
CFLAGS = -Wall -Wextra -O2 -DVERSION=\"$(VERSION)\"
PKGS   = glib-2.0
LIBS   = -L$(PREFIX)/lib -lsigrok4DSL -Wl,-rpath,$(PREFIX)/lib

CFLAGS += $(shell pkg-config --cflags $(PKGS))
LIBS   += $(shell pkg-config --libs $(PKGS))

# Use vendored headers first, fallback to system path
THIRD_PARTY = ../third_party
CFLAGS += -I$(THIRD_PARTY)/libsigrok4DSL -I$(PREFIX)/include/libsigrok4DSL

SRC = dslogic-cli.c
OUT = dslogic-cli

.PHONY: all install clean version

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

install: $(OUT)
	install -m 755 $(OUT) $(DESTDIR)$(BINDIR)/$(OUT)

clean:
	rm -f $(OUT)

version:
	@echo $(VERSION)
