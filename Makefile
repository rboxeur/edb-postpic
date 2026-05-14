# Makefile for postpic extension
MODULES = postpic
EXTENSION = postpic
DATA = postpic--0.9.1.sql
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

override SHLIB_LINK := $(shell pkg-config --keep-system-libs --libs MagickWand) -Wl,--export-dynamic
override LDFLAGS := $(shell pkg-config --keep-system-libs --libs MagickWand) -Wl,--export-dynamic

override PG_CPPFLAGS := $(shell pkg-config --keep-system-cflags --cflags MagickWand) 
override CFLAGS := $(shell pkg-config --keep-system-cflags --cflags MagickWand) -std=c99 -fPIC -fno-lto

include $(PGXS)
