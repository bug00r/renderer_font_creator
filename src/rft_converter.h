#ifndef RFT_CONVERTER_H
#define RFT_CONVERTER_H

#include <stdint.h>
#include <stdio.h>
#include <getopt.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "freetype/ftbitmap.h"
#include "freetype/ftglyph.h"

#include "rft_conv_param_builder.h"

void convert( rft_conv_param_t* params );

#endif
