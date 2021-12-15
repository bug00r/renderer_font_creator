#include "rft_converter_main.h"
#include <ctype.h>

int main(int argc, char* argv[])
{
	if( argc < 3 ) {
		fprintf( stderr, "usage: %s fontfile.ttc size\n", argv[0] );
		return 1;
	}

	rft_conv_param_t* params = rft_conv_param_build(argc, argv);

	printf_init( params );

	rft_conv_param_free(&params);
	
	return 0;
}

