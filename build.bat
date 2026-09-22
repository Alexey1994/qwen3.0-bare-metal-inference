@load src/bootloader.asm | a386 > bin/bootloader.bin ^
	&& nasm src/system.asm -o bin/system16.bin ^
	&& tcc32 -w -nostdlib -c "src/system.c" -o "bin/system32.elf" ^
	&& load "bin/system16.bin" | to ld > "bin/system16.ld" ^
	&& ld -T script.ld -o "bin/system32.o" "bin/system32.elf" ^
	&& objcopy -O binary -S "bin/system32.o" "bin/system.bin" ^
	|| pause


@node unpack-tokenizer.js
@unpack-model.js
@node generate_script.js > build_fs.bat
@cmd /c build_fs.bat