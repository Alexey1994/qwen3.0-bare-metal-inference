SECTIONS
{
	. = 0x8000;
	. = ALIGN(4);
	.text : {
		INCLUDE "bin/system16.ld";
		* (.text);
	}
	.data : {
		* (.data);
	}
}