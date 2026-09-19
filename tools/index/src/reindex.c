// TYPES //////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1) //no align for structs

// unsigned number
typedef unsigned char          Number8;  // [0, 255]
typedef unsigned short int     Number16; // [0, 65 535]
typedef unsigned long int      Number32; // [0, 4 294 967 295]
typedef unsigned long long int Number64; // [0, 18 446 744 073 709 551 615]

typedef char                   Signed_Number8;  // [-128, 127]
typedef short int              Signed_Number16; // [-32 768, 32 767]
typedef long int               Signed_Number32; // [-2 147 483 648, 2 147 483 647]
typedef long long int          Signed_Number64; // [-9 223 372 036 854 775 808, 9 223 372 036 854 775 807]

typedef float                  Real_Number32;
typedef double                 Real_Number64;
typedef long double            Real_Number80;
typedef Real_Number64          Real_Number;

typedef Number32               Number;
typedef Signed_Number32        Signed_Number;

typedef Number8                Byte;
typedef Number                 Boolean;


#define stdcall __attribute__((__stdcall__))
#define cdecl   __attribute__((__cdecl__))
#define import  __attribute__((dllimport))
#define export  __attribute__((dllexport))


// kernel32 ///////////////////////////////////////////////////////////////////////

import stdcall Byte* GetProcessHeap ();
import stdcall Byte* HeapAlloc      (Byte* heap, Number32 flags, Number size);
import stdcall void  HeapFree       (Byte* heap, Number32 flags, Byte* bytes);
import stdcall Byte* HeapReAlloc    (Byte* heap, Number32 flags, Byte* bytes, Number size);


typedef enum {
	STD_INPUT_HANDLE   = -10,
	STD_OUTPUT_HANDLE  = -11,
	STD_ERROR_HANDLE   = -12
}
Std_Handle;

import stdcall Byte* GetStdHandle(Std_Handle handle);

import stdcall Boolean ReadFile(
	Number32    file,
	Byte*       buffer,
	Number32    buffer_length,
	Number32*   bytes_readed,
	void*       overlapped
);

import stdcall Boolean WriteFile(
	Number32    file,
	Byte*       data,
	Number32    data_length,
	Number32*   bytes_writed,
	void*       overlapped
);


typedef enum {
	GENERIC_READ        = 0x80000000,
	GENERIC_WRITE       = 0x40000000,
	FILE_LIST_DIRECTORY = 0x00000001
}
Create_File_Mode;

typedef enum {
	DISABLE_ALL_FILE_OPERATION   = 0,
	FILE_SHARE_READ              = 1,
	ENABLE_WRITE_FILE_OPERATION  = 2,
	ENABLE_DELETE_FILE_OPERATION = 4
}
Enabled_File_Operation_For_Other_Processes;

typedef enum {
	CREATE_NEW        = 1,
	CREATE_ALWAYS     = 2,
	OPEN_EXISTING     = 3,
	OPEN_ALWAYS       = 4,
	TRUNCATE_EXISTING = 5,
}
Create_File_Action;

typedef enum {
	READONLY_FILE_ATTRIBUTE         = 1,
	HIDDEN_FILE_ATTRIBUTE           = 2,
	SYSTEM_FILE_ATTRIBUTE           = 4,
	DIRECTORY_FILE_ATTRIBUTE        = 0x10,
	ARCHIVE_FILE_ATTRIBUTE          = 0x20,
	DEVICE_FILE_ATTRIBUTE           = 0x40,
	FILE_ATTRIBUTE_NORMAL           = 0x80,
	TEMPORARY_FILE_ATTRIBUTE        = 0x100,
	SPARSE_FILE_FILE_ATTRIBUTE      = 0x200,
	REPARSE_POINT_FILE_ATTRIBUTE    = 0x400,
	COMPRESSED_FILE_ATTRIBUTE       = 0x800,
	OFFLINE_FILE_ATTRIBUTE          = 0x1000,
	NOT_INDEXED_FILE_ATTRIBUTE      = 0x2000,
	ENCRYPTED_FILE_ATTRIBUTE        = 0x4000,
	BACKUP_SEMANTICS_FILE_ATTRIBUTE = 0x02000000,
	NO_BUFFERING_FILE_ATTRIBUTE     = 0x20000000,
	OVERLAPPED_FILE_ATTRIBUTE       = 0x40000000,
}
File_Attribute;

import stdcall Signed_Number CreateFileA(
	Byte*                                       name,
	Create_File_Mode                            mode,
	Enabled_File_Operation_For_Other_Processes  flags2,
	void*                                       security_attributes,
	Create_File_Action                          create_action,
	File_Attribute                              attributes,
	Signed_Number                               file
);

import stdcall Byte CloseHandle(Signed_Number file);


import stdcall void ExitProcess(Number32 code);

import stdcall Number32 SetEnvironmentVariableA(Byte* name, Byte* value);
import stdcall Number32 GetEnvironmentVariableA(Byte* name, Byte* value, Number32 value_size);


// PROGRAM ////////////////////////////////////////////////////////////////////////


void* stdout;
void* stderr;
void* stdin;

void* heap;


void write_Number_align(void(*write_byte)(Byte byte), Number number, Signed_Number align)
{
	while(number && align) {
		number /= 10;
		--align;
	}

	while(align) {
		write_byte('0');
		--align;
	}
}


void write_Number(void(*write_byte)(Byte byte), Number number)
{
	Number next;

	next = number / 10;
	
	if(next) {
		write_Number(write_byte, next);
	}

	write_byte(number % 10 + '0');
}


void write_Signed_Number(void(*write_byte)(Byte byte), Signed_Number number)
{
	if(number < 0) {
		write_byte('-');
		number = -number;
	}

	write_Number(write_byte, number);
}


void write_Number_with_align(void(*write_byte)(Byte byte), Number number, Signed_Number align)
{
	write_Number_align(write_byte, number, align);
	write_Number(write_byte, number);
}


void write_String(void(*write_byte)(Byte byte), Byte* string)
{
	Byte character;

	for(;;) {
		character = *string;

		if(!character) {
			break;
		}

		write_byte(character);

		++string;
	}
}


void write_byte_in_stderr(Byte byte)
{
	Number32 bytes_writed;
	WriteFile(stderr, &byte, sizeof(byte), &bytes_writed, 0);
}

/*
void write_byte_in_stdout(Byte byte)
{
	Number32 bytes_writed;
	WriteFile(stdout, &byte, sizeof(byte), &bytes_writed, 0);
}*/


void write_bytes_in_stdout(Byte* bytes, Number number_of_bytes)
{
	Number32 bytes_writed;
	WriteFile(stdout, bytes, number_of_bytes, &bytes_writed, 0);
}


Number stdin_offset;
Number stdin_size;
Byte   stdin_buffer[4096];


Byte peek_byte()
{
	if(stdin_offset >= stdin_size) {
		Number32 bytes_readed = 0;
		ReadFile(stdin, stdin_buffer, sizeof(stdin_buffer), &bytes_readed, 0);
		stdin_offset = 0;
		stdin_size = bytes_readed;
	}

	return stdin_buffer[stdin_offset];
}


void next_byte()
{
	if(stdin_offset < stdin_size) {
		++stdin_offset;
	}
}


Boolean has_byte()
{
	return stdin_size;
}


Number stdout_size;
Byte   stdout_buffer[4096];


void flush_stdout()
{
	Number32 bytes_writed;
	WriteFile(stdout, stdout_buffer, stdout_size, &bytes_writed, 0);
	stdout_size = 0;
}


void write_byte_in_stdout(Byte byte)
{
	stdout_buffer[stdout_size] = byte;
	++stdout_size;

	if(stdout_size == 4096) {
		flush_stdout();
	}
}


Byte sector[512];


void _start()
{
	heap = GetProcessHeap();

	stdin = GetStdHandle(-10);
	stdout = GetStdHandle(-11);
	stderr = GetStdHandle(-12);


	Number next_sector = 1;
	Number i;

	for(;;) {
		for(i=0; i<sizeof(sector); ++i) {
			sector[i] = 0;
		}
	
		for(i=0; i<512; ++i) {
			Byte byte = peek_byte();

			if(!has_byte()) {
				break;
			}

			sector[i] = byte;

			next_byte();
		}

		if(!i) {
			break;
		}

		if(*(Number32*)(sector+508) != 0 && next_sector != 1) {
			*(Number32*)(sector+508) = next_sector;
		}

		write_bytes_in_stdout(sector, sizeof(sector));
		++next_sector;
	}

	ExitProcess(0);
}