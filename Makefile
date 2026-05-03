PLUGIN = midi-pitchbend-cc-tester
SO  = $(PLUGIN).so
SRC = $(PLUGIN).c
CC = gcc
CFLAGS  = -Wall -Wextra -O2 -fPIC $(shell pkg-config --cflags lv2)
LDFLAGS = -shared $(shell pkg-config --libs lv2) -lm
PREFIX ?= $(HOME)/.lv2
PINSDIR ?= $(PREFIX)/$(PLUGIN).lv2

all: $(SO)

$(SO): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

install: $(SO)
	rm -rf $(PINSDIR)
	mkdir -p $(PINSDIR)
	cp $(SO) manifest.ttl $(PLUGIN).ttl $(PINSDIR)/

clean:
	rm -f $(SO)