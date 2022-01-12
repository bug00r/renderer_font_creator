#ifndef RFT_CONVERTER_H
#define RFT_CONVERTER_H

#include <stdint.h>
#include <stdio.h>
#include <getopt.h>
#include <limits.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "freetype/ftbitmap.h"
#include "freetype/ftglyph.h"
#include "freetype/ftoutln.h"
#include "freetype/ftbbox.h"

#include "rft_conv_param_builder.h"
#include "vec.h"
#include "geometry.h"
#include "dl_list.h"

void convert( rft_conv_param_t* params );

#endif
