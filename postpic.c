/* postpic.c - corrected for ImageMagick / Postgres include conflicts
 * Replace your existing src/postpic.c with this file and recompile.
 *
 * Key fixes:
 *  - include order: Postgres headers first
 *  - avoid StringInfo conflict by renaming before including Magick headers
 *  - include utils/array.h and lib/stringinfo.h for construct_array and StringInfo
 *  - fix string lowercasing and thumbnail wrapper
 */

#include "postpic.h"

/* Postgres headers first (ensure all macros/types are visible) */
#include "postgres.h"
#include "fmgr.h"
#include "utils/varlena.h"
#include "utils/bytea.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "lib/stringinfo.h"
#include "catalog/pg_type.h"

/* Now avoid ImageMagick's StringInfo typedef colliding with Postgres.
   Rename ImageMagick's StringInfo symbol during include. */
#define StringInfo Magick_StringInfo_conflict_avoidance
#include <wand/MagickWand.h>
#undef StringInfo

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Ensure MagickWand is initialized once per process */
static void
ensure_magick_initialized(void)
{
    static int initialized = 0;
    if (!initialized)
    {
        MagickWandGenesis();
        initialized = 1;
    }
}

/* Helper: read bytea into MagickWand */
static MagickWand *
wand_from_bytea(bytea *data)
{
    MagickWand *wand;
    size_t len;
    unsigned char *buf;

    ensure_magick_initialized();

    if (data == NULL)
        ereport(ERROR, (errmsg("null image data")));

    len = VARSIZE_ANY_EXHDR(data);
    buf = (unsigned char *) VARDATA_ANY(data);

    wand = NewMagickWand();
    if (MagickReadImageBlob(wand, (const void *) buf, len) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to read image blob (unsupported or corrupted)")));
    }
    return wand;
}

/* Helper: write MagickWand to bytea in given format (jpeg/png) */
static bytea *
bytea_from_wand(MagickWand *wand, const char *format)
{
    unsigned char *blob = NULL;
    size_t length = 0;
    bytea *result;
    char fmt_lower[16];

    if (format == NULL)
        format = "jpeg";

    /* normalize to lowercase safely */
    strncpy(fmt_lower, format, sizeof(fmt_lower)-1);
    fmt_lower[sizeof(fmt_lower)-1] = '\0';
    {
        char *p = fmt_lower;
        while (*p) { *p = (char) tolower((unsigned char)*p); p++; }
    }

    if (strcmp(fmt_lower, "jpeg") == 0 || strcmp(fmt_lower, "jpg") == 0)
        MagickSetImageFormat(wand, "JPEG");
    else if (strcmp(fmt_lower, "png") == 0)
        MagickSetImageFormat(wand, "PNG");
    else
        ereport(ERROR, (errmsg("unsupported output format: %s (only jpeg and png allowed)", format)));

    blob = MagickGetImageBlob(wand, &length);
    if (blob == NULL || length == 0)
        ereport(ERROR, (errmsg("failed to write image blob")));

    /* allocate bytea */
    result = (bytea *) palloc(VARHDRSZ + length);
    SET_VARSIZE(result, VARHDRSZ + length);
    memcpy(VARDATA(result), blob, length);

    /* free Magick blob memory */
    MagickRelinquishMemory(blob);

    return result;
}

/* Helper: read file into MagickWand */
static MagickWand *
wand_from_file(const char *path)
{
    MagickWand *wand;
    ensure_magick_initialized();
    wand = NewMagickWand();
    if (MagickReadImage(wand, path) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to read image file: %s", path)));
    }
    return wand;
}

/* Helper: write MagickWand to file with format */
static void
wand_to_file(MagickWand *wand, const char *path, const char *format)
{
    char fmt_lower[16];
    const char *fmt = format ? format : "jpeg";

    /* normalize */
    strncpy(fmt_lower, fmt, sizeof(fmt_lower)-1);
    fmt_lower[sizeof(fmt_lower)-1] = '\0';
    {
        char *p = fmt_lower;
        while (*p) { *p = (char) tolower((unsigned char)*p); p++; }
    }

    if (strcmp(fmt_lower, "png") == 0)
    {
        if (MagickSetImageFormat(wand, "PNG") == MagickFalse)
            ereport(ERROR, (errmsg("failed to set image format for writing")));
    }
    else /* default to JPEG */
    {
        if (MagickSetImageFormat(wand, "JPEG") == MagickFalse)
            ereport(ERROR, (errmsg("failed to set image format for writing")));
    }

    if (MagickWriteImage(wand, path) == MagickFalse)
        ereport(ERROR, (errmsg("failed to write image to file: %s", path)));
}

/* --- Fonctions Postgres (extraits et corrigés) --- */

/* Déclarez PG_FUNCTION_INFO_V1 pour chaque fonction utilisée */
PG_FUNCTION_INFO_V1(postpic_get_width);
Datum
postpic_get_width(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    size_t w = MagickGetImageWidth(wand);
    DestroyMagickWand(wand);
    PG_RETURN_INT32((int) w);
}

PG_FUNCTION_INFO_V1(postpic_get_height);
Datum
postpic_get_height(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    size_t h = MagickGetImageHeight(wand);
    DestroyMagickWand(wand);
    PG_RETURN_INT32((int) h);
}

PG_FUNCTION_INFO_V1(postpic_get_format);
Datum
postpic_get_format(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    char *format = MagickGetImageFormat(wand);
    text *t = cstring_to_text(format ? format : "unknown");
    if (format) MagickRelinquishMemory(format);
    DestroyMagickWand(wand);
    PG_RETURN_TEXT_P(t);
}

PG_FUNCTION_INFO_V1(postpic_version);
Datum
postpic_version(PG_FUNCTION_ARGS)
{
    PG_RETURN_TEXT_P(cstring_to_text("postpic 0.9.1 (GraphicsMagick 1.3.42 compatible)"));
}

/* I/O */
PG_FUNCTION_INFO_V1(postpic_from_file);
Datum
postpic_from_file(PG_FUNCTION_ARGS)
{
    text *path_text = PG_GETARG_TEXT_PP(0);
    char *path = text_to_cstring(path_text);
    MagickWand *wand = wand_from_file(path);
    bytea *out = bytea_from_wand(wand, "jpeg"); /* default to jpeg blob */
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

PG_FUNCTION_INFO_V1(postpic_to_file);
Datum
postpic_to_file(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    text *path_text = PG_GETARG_TEXT_PP(1);
    char *path = text_to_cstring(path_text);
    char *format = NULL;
    if (PG_NARGS() >= 3 && !PG_ARGISNULL(2))
        format = text_to_cstring(PG_GETARG_TEXT_PP(2));
    MagickWand *wand = wand_from_bytea(img);
    wand_to_file(wand, path, format);
    DestroyMagickWand(wand);
    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(postpic_read);
Datum
postpic_read(PG_FUNCTION_ARGS)
{
    return postpic_from_file(fcinfo);
}

PG_FUNCTION_INFO_V1(postpic_write);
Datum
postpic_write(PG_FUNCTION_ARGS)
{
    return postpic_to_file(fcinfo);
}

/* Info */
PG_FUNCTION_INFO_V1(postpic_info);
Datum
postpic_info(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    size_t w = MagickGetImageWidth(wand);
    size_t h = MagickGetImageHeight(wand);
    char *format = MagickGetImageFormat(wand);
    StringInfoData buf;
    initStringInfo(&buf);
    appendStringInfo(&buf, "{\"width\":%u,\"height\":%u,\"format\":\"%s\"}", (unsigned)w, (unsigned)h, format ? format : "unknown");
    if (format) MagickRelinquishMemory(format);
    DestroyMagickWand(wand);
    PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}

/* Resize */
PG_FUNCTION_INFO_V1(postpic_resize);
Datum
postpic_resize(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    int width = PG_GETARG_INT32(1);
    int height = PG_GETARG_INT32(2);
    bool keep_aspect = PG_NARGS() >= 4 ? PG_GETARG_BOOL(3) : true;

    if (width <= 0 || height <= 0)
        ereport(ERROR, (errmsg("width and height must be positive")));

    MagickWand *wand = wand_from_bytea(img);
    size_t orig_w = MagickGetImageWidth(wand);
    size_t orig_h = MagickGetImageHeight(wand);

    if (keep_aspect)
    {
        double rx = (double) width / (double) orig_w;
        double ry = (double) height / (double) orig_h;
        double r = (rx < ry) ? rx : ry;
        width = (int) (orig_w * r + 0.5);
        height = (int) (orig_h * r + 0.5);
    }

    if (MagickResizeImage(wand, (size_t) width, (size_t) height, LanczosFilter, 1.0) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to resize image")));
    }

    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Thumbnail: explicit wrapper that calls resize with keep_aspect = true */
PG_FUNCTION_INFO_V1(postpic_thumbnail);
Datum
postpic_thumbnail(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    int maxw = PG_GETARG_INT32(1);
    int maxh = PG_GETARG_INT32(2);

    /* call postpic_resize with keep_aspect = true */
    /* Build a new call frame: reuse existing fcinfo by calling resize implementation directly */
    /* Simpler: implement logic here */
    MagickWand *wand = wand_from_bytea(img);
    size_t orig_w = MagickGetImageWidth(wand);
    size_t orig_h = MagickGetImageHeight(wand);

    if (maxw <= 0 || maxh <= 0)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("max width/height must be positive")));
    }

    double rx = (double) maxw / (double) orig_w;
    double ry = (double) maxh / (double) orig_h;
    double r = (rx < ry) ? rx : ry;
    int new_w = (int) (orig_w * r + 0.5);
    int new_h = (int) (orig_h * r + 0.5);

    if (MagickResizeImage(wand, (size_t) new_w, (size_t) new_h, LanczosFilter, 1.0) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to create thumbnail")));
    }

    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Crop */
PG_FUNCTION_INFO_V1(postpic_crop);
Datum
postpic_crop(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    int x = PG_GETARG_INT32(1);
    int y = PG_GETARG_INT32(2);
    int w = PG_GETARG_INT32(3);
    int h = PG_GETARG_INT32(4);

    if (w <= 0 || h <= 0)
        ereport(ERROR, (errmsg("crop width/height must be positive")));

    MagickWand *wand = wand_from_bytea(img);
    if (MagickCropImage(wand, (size_t) w, (size_t) h, x, y) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to crop image")));
    }
    MagickResetIterator(wand);
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Rotate */
PG_FUNCTION_INFO_V1(postpic_rotate);
Datum
postpic_rotate(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    double degrees = PG_GETARG_FLOAT8(1);

    MagickWand *wand = wand_from_bytea(img);
    PixelWand *bg = NewPixelWand();
    PixelSetColor(bg, "none");
    if (MagickRotateImage(wand, bg, degrees) == MagickFalse)
    {
        DestroyPixelWand(bg);
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to rotate image")));
    }
    DestroyPixelWand(bg);
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Flip horizontal */
PG_FUNCTION_INFO_V1(postpic_flip_horizontal);
Datum
postpic_flip_horizontal(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    if (MagickFlopImage(wand) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to flip image horizontally")));
    }
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Flip vertical */
PG_FUNCTION_INFO_V1(postpic_flip_vertical);
Datum
postpic_flip_vertical(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    if (MagickFlipImage(wand) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to flip image vertically")));
    }
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Flip wrapper */
PG_FUNCTION_INFO_V1(postpic_flip);
Datum
postpic_flip(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    text *mode_text = PG_GETARG_TEXT_PP(1);
    char *mode = text_to_cstring(mode_text);

    if (pg_strcasecmp(mode, "horizontal") == 0)
        return postpic_flip_horizontal(fcinfo);
    else if (pg_strcasecmp(mode, "vertical") == 0)
        return postpic_flip_vertical(fcinfo);
    else
        ereport(ERROR, (errmsg("unknown flip mode: %s (use 'horizontal' or 'vertical')", mode)));

    PG_RETURN_NULL(); /* unreachable */
}

/* Grayscale */
PG_FUNCTION_INFO_V1(postpic_grayscale);
Datum
postpic_grayscale(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    if (MagickSetImageType(wand, GrayscaleType) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to convert to grayscale")));
    }
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Brightness/contrast */
PG_FUNCTION_INFO_V1(postpic_brightness_contrast);
Datum
postpic_brightness_contrast(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    int brightness = PG_GETARG_INT32(1);
    int contrast = PG_GETARG_INT32(2);

    MagickWand *wand = wand_from_bytea(img);
    if (MagickBrightnessContrastImage(wand, (double) brightness, (double) contrast) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to adjust brightness/contrast")));
    }
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Blur */
PG_FUNCTION_INFO_V1(postpic_blur);
Datum
postpic_blur(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    double radius = PG_GETARG_FLOAT8(1);
    double sigma = PG_GETARG_FLOAT8(2);

    MagickWand *wand = wand_from_bytea(img);
    if (MagickBlurImage(wand, radius, sigma) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to blur image")));
    }
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Sharpen */
PG_FUNCTION_INFO_V1(postpic_sharpen);
Datum
postpic_sharpen(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    double radius = PG_GETARG_FLOAT8(1);
    double sigma = PG_GETARG_FLOAT8(2);

    MagickWand *wand = wand_from_bytea(img);
    if (MagickSharpenImage(wand, radius, sigma) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to sharpen image")));
    }
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Colorize */
PG_FUNCTION_INFO_V1(postpic_colorize);
Datum
postpic_colorize(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    int red = PG_GETARG_INT32(1);
    int green = PG_GETARG_INT32(2);
    int blue = PG_GETARG_INT32(3);

    MagickWand *wand = wand_from_bytea(img);
    PixelWand *color = NewPixelWand();
    char colorstr[64];
    snprintf(colorstr, sizeof(colorstr), "rgb(%d,%d,%d)", red, green, blue);
    PixelSetColor(color, colorstr);
    if (MagickColorizeImage(wand, color, color) == MagickFalse)
    {
        DestroyPixelWand(color);
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to colorize image")));
    }
    DestroyPixelWand(color);
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Convert */
PG_FUNCTION_INFO_V1(postpic_convert);
Datum
postpic_convert(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    text *fmt_text = PG_GETARG_TEXT_PP(1);
    char *fmt = text_to_cstring(fmt_text);

    MagickWand *wand = wand_from_bytea(img);
    bytea *out = bytea_from_wand(wand, fmt);
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Strip metadata */
PG_FUNCTION_INFO_V1(postpic_strip_metadata);
Datum
postpic_strip_metadata(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = wand_from_bytea(img);
    if (MagickStripImage(wand) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to strip metadata")));
    }
    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Composite */
PG_FUNCTION_INFO_V1(postpic_composite);
Datum
postpic_composite(PG_FUNCTION_ARGS)
{
    bytea *base = PG_GETARG_BYTEA_P(0);
    bytea *overlay = PG_GETARG_BYTEA_P(1);
    int x = PG_GETARG_INT32(2);
    int y = PG_GETARG_INT32(3);
    double opacity = PG_NARGS() >= 5 ? PG_GETARG_FLOAT8(4) : 1.0;

    MagickWand *basew = wand_from_bytea(base);
    MagickWand *ovw = wand_from_bytea(overlay);

    if (opacity < 1.0)
    {
        if (MagickEvaluateImage(ovw, MultiplyEvaluateOperator, opacity) == MagickFalse)
        {
            DestroyMagickWand(basew);
            DestroyMagickWand(ovw);
            ereport(ERROR, (errmsg("failed to set overlay opacity")));
        }
    }

    if (MagickCompositeImage(basew, ovw, OverCompositeOp, x, y) == MagickFalse)
    {
        DestroyMagickWand(basew);
        DestroyMagickWand(ovw);
        ereport(ERROR, (errmsg("failed to composite images")));
    }

    bytea *out = bytea_from_wand(basew, "jpeg");
    DestroyMagickWand(basew);
    DestroyMagickWand(ovw);
    PG_RETURN_BYTEA_P(out);
}

/* Resize and crop */
PG_FUNCTION_INFO_V1(postpic_resize_and_crop);
Datum
postpic_resize_and_crop(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    int target_w = PG_GETARG_INT32(1);
    int target_h = PG_GETARG_INT32(2);

    if (target_w <= 0 || target_h <= 0)
        ereport(ERROR, (errmsg("target dimensions must be positive")));

    MagickWand *wand = wand_from_bytea(img);
    size_t orig_w = MagickGetImageWidth(wand);
    size_t orig_h = MagickGetImageHeight(wand);

    double rx = (double) target_w / (double) orig_w;
    double ry = (double) target_h / (double) orig_h;
    double r = (rx > ry) ? rx : ry; /* cover */
    int new_w = (int) (orig_w * r + 0.5);
    int new_h = (int) (orig_h * r + 0.5);

    if (MagickResizeImage(wand, (size_t) new_w, (size_t) new_h, LanczosFilter, 1.0) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to resize for resize_and_crop")));
    }

    int x = (new_w - target_w) / 2;
    int y = (new_h - target_h) / 2;

    if (MagickCropImage(wand, (size_t) target_w, (size_t) target_h, x, y) == MagickFalse)
    {
        DestroyMagickWand(wand);
        ereport(ERROR, (errmsg("failed to crop for resize_and_crop")));
    }

    bytea *out = bytea_from_wand(wand, "jpeg");
    DestroyMagickWand(wand);
    PG_RETURN_BYTEA_P(out);
}

/* Pad */
PG_FUNCTION_INFO_V1(postpic_pad);
Datum
postpic_pad(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    int target_w = PG_GETARG_INT32(1);
    int target_h = PG_GETARG_INT32(2);
    text *bg_text = PG_NARGS() >= 4 && !PG_ARGISNULL(3) ? PG_GETARG_TEXT_PP(3) : NULL;
    char *bg = bg_text ? text_to_cstring(bg_text) : "white";

    if (target_w <= 0 || target_h <= 0)
        ereport(ERROR, (errmsg("target dimensions must be positive")));

    MagickWand *wand = wand_from_bytea(img);
    size_t orig_w = MagickGetImageWidth(wand);
    size_t orig_h = MagickGetImageHeight(wand);

    int x = (target_w - (int)orig_w) / 2;
    int y = (target_h - (int)orig_h) / 2;

    PixelWand *bgpw = NewPixelWand();
    PixelSetColor(bgpw, bg);

    MagickWand *canvas = NewMagickWand();
    if (MagickNewImage(canvas, (size_t) target_w, (size_t) target_h, bgpw) == MagickFalse)
    {
        DestroyPixelWand(bgpw);
        DestroyMagickWand(wand);
        DestroyMagickWand(canvas);
        ereport(ERROR, (errmsg("failed to create background canvas")));
    }

    if (MagickCompositeImage(canvas, wand, OverCompositeOp, x, y) == MagickFalse)
    {
        DestroyPixelWand(bgpw);
        DestroyMagickWand(wand);
        DestroyMagickWand(canvas);
        ereport(ERROR, (errmsg("failed to composite onto background")));
    }

    bytea *out = bytea_from_wand(canvas, "jpeg");
    DestroyPixelWand(bgpw);
    DestroyMagickWand(wand);
    DestroyMagickWand(canvas);
    PG_RETURN_BYTEA_P(out);
}

/* Supported formats */
PG_FUNCTION_INFO_V1(postpic_supported_formats);
Datum
postpic_supported_formats(PG_FUNCTION_ARGS)
{
    Datum vals[2];
    ArrayType *arr;

    vals[0] = CStringGetTextDatum("jpeg");
    vals[1] = CStringGetTextDatum("png");

    arr = construct_array(vals, 2, TEXTOID, -1, false, 'i');
    PG_RETURN_ARRAYTYPE_P(arr);
}

/* Validate */
PG_FUNCTION_INFO_V1(postpic_validate);
Datum
postpic_validate(PG_FUNCTION_ARGS)
{
    bytea *img = PG_GETARG_BYTEA_P(0);
    MagickWand *wand = NULL;
    bool ok = true;
    PG_TRY();
    {
        wand = wand_from_bytea(img);
    }
    PG_CATCH();
    {
        FlushErrorState();
        ok = false;
    }
    PG_END_TRY();

    if (wand) DestroyMagickWand(wand);
    PG_RETURN_BOOL(ok);
}

/* Memory usage (placeholder) */
PG_FUNCTION_INFO_V1(postpic_memory_usage);
Datum
postpic_memory_usage(PG_FUNCTION_ARGS)
{
    PG_RETURN_TEXT_P(cstring_to_text("{\"magick_memory\":\"unknown\"}"));
}

/* Free (no-op) */
PG_FUNCTION_INFO_V1(postpic_free);
Datum
postpic_free(PG_FUNCTION_ARGS)
{
    PG_RETURN_VOID();
}

