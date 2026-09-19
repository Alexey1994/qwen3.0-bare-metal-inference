@"tcc64.exe" -w -nostdlib -nostdinc def/kernel32.def src/index.c -o index.exe ^
	|| pause

@"tcc64.exe" -w -nostdlib -nostdinc def/kernel32.def src/reindex.c -o reindex.exe ^
	|| pause