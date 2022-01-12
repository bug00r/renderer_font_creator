#ifndef RFT_CONV_PARAM_BUILDER_H
#define RFT_CONV_PARAM_BUILDER_H

#include <getopt.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct __rft_conv_font_param {
    char *raw;
    char *fontfile;
    FT_ULong startCharcode;
    FT_ULong endCharcode;
    FT_Long faceIndex;
    struct __rft_font_param *next;
} rft_conv_font_param_t;

typedef struct 
{
    rft_conv_font_param_t *font_param;
    int showHelp;
    int verbose;
    int outline;
    int hex;
    char *name;
    FT_ULong minCharcode;
    FT_ULong maxCharcode;
    int hPixel;
    int cntFonts;
    struct option *long_options;
} rft_conv_param_t;

rft_conv_param_t* rft_conv_param_build(int argc, char* argv[]);

rft_conv_font_param_t* rft_conv_font_by_charcode(rft_conv_param_t *params, FT_ULong charcode);

void rft_conv_param_free(rft_conv_param_t **params);

#endif