# edb-postpic

edb-postpic modernise et adapte le projet original `postpic` pour EDB Postgres Advanced Server 17 et GraphicsMagick (MagickWand).

## Description
Ce dépôt contient les sources et le Makefile pour construire l'extension/outil `edb-postpic` qui utilise MagickWand (GraphicsMagick) pour le traitement d'images depuis Postgres.

## Dépendances
- **EDB Postgres Advanced Server 17** (ou PostgreSQL compatible)
- **GraphicsMagick 1.3.42** (MagickWand)
- Outils de compilation : `gcc`/`clang`, `make`, `pkg-config`
- Autres libs : `libxml2`, `zlib` (selon configuration)

## Installation (exemple)
```bash
# définir l'emplacement de GraphicsMagick si nécessaire
export MAGICK_HOME=/usr/local
export PKG_CONFIG_PATH=$MAGICK_HOME/lib/pkgconfig:$PKG_CONFIG_PATH

# build
make
# installer (exemple)
sudo make install
