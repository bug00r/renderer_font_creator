# renderer_font_creator
A simple Font to Mesh Transformer for rendering 3D Text in the renderer project. 

Usage:

make clean
make
./rft_converter.exe --hex --font ./../../../fonts/wqy-zenhei/wqy-zenhei.ttc:0x4E00:0x4E04:0x0 --font ./../../../fonts/arial/arial.ttf:0x21:0x5A:0x0 --hPixel 21 --verbose
-> font_MyChineseMixTestStyle.c

Description: 

    This Program converts Freetype readable vector based fonts into renderer usable triangulated Meshs.

Options:

	--hex		Range Info give as hex Value(see above), otherwise Codepoints
	--verbose	prints a lots of Debug Informations. Be careful, is able to spam your terminal
	--hPixel	rendering base pixelheight
	--font Record
	
		Structure:
		
			Example: ./../../../fonts/wqy-zenhei/wqy-zenhei.ttc:0x4E00:0x4E04:0x0
		
			- Delimiter ':'
			- Fields:
				1. Fontfile		    (Example: ./../../../fonts/wqy-zenhei/wqy-zenhei.ttc)
				2. Start Charcode	(Example: 0x4E00)
				3. End Charcode		(Example: 0x4E04)
				4. Font Face Index	(Example: 0x0)
				
