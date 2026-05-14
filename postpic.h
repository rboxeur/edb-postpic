/* postpic.h - header for postpic extension
 * Compatible with ISO C90/C99 and Postgres server extension API.
 */

#ifndef POSTPIC_H
#define POSTPIC_H

#include "postgres.h"
#include "fmgr.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/* Exported function declarations (V1) */
extern Datum postpic_read(PG_FUNCTION_ARGS);
extern Datum postpic_write(PG_FUNCTION_ARGS);
extern Datum postpic_from_file(PG_FUNCTION_ARGS);
extern Datum postpic_to_file(PG_FUNCTION_ARGS);
extern Datum postpic_info(PG_FUNCTION_ARGS);
extern Datum postpic_get_format(PG_FUNCTION_ARGS);
extern Datum postpic_get_width(PG_FUNCTION_ARGS);
extern Datum postpic_get_height(PG_FUNCTION_ARGS);
extern Datum postpic_version(PG_FUNCTION_ARGS);

extern Datum postpic_resize(PG_FUNCTION_ARGS);
extern Datum postpic_thumbnail(PG_FUNCTION_ARGS);
extern Datum postpic_crop(PG_FUNCTION_ARGS);
extern Datum postpic_rotate(PG_FUNCTION_ARGS);
extern Datum postpic_flip_horizontal(PG_FUNCTION_ARGS);
extern Datum postpic_flip_vertical(PG_FUNCTION_ARGS);
extern Datum postpic_flip(PG_FUNCTION_ARGS);

extern Datum postpic_grayscale(PG_FUNCTION_ARGS);
extern Datum postpic_brightness_contrast(PG_FUNCTION_ARGS);
extern Datum postpic_blur(PG_FUNCTION_ARGS);
extern Datum postpic_sharpen(PG_FUNCTION_ARGS);
extern Datum postpic_colorize(PG_FUNCTION_ARGS);

extern Datum postpic_convert(PG_FUNCTION_ARGS);
extern Datum postpic_strip_metadata(PG_FUNCTION_ARGS);

extern Datum postpic_composite(PG_FUNCTION_ARGS);
extern Datum postpic_resize_and_crop(PG_FUNCTION_ARGS);
extern Datum postpic_pad(PG_FUNCTION_ARGS);

extern Datum postpic_supported_formats(PG_FUNCTION_ARGS);
extern Datum postpic_validate(PG_FUNCTION_ARGS);
extern Datum postpic_memory_usage(PG_FUNCTION_ARGS);
extern Datum postpic_free(PG_FUNCTION_ARGS);

#endif /* POSTPIC_H */
