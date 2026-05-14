# edb-postpic

edb-postpic modernizes and adapts the original `postpic` project to work with EDB Postgres Advanced Server 17 and GraphicsMagick MagickWand.

## Description
This repository contains the source code and a Makefile to build the `edb-postpic` extension/tool that enables image processing from Postgres using GraphicsMagick MagickWand.

## Dependencies
**Required**
- EDB Postgres Advanced Server 17 or a compatible PostgreSQL distribution
- GraphicsMagick 1.3.42 with MagickWand
- pkg-config
- A C compiler such as gcc or clang

**Optional**
- libxml2, zlib, and other libraries depending on your configuration

## Installation example
```bash
# set GraphicsMagick location if needed
export MAGICK_HOME=/usr/local
export PKG_CONFIG_PATH=$MAGICK_HOME/lib/pkgconfig:$PKG_CONFIG_PATH

# build
make

# install example
sudo make install

