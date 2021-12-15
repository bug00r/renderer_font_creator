#include "rft_conv_param_builder.h"

/* Count of possible parameter parsing steps */
static const int __rft_PARSE_CYCLE_MAX = 50;
  char *fontfile;
    FT_ULong startCharcode;
    FT_ULong endCharcode;
    FT_Long faceIndex;
    int hPixel;
/* 5 Token: 
        1. font file
        2. startCharcode
        3. endCharcode
        4. faceIndex
*/
static const int __rft_PARSE_TOKEN_MAX = 4;

/* font param delimiter */
static const char __rft_PARSE_TOKEN_DELIMITER = ':';


static struct option __rft_long_options[] =
        {
          /* These options set a flag. */
          {"verbose", no_argument, NULL, 1},                //&params->verbose
          {"help",   no_argument, NULL, 1},                 //&params->showHelp
          {"hex",   no_argument, NULL, 1},                  //&params->hex
          {"outline",   no_argument, NULL, 1},                  //&params->outline
          {"hPixel",   required_argument, 0, 'h'},
          {"font",  required_argument, 0, 'd'},
          /* These options don’t set a flag.
             We distinguish them by their indices. */
          /*{"add",     no_argument,       0, 'a'},
          {"append",  no_argument,       0, 'b'},
          {"font",  required_argument, 0, 'd'},
          {"create",  required_argument, 0, 'c'},
          {"file",    required_argument, 0, 'f'},*/
          {0, 0, 0, 0}
        };

/*
######
###### PRIVATE API
######
*/


char * __rft_param_format_string_new(const char * msg, ...) {
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

static void __rft_conv_font_param_init(rft_conv_font_param_t *_font_params) 
{
    rft_conv_font_param_t* params = _font_params;
    params->raw             = NULL;
    params->fontfile        = NULL;
    params->startCharcode   = 0UL;
    params->endCharcode     = 0UL;
    params->faceIndex       = 0UL;
    params->next            = NULL;
}

static void __rft_conv_font_param_free(rft_conv_font_param_t **_font_params) 
{
    if ( _font_params != NULL && *_font_params != NULL ) 
    {
        
        rft_conv_font_param_t *curfont_params = *_font_params;
        free(curfont_params->raw);
        free(curfont_params->fontfile);

        if (curfont_params->next != NULL) 
        {
            __rft_conv_font_param_free((rft_conv_font_param_t **)&curfont_params->next);
        }
        
        free(curfont_params);
        *_font_params = NULL;
    }
}

static void __rft_conv_param_init(rft_conv_param_t* _params) 
{
    rft_conv_param_t* params = _params;
    params->font_param      = NULL;
    params->showHelp        = 0;
    params->verbose         = 0;
    params->hex             = 0;
    params->hPixel          = 0;
    params->cntFonts        = 0;
    params->minCharcode     = ULONG_MAX;
    params->maxCharcode     = 0UL;
    params->long_options    = __rft_long_options;
    params->long_options[0].flag = &params->verbose;
    params->long_options[1].flag = &params->showHelp;
    params->long_options[2].flag = &params->hex;
    params->long_options[3].flag = &params->outline;
}

static void __rft_conv_param_free(rft_conv_param_t **_params) 
{
    if ( _params != NULL && *_params != NULL ) 
    {
        rft_conv_param_t *params = *_params;
        if ( params->font_param != NULL ) 
        {
            __rft_conv_font_param_free(&params->font_param);
        }
        free(params);
        *_params = NULL;
    }
}

static char* __rft_copy_str_range(char *start, char *end) {
    	size_t count = end - start;
		count++;
		char *copy = malloc((count+1)*sizeof(char));
		strncpy(copy, start, count);
		copy[count] = '\0';
        return copy;
}

static void __rft_parse_font_entries(rft_conv_param_t * _params, rft_conv_font_param_t *_font_param) 
{
    rft_conv_font_param_t *font_param = _font_param;
    rft_conv_param_t * params = _params;


    char *ft_param_raw = font_param->raw;
    size_t raw_len = strlen(ft_param_raw);

    /* minimum len should be 7 (excl. '\0') because:  --font f:1:2:3 */
    if ( raw_len < 7 ) {
        //TODO some error parameter
        return;
    }

    
    char *raw_end = ft_param_raw+(raw_len-1);
    char *token_start = ft_param_raw;
    char *token_end = token_start;

    int tokenCnt = 0;

    while ( (tokenCnt < __rft_PARSE_TOKEN_MAX) && token_end != raw_end )
    {
        char *token_end = strchr(token_start, __rft_PARSE_TOKEN_DELIMITER);

        if ( token_end == NULL ) token_end = raw_end;

        token_end = (token_end == raw_end ? token_end : token_end - 1); 
        char * token_value = __rft_copy_str_range(token_start, token_end);
        char * parseEnd;

        int numberBase = ( params->hex == 1 ? 16 : 10 );

        switch(tokenCnt)
        {
            /* handling font_file */
            case 0: font_param->fontfile = token_value;
                    break;
            /* handling startCharcode */
            case 1: font_param->startCharcode = strtoul(token_value, &parseEnd, numberBase);
                    params->minCharcode = ( params->minCharcode < font_param->startCharcode ?
                                            params->minCharcode : font_param->startCharcode );
                    break;
            /* handling endCharcode */
            case 2: font_param->endCharcode = strtoul(token_value, &parseEnd, numberBase);
                    params->maxCharcode = ( params->maxCharcode < font_param->endCharcode ?
                                            font_param->endCharcode : params->maxCharcode );
                    break;
            /* handling Font Face */
            case 3: font_param->faceIndex = strtoul(token_value, &parseEnd, numberBase);
                    break;
            /* something went wrong */
            default: break;
        }

        if ( tokenCnt > 0 ) free(token_value);
        
        token_start = (token_end == raw_end ? token_end : token_end + 2);
        
        tokenCnt++;
    }



}

static void __rft_parse_font_param(rft_conv_param_t *params, char * fontarg) 
{
    rft_conv_font_param_t *font_param = malloc(sizeof(rft_conv_font_param_t));

    __rft_conv_font_param_init(font_param);
    
    font_param->raw = __rft_param_format_string_new("%s", fontarg);
    
    rft_conv_font_param_t **last_font_param = &params->font_param;
    rft_conv_font_param_t **cur_font_param;
    
    do 
    {
        cur_font_param = last_font_param;
        if ( *last_font_param == NULL ) 
        {
            break;
        } else 
        {
            last_font_param = (rft_conv_font_param_t **)&(*cur_font_param)->next;
        }
    } while (*last_font_param != NULL);

    *last_font_param = font_param;

    __rft_parse_font_entries(params, font_param);
}

/*
######
###### PUBLIC API
######
*/

rft_conv_param_t* rft_conv_param_build(int argc, char* argv[])
{
    rft_conv_param_t *params = malloc(sizeof(rft_conv_param_t));
    __rft_conv_param_init(params);

    int curParseCnt = 0;
    
    while( curParseCnt++ < __rft_PARSE_CYCLE_MAX ) 
    {

        int option_index = 0;

        int c = getopt_long (argc, argv, "", params->long_options, &option_index);

        if (c == -1) break;
        
        switch (c)
        {
            case 'd':
                params->cntFonts++;
                __rft_parse_font_param(params, optarg);
                break;
            
            case 'h': {
                char *parseEnd;
                params->hPixel = strtoul(optarg, &parseEnd, 10);
                break;
            }
            case '?':
            /* getopt_long already printed an error message. */
            break;

            default:
                break;
        }
    }

    return params;
}

rft_conv_font_param_t* rft_conv_font_by_charcode(rft_conv_param_t *params, FT_ULong charcode) 
{
    rft_conv_font_param_t *last_font_param = params->font_param;
    rft_conv_font_param_t *cur_font_param;
    
    while (last_font_param != NULL) {
        cur_font_param = last_font_param;

        if ( charcode >= cur_font_param->startCharcode && charcode <= cur_font_param->endCharcode ) break;

        last_font_param = (rft_conv_font_param_t *)cur_font_param->next;
        cur_font_param = NULL;
    }

    return cur_font_param;
}

void rft_conv_param_free(rft_conv_param_t **_params) 
{
    __rft_conv_param_free(_params);
}
