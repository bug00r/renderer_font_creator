#include "rft_converter.h"

char EMPTY_ENTRY;

//struct for printcache
typedef struct {
	char *fontname;
	char *fontfilename;
	char *familyname;
	FT_ULong startCode;
	FT_ULong endCode;
	FT_ULong cntCodes;
	FILE *out_file;
	int emptyWasSet;
} rft_pcache_t;

void __rft_pcache_free(rft_pcache_t *cache) {
	free(cache->fontname);
	free(cache->fontfilename);
	free(cache->familyname);
}

void __rft_pcache_init(rft_pcache_t *cache) {
	cache->fontname = NULL;
	cache->fontfilename = NULL;
	cache->familyname = NULL;
	cache->emptyWasSet = 0;
	cache->out_file = NULL;
	cache->startCode = 0UL;
	cache->endCode = 0UL;
	cache->cntCodes = 0UL;
}

char * __rft_format_string_new(const char * msg, ...) {
	va_list vl;
	va_start(vl, msg);
	int buffsize = vsnprintf(NULL, 0, msg, vl);
	va_end(vl);
	buffsize += 1;
	char * buffer = malloc(buffsize);
	va_start(vl, msg);
	vsnprintf(buffer, buffsize, msg, vl);
	va_end( vl);
	return buffer;
}

void __rft_replace_string(char * string, char src, char replace) {

	char *str = string;
	int len = strlen(str);
	for ( int curChrIdx = 0; curChrIdx < len; curChrIdx++ ) {
		if ( str[curChrIdx] == src ) str[curChrIdx] = replace;
	}
}

void __rft_create_font_name_test(rft_pcache_t *cache, const char *family_name, int size) {
	cache->familyname = __rft_format_string_new("%s", family_name);
	__rft_replace_string(cache->familyname, ' ', '_');
	cache->fontname = __rft_format_string_new("%s%s%i", cache->familyname, "TestStyle", size);
}

void __rft_create_font_name(rft_pcache_t *cache, FT_Face font_face, int size) {
	cache->familyname = __rft_format_string_new("%s", font_face->family_name);
	__rft_replace_string(cache->familyname, ' ', '_');
	cache->fontname = __rft_format_string_new("%s%s%i", cache->familyname, font_face->style_name, size);
}

void __rft_create_font_file_name(rft_pcache_t *cache) {
	cache->fontfilename = __rft_format_string_new("font_%s.c", cache->fontname);
}

void __rft_print_header(rft_pcache_t *cache) {
	fprintf(cache->out_file, "// generated by %s\n//font(s): %s\n\n", __FILE__, cache->fontname);
	fprintf(cache->out_file,"//including renderer header\n#include \"vec.h\"\n\n");
}

void __rft_set_charcode_range(rft_pcache_t *_cache, FT_ULong startCharcode, FT_ULong endCharcode) {
	rft_pcache_t *cache = _cache;
	cache->startCode = startCharcode;
	cache->endCode = endCharcode;
	cache->cntCodes = endCharcode - startCharcode + 1; // +1 including last Sign
}

int __rft_move_to( const FT_Vector*  to, void* user ) 
{
	printf("move_to (x/y): %ld/%ld\n", to->x, to->y);
	return 0;	
}

int __rft_line_to( const FT_Vector*  to, void* user ) 
{
	printf("line_to (x/y): %ld/%ld\n", to->x, to->y);
	return 0;
}

int __rft_conic_to( const FT_Vector*  control, const FT_Vector*  to,void* user ) 
{
	printf("conic_to: ctrl_p  (x/y): %ld/%ld ", control->x, control->y);
	printf("to (x/y): %ld/%ld\n", to->x, to->y);
	return 0;
}

int __rft_cubic_to( const FT_Vector*  control1, const FT_Vector*  control2, const FT_Vector*  to, void* user ) 
{
	printf("conic_to: ctrl_p 1  (x/y): %ld/%ld ", control1->x, control1->y);
	printf("ctrl_p 2  (x/y): %ld/%ld ", control2->x, control2->y);
	printf("to (x/y): %ld/%ld\n", to->x, to->y);
	return 0;
}


void __rft_process_charcode(rft_conv_param_t* params, FT_Library  _library, FT_Face _face, int hPixel, rft_pcache_t *pCache, FT_ULong charcode) {

	FT_Error error;
	FT_Library  library = _library;
	FT_Face face = _face;
	//printf("searching charcode: %lx\n", charcode);
	FT_UInt glyphIdx = FT_Get_Char_Index(face, charcode);

	if ( glyphIdx == 0 ) {
		printf("%u not found :(\n", glyphIdx);
	} else {
		//printf("%lx found at idx %i...loading Glyph\n", charcode, glyphIdx);
		error = FT_Load_Glyph(face, glyphIdx, FT_LOAD_DEFAULT );
		if ( error ) {
			printf("something went wrong during loading: %x\n", error);
		} else {
			//printf("glyph loaded ok\n");
			/* not rendered, render it now */
			FT_GlyphSlot glyphSlot = face->glyph;
			//if ( glyphSlot->format != FT_GLYPH_FORMAT_BITMAP ) {
			
			if ( params->outline ) {
				printf("Outline Prcessing:\n");
				const FT_Outline_Funcs _outline_fns = { __rft_move_to, __rft_line_to, __rft_conic_to, __rft_cubic_to,0,0};
				FT_BBox abbox;
				error = FT_Outline_Get_BBox( &glyphSlot->outline, &abbox );

				if ( error ) {
					printf("OUTLINE BBOX ERROR: %x\n", error);
				} else {
					printf("BBOX: xmin/ymin xmax/ymax: %ld/%ld %ld/%ld \n", 
							abbox.xMin, abbox.yMin, abbox.xMax, abbox.yMax);
				}

				error = FT_Outline_Decompose( &glyphSlot->outline, /* FT_Outline*              outline,*/
                        					  &_outline_fns, /*const FT_Outline_Funcs*  func_interface,*/
                        					  NULL /*void* user*/ );
				if ( error ) {
					printf("OUTLINE DECOMPOSE ERROR: %x\n", error);
				}
			}
			
			FT_Glyph glyph;
			FT_Get_Glyph(glyphSlot, &glyph);
			FT_Done_Glyph(glyph);
		}
	}
}

static void __rft_process_font_face(rft_conv_param_t* params, FT_Library  library, rft_pcache_t *pCache, FT_ULong charcode) {

	FT_Error error;

	rft_conv_font_param_t* font_param = rft_conv_font_by_charcode(params, charcode);

	if ( font_param == NULL ) {
		//__rft_print_byte_array_and_set_fi_entry_empty(params, pCache, params->hPixel, 1, 8, charcode);		
		return;
	}

	FT_Face face;
	error = FT_New_Face( library, font_param->fontfile, font_param->faceIndex, &face);
	
	if ( error > 0 )
	{
		fprintf(stderr, "FontFace error: %s\n", FT_Error_String(error));
	} else {
		//printf("loaded:\nfamily: %s\nstyle: %s\nglyphs: %li\n", face->family_name, face->style_name, face->num_glyphs);

		int hPixel = params->hPixel;

		/* INFO: SET CHARSIZE */
		error = FT_Set_Pixel_Sizes(
							face,   /* handle to face object */
							0,      /* pixel_width           */
							hPixel );   /* pixel_height          */
		
		if ( error > 0 ) {
			fprintf(stderr, "Set Pixel error: %s\n", FT_Error_String(error));
		}

		//FT_ULong startCharcode = 0x4E00;
		//FT_ULong charcode = 0x4E1F;
		//FT_ULong charcode = 0x4E01;
		//FT_ULong charcode = 0x4FF7;
		//FT_ULong startCharcode = 0x4E20;
		//FT_ULong charcode = 0x706C;
		//FT_ULong endCharcode = 0x4E2C;
		//FT_ULong endCharcode = 0x9FA5;
		
		//for ( FT_ULong curCharcode = pCache->startCode; curCharcode <= pCache->endCode; curCharcode++ ) {
		__rft_process_charcode(params, library, face, hPixel, pCache, charcode);
		//}

		//old pCache cleanup

		FT_Done_Face(face);
	}

}

void convert( rft_conv_param_t* params )
{
	printf("init font converter\n");
	printf("verbose: %i \n", params->verbose);
    printf("help: %i \n", params->showHelp);
    printf("hex: %i \n", params->hex);
    printf("minCharcode: %lu \n", params->minCharcode);
    printf("maxCharcode: %lu \n", params->maxCharcode);
    printf("hPixel: %i \n", params->hPixel);
    printf("#fonts: %i \n", params->cntFonts);

	int hPixel = params->hPixel;

	FT_Library  library;


	FT_Error error = FT_Init_FreeType( &library );
	if ( error > 0 )
	{
		fprintf(stderr, "Init FT error: %s\n", FT_Error_String(error));
	} else {
		
		rft_pcache_t *pCache = malloc(sizeof(rft_pcache_t));
		__rft_pcache_init(pCache);
		__rft_create_font_name_test(pCache,"MyChineseMix", hPixel);

		__rft_create_font_file_name(pCache);

		pCache->out_file = fopen(pCache->fontfilename, "w");

		__rft_print_header(pCache);

		__rft_set_charcode_range(pCache, params->minCharcode, params->maxCharcode);

		for ( FT_ULong curCharcode = params->minCharcode; curCharcode <= params->maxCharcode ; curCharcode++ ) {
			__rft_process_font_face(params, library, pCache, curCharcode);
		}

		fclose(pCache->out_file);

		__rft_pcache_free(pCache);

		free(pCache);

		FT_Done_FreeType(library);
	}
}
