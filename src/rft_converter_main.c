#include "rft_converter_main.h"
#include <ctype.h>

int main(int argc, char* argv[])
{
	rft_conv_param_t* params = rft_conv_param_build(argc, argv);

	if ( params->font_param != NULL )
	{
		convert( params );
	} 

	rft_conv_param_free(&params);
	
	return 0;
}

