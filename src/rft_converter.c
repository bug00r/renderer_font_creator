#include "rft_converter.h"

char EMPTY_ENTRY;

typedef struct {
	/* storing FT_Vector instances */
	dl_list_t *points;
} rft_outline_t;

typedef struct {
	FT_ULong charcode;
	FT_BBox bbox;
	/* storing rft_outline_t instance */
	dl_list_t *outline_list; 
} rft_outlines_t;

typedef struct {
	dl_list_t *outlines_list;
} rft_glyphs_t;

//struct for printcache
typedef struct {
	char *fontname;
	char *fontfilename;
	char *fontfilenameheader;
	char *familyname;
	FT_ULong startCode;
	FT_ULong endCode;
	FT_ULong cntCodes;
	FILE *out_file;
	int emptyWasSet;
	/* created outline data */
	rft_glyphs_t *glyphs;
	FT_Vector lastPoint;
	FT_BBox globalBbox;
	char *providername;
} rft_pcache_t;

static rft_glyphs_t* __rft_glyphs_new() 
{
	rft_glyphs_t *new_glyphs = malloc(sizeof(rft_glyphs_t));
	new_glyphs->outlines_list = dl_list_new();
	return new_glyphs;
} 

static void __rft_outline_free( rft_outline_t **outline )
{
	if ( outline != NULL && *outline != NULL )
	{
		rft_outline_t *to_delete = *outline; 
		dl_list_free(&to_delete->points);
		free(to_delete);
		*outline = NULL;
	}	
} 

static void __rft_outline_free_wrapper(void **data, void *eachdata)
{
	(void)(eachdata);

	rft_outline_t *outline = (rft_outline_t *)*data;
	__rft_outline_free(&outline);
}

static void __rft_outlines_free( rft_outlines_t **outlines )
{
	if ( outlines != NULL && *outlines != NULL )
	{
		rft_outlines_t *to_delete = *outlines; 

		if ( to_delete->outline_list != NULL )
		{
			dl_list_each_data(to_delete->outline_list, NULL, __rft_outline_free_wrapper);
		}

		free(to_delete);
		*outlines = NULL;
	}
	
}

static void __rft_outlines_free_wrapper(void **data, void *eachdata)
{
	(void)(eachdata);

	rft_outlines_t *outlines = (rft_outlines_t *)*data;
	__rft_outlines_free(&outlines);
}

static void __rft_glyphs_free( rft_glyphs_t **glyphs )
{
	if ( glyphs != NULL && *glyphs != NULL )
	{
		rft_glyphs_t *to_delete = *glyphs; 

		dl_list_each_data(to_delete->outlines_list, NULL, __rft_outlines_free_wrapper);

		free(to_delete);
		*glyphs = NULL;
	}
	
}

static rft_outlines_t* __rft_outlines_new(FT_ULong charcode, FT_BBox *bbox) 
{
	rft_outlines_t *new_outlines = malloc(sizeof(rft_outlines_t));
	new_outlines->outline_list = dl_list_new();
	new_outlines->charcode = charcode;
	new_outlines->bbox.xMin = bbox->xMin;
	new_outlines->bbox.yMin = bbox->yMin;
	new_outlines->bbox.xMax = bbox->xMax;
	new_outlines->bbox.yMax = bbox->yMax;
	return new_outlines;
}

static rft_outlines_t* __rft_outlines_empty(FT_ULong charcode) 
{
	rft_outlines_t *new_outlines = malloc(sizeof(rft_outlines_t));
	new_outlines->outline_list = NULL;
	new_outlines->charcode = charcode;
	new_outlines->bbox.xMin = 0;
	new_outlines->bbox.yMin = 0;
	new_outlines->bbox.xMax = 0;
	new_outlines->bbox.yMax = 0;
	return new_outlines;
}

static rft_outline_t* __rft_outline_new() 
{
	rft_outline_t *new_outline = malloc(sizeof(rft_outline_t));
	new_outline->points = dl_list_new();
	return new_outline;
} 

void __rft_pcache_free(rft_pcache_t *cache) {
	free(cache->fontname);
	free(cache->fontfilename);
	free(cache->fontfilenameheader);
	free(cache->familyname);
	__rft_glyphs_free(&cache->glyphs);
}

void __rft_pcache_init(rft_pcache_t *cache) {
	cache->fontname = NULL;
	cache->providername = NULL;
	cache->fontfilename = NULL;
	cache->fontfilenameheader = NULL;
	cache->familyname = NULL;
	cache->emptyWasSet = 0;
	cache->out_file = NULL;
	cache->startCode = 0UL;
	cache->endCode = 0UL;
	cache->cntCodes = 0UL;
	cache->globalBbox.xMin = LONG_MAX;
	cache->globalBbox.yMin = LONG_MAX;
	cache->globalBbox.xMax = LONG_MIN;
	cache->globalBbox.yMax = LONG_MIN;
	cache->glyphs = __rft_glyphs_new();
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

void __rft_create_font_file_name(rft_pcache_t *_cache) {
	rft_pcache_t *cache = _cache;
	cache->fontfilename = __rft_format_string_new("font_provider_%s.c", cache->providername);
	cache->fontfilenameheader = __rft_format_string_new("font_provider_%s.h", cache->providername);
}

void __rft_print_header(rft_pcache_t *cache) {
	fprintf(cache->out_file, "// generated by %s\n//font(s): %s\n\n", __FILE__, cache->fontname);
	fprintf(cache->out_file,"#include \"%s\"\n\n", cache->fontfilenameheader);
}

void __rft_print_glyph_not_found(rft_pcache_t *cache)
{
	FILE *out_file = cache->out_file;
	fprintf(out_file, "//NOT FOUND DEFAULT GLYPH\n");
	fprintf(out_file, "static vec2_t outlinePts_empty_0[5] = {{ 0.f, 0.f }, { 0.f, 100.f }, { 100.f, 100.f }, { 100.f, 0.f }, { 0.f, 0.f }};\n");
	fprintf(out_file, "static vec2_t outlinePts_empty_1[5] = {{ 10.f, 10.f }, { 90.f, 10.f }, { 90.f, 90.f }, { 10.f, 90.f }, { 10.f, 10.f }};\n");
	fprintf(out_file, "static rf_outlines_t glyph_empty_outlines[] = { { &outlinePts_empty_0[0], 5 }, { &outlinePts_empty_1[0], 5 }, { NULL, 0 }};\n");
	fprintf(out_file, "static rf_glyph_t __rf_glyph_empty = {{ 0, 0, 100, 100 }, &glyph_empty_outlines[0] };\n\n");
}

void __rft_print_glyph_container(rft_pcache_t *cache)
{
	FILE *out_file = cache->out_file;
	fprintf(out_file, "static rf_glyph_t* __rf_gl_container_get_fn(unsigned long charcode)\n{\n\trf_glyph_t* found = ( charcode <= %lu ? __rf_glyphs[charcode] : &__rf_glyph_empty);\n\n\tif( found == NULL )\n\t{\n\t\tfound = &__rf_glyph_empty;\n\t}\n\n\treturn found;\n}\n\n", cache->endCode);
	
	FT_BBox *bbox = &cache->globalBbox;
	
	fprintf(out_file, "static rf_glyph_container_t __rf_gl_container = { NULL, __rf_gl_container_get_fn, { %li, %li, %li, %li } };\n", 
				bbox->xMin, bbox->yMin, bbox->xMax, bbox->yMax);
}

void __rft_print_glyph_provider(rft_pcache_t *cache)
{
	FILE *out_file = cache->out_file;
	fprintf(out_file, "static rf_glyph_container_t* __rf_gl_provider_get_fn()\n{\n\treturn &__rf_gl_container;\n}\n\n");
	fprintf(out_file, "static void __rf_gl_provider_free_fn(rf_glyph_container_t** gl_container)\n{\n\t(void)(gl_container);\n}\n\n");
	fprintf(out_file, "static rf_provider_t __rf_gl_provider = { NULL, __rf_gl_provider_get_fn, __rf_gl_provider_free_fn };\n");
	fprintf(out_file, "rf_provider_t* get_%s_provider()\n{\n\treturn &__rf_gl_provider;\n}\n\n", cache->providername);
}

void __rft_set_charcode_range(rft_pcache_t *_cache, FT_ULong startCharcode, FT_ULong endCharcode) {
	rft_pcache_t *cache = _cache;
	cache->startCode = startCharcode;
	cache->endCode = endCharcode;
	cache->cntCodes = endCharcode - startCharcode + 1; // +1 including last Sign
}

static void __rft_add_new_outline(rft_pcache_t * _cache)
{
	rft_pcache_t *cache = _cache;
	rft_glyphs_t *glyphs = cache->glyphs;
	rft_outlines_t *outlines = (rft_outlines_t *)glyphs->outlines_list->last->data;

	dl_list_append(outlines->outline_list, __rft_outline_new());
}

static void __rft_add_new_outlines(rft_pcache_t * _cache, FT_ULong charcode, FT_BBox *bbox)
{
	rft_pcache_t *cache = _cache;
	rft_glyphs_t *glyphs = cache->glyphs;

	dl_list_append(glyphs->outlines_list, __rft_outlines_new(charcode, bbox));
}

static void __rft_add_empty_outlines(rft_pcache_t * _cache, FT_ULong charcode)
{
	rft_pcache_t *cache = _cache;
	rft_glyphs_t *glyphs = cache->glyphs;

	dl_list_append(glyphs->outlines_list, __rft_outlines_empty(charcode));
}

static void __rft_add_point(rft_pcache_t * _cache, const FT_Vector *point)
{
	rft_pcache_t *cache = _cache;
	rft_glyphs_t *glyphs = cache->glyphs;
	rft_outlines_t *outlines = (rft_outlines_t *)glyphs->outlines_list->last->data;
	rft_outline_t *outline = (rft_outline_t *)outlines->outline_list->last->data;

	FT_Vector *copy = malloc(sizeof(FT_Vector));

	copy->x = point->x;
	copy->y = point->y;

	dl_list_append(outline->points, copy);
}

static void __rft_add_bezier_point(rft_pcache_t * _cache, vec2_t const * const p2)
{
	rft_pcache_t *cache = _cache;

	FT_Vector point;
	point.x = (FT_Pos)p2->x;
	point.y = (FT_Pos)p2->y;

	__rft_add_point(cache, &point);
}

static void print_bezier(vec2_t const * const p, vec2_t const * const p2, void *data) {
	(void)(p);
	
	rft_pcache_t *cache = (rft_pcache_t *)data;

	__rft_add_bezier_point(cache, p2);

	//printf("b1:= x: %.0f, y: %.0f\n", (float)round_f(p2->x), (float)round_f(p2->y));
}

void __rft_set_last_pt(rft_pcache_t *_cache,const FT_Vector*  cur)
{
	rft_pcache_t *cache = _cache;
	FT_Vector *lastPoint = &cache->lastPoint;
	lastPoint->x = cur->x;
	lastPoint->y = cur->y;
}

int __rft_move_to( const FT_Vector*  to, void* data ) 
{
	rft_pcache_t *cache = (rft_pcache_t *)data;
	//printf("move_to (x/y): %ld/%ld\n", to->x, to->y);

	__rft_add_new_outline(cache);
	__rft_add_point(cache, to);
	__rft_set_last_pt(cache, to);

	return 0;	
}

int __rft_line_to( const FT_Vector*  to, void* data ) 
{
	rft_pcache_t *cache = (rft_pcache_t *)data;
	//printf("line_to (x/y): %ld/%ld\n", to->x, to->y);
	__rft_add_point(cache, to);
	__rft_set_last_pt(cache, to);
	return 0;
}

int __rft_conic_to( const FT_Vector*  control, const FT_Vector*  to, void* data ) 
{
	rft_pcache_t *cache = (rft_pcache_t *)data;
	FT_Vector *lastPoint = &cache->lastPoint;

	vec2_t start = { (float)lastPoint->x, (float)lastPoint->y };
	vec2_t cp = { (float)control->x, (float)control->y };
	vec2_t end = { (float)to->x, (float)to->y };
	__rft_set_last_pt(cache, to);
	uint32_t steps = 3;
	
	geometry_bezier1(&start, &cp, &end, &steps, print_bezier, cache);
	return 0;
}

int __rft_cubic_to( const FT_Vector*  control1, const FT_Vector*  control2, const FT_Vector*  to, void* data ) 
{

	rft_pcache_t *cache = (rft_pcache_t *)data;
	FT_Vector *lastPoint = &cache->lastPoint;

	vec2_t start = {  (float)lastPoint->x, (float)lastPoint->y };
	vec2_t cp1 = {  (float)control1->x, (float)control1->y };
	vec2_t cp2 = { (float)control2->x, (float)control2->y };
	vec2_t end = {  (float)to->x, (float)to->y };
	__rft_set_last_pt(cache, to);
	uint32_t steps = 3;
	
	geometry_bezier2(&start, &cp1, &cp2, &end, &steps, print_bezier, cache);

	return 0;
}

static void __rft_align_global_bbox(FT_BBox *globalbbox, FT_BBox *curbbox)
{
	globalbbox->xMin = ( curbbox->xMin < globalbbox->xMin ? curbbox->xMin : globalbbox->xMin);
	globalbbox->yMin = ( curbbox->yMin < globalbbox->yMin ? curbbox->yMin : globalbbox->yMin);
	globalbbox->xMax = ( curbbox->xMax > globalbbox->xMax ? curbbox->xMax : globalbbox->xMax);
	globalbbox->yMax = ( curbbox->yMax > globalbbox->yMax ? curbbox->yMax : globalbbox->yMax);
}

void __rft_process_charcode(rft_conv_param_t* params, FT_Face _face, rft_pcache_t *pCache, FT_ULong charcode) {

	FT_Error error;
	FT_Face face = _face;
	//printf("searching charcode: %lx\n", charcode);
	FT_UInt glyphIdx = FT_Get_Char_Index(face, charcode);

	if ( glyphIdx == 0 ) {
		//printf("%u not found :(\n", glyphIdx);
		__rft_add_empty_outlines(pCache, charcode);
	} else {
		//printf("%lx found at idx %i...loading Glyph\n", charcode, glyphIdx);
		error = FT_Load_Glyph(face, glyphIdx, FT_LOAD_DEFAULT );
		if ( error ) {
			printf("something went wrong during loading: %x\n", error);
			__rft_add_empty_outlines(pCache, charcode);
		} else {
			//printf("glyph loaded ok\n");
			/* not rendered, render it now */
			FT_GlyphSlot glyphSlot = face->glyph;
			//if ( glyphSlot->format != FT_GLYPH_FORMAT_BITMAP ) {
			
			if ( params->outline ) {
				const FT_Outline_Funcs _outline_fns = { __rft_move_to, __rft_line_to, __rft_conic_to, __rft_cubic_to,0,0};
				FT_BBox abbox;
				error = FT_Outline_Get_BBox( &glyphSlot->outline, &abbox );

				if ( error ) {
					printf("OUTLINE BBOX ERROR: %x\n", error);
				} else {
					__rft_align_global_bbox(&pCache->globalBbox, &abbox);
				}

				__rft_add_new_outlines(pCache, charcode, &abbox);

				error = FT_Outline_Decompose( &glyphSlot->outline, /* FT_Outline*              outline,*/
                        					  &_outline_fns, /*const FT_Outline_Funcs*  func_interface,*/
                        					  pCache /*void* user*/ );

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
		__rft_add_empty_outlines(pCache, charcode);		
		return;
	}

	FT_Face face;
	error = FT_New_Face( library, font_param->fontfile, font_param->faceIndex, &face);
	
	if ( error > 0 )
	{
		fprintf(stderr, "FontFace error: %s\n", FT_Error_String(error));
		__rft_add_empty_outlines(pCache, charcode);
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
			__rft_add_empty_outlines(pCache, charcode);	
		} else {
			__rft_process_charcode(params, face, pCache, charcode);
		}

		FT_Done_Face(face);
	}

}

typedef struct {
	FILE *out_file;
	size_t cnt;
	size_t cntPt;
	size_t maxPt;
	FT_ULong charcode;
} rft_outline_print_t;

static void __rft_print_outlines_declaration_record(void **data, void *eachdata)
{
	rft_outline_t *outline = (rft_outline_t *)*data;
	rft_outline_print_t *rft_op = (rft_outline_print_t *)eachdata;

	// { &outlinePts1_1[0], cntOutlinePts1_1}, 
	fprintf(rft_op->out_file, " { &outlinePts%lx_%lli[0], %u },",
		  rft_op->charcode, rft_op->cnt++, outline->points->cnt);

}

static void __rft_print_outlines_declaration(FILE *out_file, rft_outlines_t *outlines, rft_outline_print_t *_rft_op)
{
	dl_list_t *outline_list = outlines->outline_list;
	rft_outline_print_t *rft_op = _rft_op;

	fprintf(out_file, "static rf_outlines_t glyph_%lx_outlines[] = {", rft_op->charcode);

	dl_list_each_data(outline_list, rft_op, __rft_print_outlines_declaration_record);

	fprintf(out_file, " { NULL, 0 }};\n");

}

static void __rft_print_point(void **data, void *eachdata)
{
	FT_Vector *vector = (FT_Vector *)*data;
	rft_outline_print_t *rft_op = (rft_outline_print_t *)eachdata;
	
	//printf("{ %ld, %ld },", vector->x, vector->y);
    fprintf(rft_op->out_file, "{ %ld.f, %ld.f }", vector->x, vector->y);

	rft_op->cntPt++;
	if ( rft_op->cntPt < rft_op->maxPt )
	{
		fprintf(rft_op->out_file, ", ");
	}

}

static void __rft_print_outline(void **data, void *eachdata)
{
	rft_outline_t *outline = (rft_outline_t *)*data;
	rft_outline_print_t *rft_op = (rft_outline_print_t *)eachdata;
	dl_list_t *points = outline->points;

	fprintf(rft_op->out_file, "static vec2_t outlinePts%lx_%lli[%i] = {",
		  rft_op->charcode, rft_op->cnt++, points->cnt);

	rft_op->cntPt = 0;
	rft_op->maxPt = points->cnt;
	dl_list_each_data(points, rft_op, __rft_print_point);
	
	fprintf(rft_op->out_file, "};\n");
}

static void __rft_print_glyph_entry(FILE *_out_file, rft_outlines_t *_outlines)
{
	rft_outlines_t *outlines = _outlines;
	FT_BBox *bbox = &outlines->bbox;
	FILE *out_file = _out_file;

	fprintf(out_file, "static rf_glyph_t __rf_glyph_%lx = {{ %li, %li, %li, %li }, &glyph_%lx_outlines[0] };\n\n", 
			outlines->charcode, bbox->xMin, bbox->yMin, bbox->xMax, bbox->yMax, outlines->charcode);
}

static void __rft_print_outlines(void **data, void *eachdata)
{
	rft_outlines_t *outlines = (rft_outlines_t *)*data;
	FILE *out_file = (FILE *)eachdata;

	if ( outlines->outline_list != NULL )
	{
		rft_outline_print_t rft_op;
		rft_op.out_file = out_file;
		rft_op.cnt = 0;
		rft_op.charcode = outlines->charcode;

		dl_list_each_data(outlines->outline_list, &rft_op, __rft_print_outline);

		rft_op.cnt = 0;
		__rft_print_outlines_declaration(out_file, outlines, &rft_op);

		__rft_print_glyph_entry(out_file, outlines);
	}
}

/*
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
	// created outline data
	rft_glyphs_t *glyphs;
	FT_Vector lastPoint;
} rft_pcache_t;
*/

static void __rft_print_glyphs(rft_pcache_t *_pCache, rft_glyphs_t * glyphs)
{
	rft_pcache_t *pCache = _pCache;

	dl_list_each_data(glyphs->outlines_list, pCache->out_file, __rft_print_outlines);
}

static void __rft_print_glyph_entries(void **data, void *eachdata)
{
	rft_outlines_t *outlines = (rft_outlines_t *)*data;
	FILE *out_file = (FILE *)eachdata;

	if ( outlines->outline_list != NULL )
	{
		fprintf(out_file, "\t\t\t&__rf_glyph_%lx,\n", outlines->charcode);
	} else {
		fprintf(out_file, "\t\t\tNULL,\n");
	}
}

static void __rft_print_glyphs_array(rft_pcache_t *_pCache, rft_glyphs_t * glyphs)
{
	rft_pcache_t *pCache = _pCache;

	fprintf(pCache->out_file, "static rf_glyph_t* __rf_glyphs[%lu] = {\n", pCache->endCode + 1);

	dl_list_each_data(glyphs->outlines_list, pCache->out_file, __rft_print_glyph_entries);

	fprintf(pCache->out_file, "};\n\n");
}



static void __rft_print_header_file(rft_pcache_t *pCache)
{
	FILE *out_file = fopen(pCache->fontfilenameheader, "w");

	fprintf(out_file,"#ifndef RF_DEFAULT_PROVIDER_H\n#define RF_DEFAULT_PROVIDER_H\n\n");

	fprintf(out_file,"//including renderer header\n#include \"vec.h\"\n#include \"r_font.h\"\n\n");

	fprintf(out_file,"rf_provider_t* get_default_provider();\n\n");

	fprintf(out_file,"#endif\n");

	fclose(out_file);
}

static void __rft_print_complete_file(rft_pcache_t *pCache)
{
	pCache->out_file = fopen(pCache->fontfilename, "w");

	__rft_print_header(pCache);
	__rft_print_glyph_not_found(pCache);

	rft_glyphs_t * glyphs = pCache->glyphs;

	__rft_print_glyphs(pCache, glyphs);

	__rft_print_glyphs_array(pCache, glyphs);

	__rft_print_glyph_container(pCache);
	__rft_print_glyph_provider(pCache);

	fclose(pCache->out_file);

	__rft_print_header_file(pCache);
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

		pCache->providername = ( params->name == NULL ? "default" : params->name );
		__rft_create_font_file_name(pCache);

		__rft_set_charcode_range(pCache, params->minCharcode, params->maxCharcode);

		//for ( FT_ULong curCharcode = params->minCharcode; curCharcode <= params->maxCharcode ; curCharcode++ ) {
		for ( FT_ULong curCharcode = 0; curCharcode <= params->maxCharcode ; curCharcode++ ) {
			__rft_process_font_face(params, library, pCache, curCharcode);
		}

		__rft_print_complete_file(pCache);

		__rft_pcache_free(pCache);

		free(pCache);

		FT_Done_FreeType(library);
	}
}
