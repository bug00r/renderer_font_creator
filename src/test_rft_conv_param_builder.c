#include "rft_conv_param_builder.h"

#include <ctype.h>
#include <assert.h>

static void __test_alloc_free() 
{
    int argc = 0;
    char **argv = NULL;

    rft_conv_param_t* params = rft_conv_param_build(argc, argv);

    assert(params->font_param == NULL);
    assert(params->verbose == 0);
    assert(params->showHelp == 0);

    rft_conv_param_free(&params);

    assert(params == NULL);

}

static void __test_params(int argc, char* argv[]) 
{

    if ( argc < 5 ) return;  

    rft_conv_param_t* params = rft_conv_param_build(argc, argv);

    rft_conv_font_param_t *last_font_param = params->font_param;
    rft_conv_font_param_t *cur_font_param;
    
    while (last_font_param != NULL) {
        cur_font_param = last_font_param;
        printf("font: %s \n", cur_font_param->raw);
        printf("font_file: %s \n", cur_font_param->fontfile);
        printf("startCharcode: %lu \n", cur_font_param->startCharcode);
        printf("endCharcode: %lu \n", cur_font_param->endCharcode);
        printf("faceIndex: %lu \n", cur_font_param->faceIndex);
       
        last_font_param = (rft_conv_font_param_t *)cur_font_param->next;
    }

    printf("verbose: %i \n", params->verbose);
    printf("help: %i \n", params->showHelp);
    printf("hex: %i \n", params->hex);
    printf("minCharcode: %lu \n", params->minCharcode);
    printf("maxCharcode: %lu \n", params->maxCharcode);
    printf("hPixel: %i \n", params->hPixel);
    printf("#fonts: %i \n", params->cntFonts);

    //executed with: test_rft_conv_param_builder.exe --hex --verbose --font font1:5:10:0:21 --font font2:10:20:1:21 --font blu:0xA:0xb::
    rft_conv_font_param_t* fParam = rft_conv_font_by_charcode(params, 4);

    assert(fParam == NULL);

    fParam = rft_conv_font_by_charcode(params, 5);
    assert(fParam == (rft_conv_font_param_t*)params->font_param);

    fParam = rft_conv_font_by_charcode(params, 16);
    assert(fParam == (rft_conv_font_param_t*)params->font_param);

    fParam = rft_conv_font_by_charcode(params, 10);
    assert(fParam == (rft_conv_font_param_t*)params->font_param);

    fParam = rft_conv_font_by_charcode(params, 17);
    assert(fParam == (rft_conv_font_param_t*)params->font_param->next);

    fParam = rft_conv_font_by_charcode(params, 32);
    assert(fParam == (rft_conv_font_param_t*)params->font_param->next);

    fParam = rft_conv_font_by_charcode(params, 24);
    assert(fParam == (rft_conv_font_param_t*)params->font_param->next);

    rft_conv_param_free(&params);

    assert(params == NULL);

}

int main(int argc, char* argv[])
{
    /* unused */
	(void)(argc);
    (void)(argv);
	
    __test_alloc_free();

    __test_params(argc, argv);

	return 0;
}