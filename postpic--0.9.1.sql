-- postpic--0.9.1.sql
-- SQL wrappers for postpic extension (EPAS 17)
-- Uses MODULE_PATHNAME to call C functions in the shared library.

-- Loading the extension will create these functions.

-- I/O
CREATE FUNCTION postpic_read(path text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_read'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_write(img bytea, path text, format text DEFAULT 'jpeg')
RETURNS void
AS 'MODULE_PATHNAME', 'postpic_write'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_from_file(path text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_from_file'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_to_file(img bytea, path text, format text DEFAULT 'jpeg')
RETURNS void
AS 'MODULE_PATHNAME', 'postpic_to_file'
LANGUAGE C STRICT;

-- Info
CREATE FUNCTION postpic_info(img bytea)
RETURNS text
AS 'MODULE_PATHNAME', 'postpic_info'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_get_format(img bytea)
RETURNS text
AS 'MODULE_PATHNAME', 'postpic_get_format'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_get_width(img bytea)
RETURNS integer
AS 'MODULE_PATHNAME', 'postpic_get_width'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_get_height(img bytea)
RETURNS integer
AS 'MODULE_PATHNAME', 'postpic_get_height'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_version()
RETURNS text
AS 'MODULE_PATHNAME', 'postpic_version'
LANGUAGE C STRICT;

-- Basic transforms
CREATE FUNCTION postpic_resize(img bytea, width integer, height integer, keep_aspect boolean DEFAULT true)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_resize'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_thumbnail(img bytea, max_width integer, max_height integer)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_thumbnail'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_crop(img bytea, x integer, y integer, width integer, height integer)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_crop'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_rotate(img bytea, degrees double precision)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_rotate'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_flip_horizontal(img bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_flip_horizontal'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_flip_vertical(img bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_flip_vertical'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_flip(img bytea, mode text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_flip'
LANGUAGE C STRICT;

-- Color and filters
CREATE FUNCTION postpic_grayscale(img bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_grayscale'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_brightness_contrast(img bytea, brightness integer, contrast integer)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_brightness_contrast'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_blur(img bytea, radius double precision, sigma double precision)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_blur'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_sharpen(img bytea, radius double precision, sigma double precision)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_sharpen'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_colorize(img bytea, red integer, green integer, blue integer)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_colorize'
LANGUAGE C STRICT;

-- Conversion / metadata
CREATE FUNCTION postpic_convert(img bytea, format text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_convert'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_strip_metadata(img bytea)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_strip_metadata'
LANGUAGE C STRICT;

-- Advanced
CREATE FUNCTION postpic_composite(base_img bytea, overlay_img bytea, x integer, y integer, opacity double precision DEFAULT 1.0)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_composite'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_resize_and_crop(img bytea, target_width integer, target_height integer)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_resize_and_crop'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_pad(img bytea, target_width integer, target_height integer, bgcolor text DEFAULT 'white')
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_pad'
LANGUAGE C STRICT;

-- Utilities
CREATE FUNCTION postpic_supported_formats()
RETURNS text[]
AS 'MODULE_PATHNAME', 'postpic_supported_formats'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_validate(img bytea)
RETURNS boolean
AS 'MODULE_PATHNAME', 'postpic_validate'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_memory_usage()
RETURNS text
AS 'MODULE_PATHNAME', 'postpic_memory_usage'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_free(img bytea)
RETURNS void
AS 'MODULE_PATHNAME', 'postpic_free'
LANGUAGE C STRICT;

-- Backwards-compatible aliases
CREATE FUNCTION postpic_load(path text)
RETURNS bytea
AS 'MODULE_PATHNAME', 'postpic_from_file'
LANGUAGE C STRICT;

CREATE FUNCTION postpic_save(img bytea, path text)
RETURNS void
AS 'MODULE_PATHNAME', 'postpic_to_file'
LANGUAGE C STRICT;
