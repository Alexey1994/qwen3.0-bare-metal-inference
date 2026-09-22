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

typedef Number16               BFloat16;
typedef float                  Float32;
typedef double                 Float64;
typedef long double            Float80;

typedef Number32               Number;
typedef Signed_Number32        Signed_Number;

typedef Number8                Byte;
typedef Number                 Boolean;


#define stdcall __attribute__((__stdcall__))
#define cdecl   __attribute__((__cdecl__))


typedef struct {
	void (*read_sector) (Number32 sector);
	void (*matmul_optimized) (Float32* C, Float32* A, Float32* B, Number M, Number K, Number N);
}
Loader_Api;


Loader_Api* loader_api;

void (*matmul_optimized)(Float32* C, Float32* A, Float32* B, Number M, Number K, Number N) = 0;

void main();
void _start(Loader_Api api)
{
	loader_api = &api;
	matmul_optimized = api.matmul_optimized;
	main();
}


// heap ///////////////////////////////////////////////////////////////////////////

Byte* heap_top = 1024 * 1024;


void set_heap_top(Number new_top)
{
	heap_top = new_top;
}


void reset_heap()
{
	set_heap_top(1024 * 1024);
}


Byte* allocate_memory(Number size)
{
	Byte* allocated_memory;
	
	allocated_memory = heap_top;
	heap_top += size;
	
	return allocated_memory;
}


Byte* allocate_aligned_memory(Number size, Number align)
{
	Byte* allocated_memory;
	
	while((Number)heap_top % align) {
		++heap_top;
	}
	
	allocated_memory = heap_top;
	heap_top += size;
	
	return allocated_memory;
}


void free_memory(Byte* allocated_memory)
{
	heap_top = allocated_memory;
}


// file ///////////////////////////////////////////////////////////////////////////

//Byte sector[512];

Number32 open_file(Byte* name)
{
	Number32 sector_number;
	Byte* sector;
	Number i;
	Number j;

	sector_number = 1;

	for(;;) {
		loader_api->read_sector(sector_number);
		sector = 0x20000;

		for(i=0; i<31; ++i) {
			Byte* file = sector + i*16;

			for(j=0; j<12; ++j) {
				if(file[j]==0 || name[j]==0 || file[j]!=name[j]) {
					break;
				}
			}

			if(file[j]==name[j]) {
				return *(Number32*)(file+12);
			}
		}

		sector_number = *(Number32*)(sector+508);

		if(sector_number==0) {
			break;
		}
	}

	return 0;
}


Byte* load_file(Number32 sector_number)
{
	Byte* file_data;
	Byte* sector;
	Number i;
	Number32 current_sector;

	file_data = heap_top;

	loader_api->read_sector(sector_number);
	current_sector = sector_number;

	while(sector_number) {
		if(sector_number >= current_sector && sector_number < current_sector + 127) {
			sector = 0x20000 + (sector_number - current_sector) * 512;
		}
		else {
			loader_api->read_sector(sector_number);
			sector = 0x20000;
			current_sector =  sector_number;
		}

		for(i=0; i<508; ++i) {
			heap_top[i] = sector[i];
		}
		heap_top += 508;

		sector_number = *(Number32*)(sector+508);
	}

	return file_data;
}


// IO /////////////////////////////////////////////////////////////////////////////


Number8 in_8(Number16 port)
{
	Number8 data;

	asm volatile(
		"inb %1, %0"
		: "=a"(data)
		: "Nd"(port)
	);

	return data;
}


void out_8(Number16 port, Number8 data)
{
	asm volatile(
		"outb %0, %1"
		:
		: "a"(data), "Nd"(port)
	);
}


Number16 in_16(Number16 port)
{
	Number16 data;

	asm volatile(
		"inw %1, %0"
		: "=a"(data)
		: "Nd"(port)
	);

	return data;
}


void out_16(Number16 port, Number16 data)
{
	asm volatile(
		"outw %0, %1"
		:
		: "a"(data), "Nd"(port)
	);
}


Number32 in_32(Number16 port)
{
	Number32 data;

	asm volatile(
		"inl %1, %0"
		: "=a"(data)
		: "Nd"(port)
	);

	return data;
}


void out_32(Number16 port, Number32 data)
{
	asm volatile(
		"outl %0, %1"
		:
		: "a"(data), "Nd"(port)
	);
}


// text display ///////////////////////////////////////////////////////////////////

#define TEXT_DISPLAY_WIDTH 80
#define TEXT_DISPLAY_HEIGHT 25
//#define TEXT_DISPLAY_HEIGHT 50


Number16* display = 0xB8000;

Number cursor_pos_x = 0;
Number cursor_pos_y = TEXT_DISPLAY_HEIGHT - 1;

Byte text_color = 7;
Byte background_color = 0;


void hide_text_display_cursor()
{
	out_8(0x3D4, 0x0A);
	out_8(0x3D5, 0x20);
}


void set_text_display_cursor_position(Number x, Number y)
{
	Number16 position;
	
	cursor_pos_x = x;
	cursor_pos_y = y;
	
	position = y * TEXT_DISPLAY_WIDTH + x;
	
	out_8(0x3D4, 0x0F);
	out_8(0x3D5, position);
	
	out_8(0x3D4, 0x0E);
	out_8(0x3D5, position >> 8);
}


void set_character_in_text_display(Number x, Number y, Number character)
{
	display[y * TEXT_DISPLAY_WIDTH + x] = (Byte)character
		+ (text_color << 8)
		+ (background_color << 12);
}


void scroll_text_display_down()
{
	Number x;
	Number y;
	
	for(y = 0; y < TEXT_DISPLAY_HEIGHT - 1; ++y) {
		for(x = 0; x < TEXT_DISPLAY_WIDTH; ++x) {
			display[y * TEXT_DISPLAY_WIDTH + x] = display[(y + 1) * TEXT_DISPLAY_WIDTH + x];
		}
	}
		
	for(x = 0; x < TEXT_DISPLAY_WIDTH; ++x) {
		display[(TEXT_DISPLAY_HEIGHT - 1) * TEXT_DISPLAY_WIDTH + x] = (15 << 8);
	}
}


void write_character_in_text_display(Number character)
{
	switch(character) {
		case '\n': {
			cursor_pos_x = 0;
			++cursor_pos_y;
			break;
		}
		
		case '\r': {
			cursor_pos_x = 0;
			break;
		}
		
		case '\t': {
			write_character_in_text_display(' ');
			write_character_in_text_display(' ');
			write_character_in_text_display(' ');
			write_character_in_text_display(' ');
			break;
		}
		
		default: {
			set_character_in_text_display(cursor_pos_x, cursor_pos_y, character);
			++cursor_pos_x;
		}
	}
	
	if(cursor_pos_x >= TEXT_DISPLAY_WIDTH) {
		cursor_pos_x = 0;
		++cursor_pos_y;
	}
	
	if(cursor_pos_y >= TEXT_DISPLAY_HEIGHT) {
		--cursor_pos_y;
		
		scroll_text_display_down();
	}
	
	set_text_display_cursor_position(cursor_pos_x, cursor_pos_y);
}


// Keyboard ///////////////////////////////////////////////////////////////////////


typedef enum {
	PS2_OUTPUT_BUFFER_FULL       = 0b00000001,
	PS2_INPUT_BUFFER_FULL        = 0b00000010,
	PS2_INITIALIZED              = 0b00000100,
	PS2_A2_STATE                 = 0b00001000,
	PS2_KEYBOARD_CONNECTED       = 0b00010000,
	PS2_MOUSE_OUTPUT_BUFFER_FULL = 0b00100000,
	PS2_TIMEOUT_ERROR            = 0b01000000,
	PS2_PARITY_ERROR             = 0b10000000
}
PS2_State;


typedef enum {
	KEY_ESCAPE = 1,
	
	KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE,
	KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_OPEN_SQUARE_BRACKET, KEY_CLOSE_SQUARE_BRACKET, KEY_ENTER,
	KEY_LEFT_CONTROL, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_BACKTICK,
	KEY_LEFT_SHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHT_SHIFT,
	
	KEY_NUMPAD_MUL,
	KEY_LEFT_ALT,
	KEY_SPACE,
	
	KEY_CAPSLOCK,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
	KEY_NUM_LOCK, KEY_SCROLL_LOCK,
	
	KEY_NUMPAD_7, KEY_NUMPAD_8, KEY_NUMPAD_9, KEY_NUMPAD_MINUS,
	KEY_NUMPAD_4, KEY_NUMPAD_5, KEY_NUMPAD_6, KEY_NUMPAD_PLUS,
	KEY_NUMPAD_1, KEY_NUMPAD_2, KEY_NUMPAD_3,
	KEY_NUMPAD_0, KEY_NUMPAD_DOT,
	
	KEY_F11 = 87,
	KEY_F12 = 88,




	KEY_NUMPAD_ENTER = 156,
	KEY_RIGHT_CONTROL = 157,
	KEY_PRINT_SCREEN2 = 170,
	KEY_NUMPAD_DIV = 181,
	KEY_PRINT_SCREEN = 183,
	KEY_RIGHT_ALT = 184,
	
	KEY_HOME = 199,
	KEY_ARROW_UP = 200,
	KEY_PAGE_UP = 201,
	KEY_ARROW_LEFT = 203,
	KEY_ARROW_RIGHT = 205,
	KEY_END = 207,
	KEY_ARROW_DOWN = 208,
	KEY_PAGE_DOWN = 209,
	KEY_INSERT = 210,
	KEY_DELETE = 211,
	
	KEY_OS = 219,
	KEY_CONTEXT_MENU = 221,
}
Key_Code;

Byte key_to_char_code[128] = {
	0,

	0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, //'\r',
	0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
	
	'*', //55, * on numpad
	
	0, ' ', //57
	
	0, //58 CapsLock
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //68 F10
	
	0, 0, //70 ScrollLock
	
	//Numpad
	'7', '8', '9', '-',
	'4', '5', '6', '+',
	'1', '2', '3',
	'0', '.', //83
};

Byte key_to_shifted_char_code[128] = {
	0,

	0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
	0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, //'\r',
	0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
	
	'*', //55, * on numpad
	
	0, ' ', //57
	
	0, //58 CapsLock
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //68 F10
	
	0, 0, //70 ScrollLock
	
	//Numpad
	0, 0, 0, '-',
	0, 0, 0, '+',
	0, 0, 0,
	0, 0, //83
};

Number32 read_key_state()
{
	Byte     ps2_key_state;
	Number32 key_state;

	if(!(in_8(0x64) & PS2_OUTPUT_BUFFER_FULL)) {
		return 0;
	}

	ps2_key_state = in_8(0x60);

	if(ps2_key_state == 0xE0) {
		ps2_key_state = in_8(0x60);
		key_state = (ps2_key_state & 0b1111111) + 128;
	}
	else {
		key_state = ps2_key_state & 0b1111111;
	}
	
	if(ps2_key_state & 0b10000000) {
		key_state |= 0x80000000;
	}

	return key_state;
}


// Writer /////////////////////////////////////////////////////////////////////////

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


Signed_Number write(void(*write_byte)(Byte byte), Byte* format, Byte** values)
{
	Byte character;

	for(;;) {
		character = *format;
		
		if(!character) {
			break;
		}
		
		++format;

		if(character == '%') {
			character = *format;
			++format;

			switch(character) {
				
				case 'c':
					write_byte(*values);
					++values;
					break;

				case 'u':
					write_Number(write_byte, *(Number32*)values);
					++values;
					break;

				case 'd':
					write_Signed_Number(write_byte, *(Signed_Number32*)values);
					++values;
					break;

				case 's':
					write_String(write_byte, *values);
					++values;
					break;

				default:
					write_byte(character);
			}
		}
		else {
			write_byte(character);
		}
	}
}


void print(Byte* format, ...)
{
	write(&write_character_in_text_display, format, &format + 1);
}


// math ///////////////////////////////////////////////////////////////////////////

#define NAN (Number32)0x7FC00000
#define INFINITY (Number32)0x7F800000

#define PI 3.14159265358979323846f
#define LN2 0.69314718055994530942f
#define INV_LN2 1.44269504088896340736f

float sqrtf(float x) {
	if (x < 0.0f) return *(Float32*)&(Number32){NAN};
	if (x == 0.0f) return 0.0f;

	float guess = x;

	for (int i = 0; i < 20; i++) {
		guess = (guess + x / guess) * 0.5f;
	}

	return guess;
}


float reduce_angle(float x) {
	float two_pi = 2.0f * PI;
	while (x > PI) x -= two_pi;
	while (x < -PI) x += two_pi;
	return x;
}

float sinf(float x) {
	x = reduce_angle(x);

	float x2 = x * x;
	float x3 = x2 * x;
	float x5 = x3 * x2;
	float x7 = x5 * x2;
	float x9 = x7 * x2;

	return x - x3/6.0f + x5/120.0f - x7/5040.0f + x9/362880.0f;
}

float cosf(float x) {
	x = reduce_angle(x);

	float x2 = x * x;
	float x4 = x2 * x2;
	float x6 = x4 * x2;
	float x8 = x6 * x2;

	return 1.0f - x2/2.0f + x4/24.0f - x6/720.0f + x8/40320.0f;
}

float expf(float x) {
	if (x == 0.0f) return 1.0f;
	if (x > 88.0f) return *(Float32*)&(Number32){INFINITY};
	if (x < -88.0f) return 0.0f;

	// Без (int)(x * INV_LN2) — это вызывает _fixsfdi в TCC
	int k = 0;
	float r = x;
	while (r > LN2) { r -= LN2; k++; }
	while (r < -LN2) { r += LN2; k--; }

	float r2 = r * r;
	float r3 = r2 * r;
	float r4 = r3 * r;
	float r5 = r4 * r;
	float r6 = r5 * r;

	float exp_r = 1.0f + r + r2/2.0f + r3/6.0f + r4/24.0f + r5/120.0f + r6/720.0f;

	Number32 bits = *(Number32*)&exp_r;

	Number32 exponent = (bits >> 23) & 0xFF;
	exponent += k;
	bits = (bits & 0x807FFFFF) | (exponent << 23);

	float result;
	result = *(Float32*)&bits;

    return result;
}

float logf(float x) {
	if (x <= 0.0f) return *(Float32*)&(Number32){NAN};
	if (x == 1.0f) return 0.0f;

	Number32 bits = *(Number32*)&x;

	int E = ((bits >> 23) & 0xFF) - 127;
	bits = (bits & 0x807FFFFF) | 0x3F800000;

	float M = *(Float32*)&bits;

	float t = M - 1.0f;
	float t2 = t * t;
	float t3 = t2 * t;
	float t4 = t3 * t;
	float t5 = t4 * t;
	float t6 = t5 * t;
	float t7 = t6 * t;

	float ln_M = t - t2/2.0f + t3/3.0f - t4/4.0f + t5/5.0f - t6/6.0f + t7/7.0f;

	return E * LN2 + ln_M;
}

float powf(float x, float y) {
	if (x == 0.0f) {
		if (y > 0.0f) return 0.0f;
		if (y == 0.0f) return 1.0f;
		return *(Float32*)&(Number32){INFINITY};
	}

	if (x == 1.0f) return 1.0f;
	if (y == 0.0f) return 1.0f;
	if (y == 1.0f) return x;

	return expf(y * logf(x));
}


// transformer ////////////////////////////////////////////////////////////////////


#define HIDDEN_SIZE 1024
#define NUM_LAYERS 28
#define EMBED_TOKENS_SIZE 151936
#define INTERMEDIATE_SIZE 3072
#define NUM_HEADS 16
#define NUM_KV_HEADS 8
#define HEAD_DIM 128
#define KV_HIDDEN_SIZE (NUM_KV_HEADS * HEAD_DIM)
#define Q_HIDDEN_SIZE NUM_HEADS * HEAD_DIM
#define ROPE_THETA 10000.0f
#define RMS_NORM_EPS 0.000001f

#define MAX_SEQ_LEN 4096


Float32* embed_tokens;

Float32* layer_norm[NUM_LAYERS];

Float32* attn_wq[NUM_LAYERS];   // [HIDDEN_SIZE, HIDDEN_SIZE]
Float32* attn_wk[NUM_LAYERS];
Float32* attn_wv[NUM_LAYERS];
Float32* attn_wo[NUM_LAYERS];
Float32* attn_q_norm[NUM_LAYERS];
Float32* attn_k_norm[NUM_LAYERS];

Float32* mlp_gate[NUM_LAYERS];  // [HIDDEN_SIZE, INTERMEDIATE_SIZE]
Float32* mlp_up[NUM_LAYERS];    // [HIDDEN_SIZE, INTERMEDIATE_SIZE]
Float32* mlp_down[NUM_LAYERS];  // [INTERMEDIATE_SIZE, HIDDEN_SIZE]

Float32* post_attn_norm[NUM_LAYERS];

Float32* final_norm;


Number32 tokens[MAX_SEQ_LEN];// = {17, 10, 17};
Number32 seq_len = 0;


Float32* create_tensor(Number size)
{
	return allocate_memory(size*sizeof(Float32));
}


void copy_tensor(Float32* output, Float32* input, Number size)
{
	Number i;

	for(i=0; i<size; ++i) {
		output[i] = input[i];
	}
}


Byte** string_pointer;

void write_byte_in_string(Byte byte)
{
	Number i;
	Byte*  string;

	string = *string_pointer;
	*string = byte;
	*string_pointer += 1;
}


Byte tensor_name[13];

Float32* read_BF16_tensor(Number size, Byte* format, ...)
{
	Byte* file;
	Byte* buffer;
	Byte* tensor;
	Number i;

	tensor = create_tensor(size);

	Byte* tmp = tensor_name;
	string_pointer = &tmp;
	write(&write_byte_in_string, format, &format + 1);
	write_byte_in_string('\0');

	file = open_file(tensor_name);

	if(file==0) {
		print("%s not loaded\n", tensor_name);
	}

	buffer = load_file(file);

	for(i=0; i<size; ++i) {
		((Number32*)tensor)[i] = ((Number16*)buffer)[i] << 16;
	}

	free_memory(buffer);

	return tensor;
}


//#include "cpu.c"
//#include "opencl.c"
//#include "sse2.c"


void normalize(Float32* input, Float32* norm, Number32 size)
{
	Number i;

	Float32 sum_square = 0;

	for(i=0; i<size; ++i) {
		Float32 x = input[i];
		sum_square += x * x;
	}

	Float32 inverse = 1.0f / sqrtf(sum_square / size + RMS_NORM_EPS);

	for(i=0; i<size; ++i) {
		input[i] *= inverse * norm[i];
	}
}


Float32* Q;
Float32* K;
Float32* V;
Float32* O;
Float32* scores;

Float32* cache_K[4096];
Float32* cache_V[4096];

Float32* gate;
Float32* up;
Float32* hidden;

Float32* last_logits;
Float32* tmp0;
Float32* output;

//Number num_of_message = 0;

Number generate_next_token()
{
	/*
	++num_of_message;

	if(num_of_message % 8) {
		return 1;
	}
	else {
		return 151645;
	}
	*/

	Number32 i, j, k, l, h, d;

	cache_K[seq_len-1] = create_tensor(NUM_LAYERS * KV_HIDDEN_SIZE);
	cache_V[seq_len-1] = create_tensor(NUM_LAYERS * KV_HIDDEN_SIZE);


	Float32* embed = embed_tokens + tokens[seq_len-1] * HIDDEN_SIZE;

	for(i=0; i<HIDDEN_SIZE; ++i) {
		output[i] = embed[i];
	}


	Number32 half_dim = HEAD_DIM / 2;

	for(l=0; l<NUM_LAYERS; ++l) {
		copy_tensor(tmp0, output, HIDDEN_SIZE);
		normalize(output, layer_norm[l], HIDDEN_SIZE);

		// Self Attention
		matmul_optimized(Q, output, attn_wq[l], 1, HIDDEN_SIZE, Q_HIDDEN_SIZE);
		matmul_optimized(K, output, attn_wk[l], 1, HIDDEN_SIZE, HIDDEN_SIZE);
		matmul_optimized(V, output, attn_wv[l], 1, HIDDEN_SIZE, HIDDEN_SIZE);


		for(h=0; h<NUM_HEADS; ++h) {
			normalize(Q + h*HEAD_DIM, attn_q_norm[l], HEAD_DIM);
		}

		for(h=0; h<NUM_KV_HEADS; ++h) {
			normalize(K + h*HEAD_DIM, attn_k_norm[l], HEAD_DIM);
		}


		for(h=0; h<NUM_HEADS; ++h) {
			Number q_offset = h * HEAD_DIM;

			for(d=0; d<half_dim; ++d) {
				Float32 freq = 1.0f / powf(ROPE_THETA, ((Float32)d) / ((Float32)half_dim));
				Float32 angle = (Float32)(seq_len-1) * freq;

				Float32 c = cosf(angle);
				Float32 s = sinf(angle);

				Float32 q0 = Q[q_offset + d];
				Float32 q1 = Q[q_offset + d + half_dim];

				Q[q_offset + d]            = q0 * c - q1 * s;
				Q[q_offset + d + half_dim] = q1 * c + q0 * s;
			}
		}

		for(h=0; h<NUM_KV_HEADS; ++h) {
			Number k_offset = h * HEAD_DIM;

			for(d=0; d<half_dim; ++d) {
				Float32 freq = 1.0f / powf(ROPE_THETA, ((Float32)d) / ((Float32)half_dim));
				Float32 angle = (Float32)(seq_len-1) * freq;

				Float32 c = cosf(angle);
				Float32 s = sinf(angle);

				Float32 k0 = K[k_offset + d];
				Float32 k1 = K[k_offset + d + half_dim];

				K[k_offset + d]            = k0 * c - k1 * s;
				K[k_offset + d + half_dim] = k1 * c + k0 * s;
			}
		}

		copy_tensor(cache_K[seq_len-1] + l*KV_HIDDEN_SIZE, K, KV_HIDDEN_SIZE);
		copy_tensor(cache_V[seq_len-1] + l*KV_HIDDEN_SIZE, V, KV_HIDDEN_SIZE);


		for(i=0; i<Q_HIDDEN_SIZE; ++i) {
			O[i] = 0.0f;
		}

		Float32 scale = 1.0f / sqrtf((Float32)HEAD_DIM);

		for(h = 0; h < NUM_HEADS; ++h) {
			Float32* Q_head = Q + h * HEAD_DIM;
			//Float32* K_head = K + (h * NUM_KV_HEADS / NUM_HEADS) * HEAD_DIM;
			//Float32* V_head = V + (h * NUM_KV_HEADS / NUM_HEADS) * HEAD_DIM;
			Float32* O_head = O + h * HEAD_DIM;

			// Q * K^T
			for(i=0; i<seq_len; ++i) {
				Float32 dot = 0;
				Float32* K_cache = cache_K[i] + l*KV_HIDDEN_SIZE + (h * NUM_KV_HEADS / NUM_HEADS)*HEAD_DIM;

				for(k = 0; k < HEAD_DIM; ++k) {
					dot += Q_head[k] * K_cache[k];
				}
				scores[i] = dot * scale;
			}

			// Softmax
			Float32 max_val = scores[0];
			for(i=1; i<seq_len; ++i) {
				if(scores[i] > max_val) {
					max_val = scores[i];
				}
			}

			Float32 sum_exp = 0;
			for(i=0; i<seq_len; ++i) {
				scores[i] = expf(scores[i] - max_val);
				sum_exp += scores[i];
			}

			Float32 inv_sum = 1.0f / sum_exp;
			for(i=0; i<seq_len; ++i) {
				scores[i] *= inv_sum;
			}

			// Scores * V
			for(i=0; i<seq_len; ++i) {
				Float32* V_cache = cache_V[i] + l*KV_HIDDEN_SIZE + (h * NUM_KV_HEADS / NUM_HEADS)*HEAD_DIM;
				Float32 score_val = scores[i];
				for(k=0; k<HEAD_DIM; ++k) {
					O_head[k] += V_cache[k] * score_val;
				}
			}
		}

		matmul_optimized(output, O, attn_wo[l], 1, Q_HIDDEN_SIZE, HIDDEN_SIZE);

		for(i=0; i<HIDDEN_SIZE; ++i) {
			output[i] += tmp0[i];
		}


		// MLP
		copy_tensor(tmp0, output, HIDDEN_SIZE);
		normalize(output, post_attn_norm[l], HIDDEN_SIZE);

		matmul_optimized(gate, output, mlp_gate[l], 1, HIDDEN_SIZE, INTERMEDIATE_SIZE);
		matmul_optimized(up, output, mlp_up[l], 1, HIDDEN_SIZE, INTERMEDIATE_SIZE);

		for(j = 0; j < INTERMEDIATE_SIZE; ++j) {
			Float32 g = gate[j];
			hidden[j] = (g / (1.0f + expf(-g))) * up[j];
		}

		matmul_optimized(output, hidden, mlp_down[l], 1, INTERMEDIATE_SIZE, HIDDEN_SIZE);

		for(j=0; j<HIDDEN_SIZE; ++j) {
			output[j] += tmp0[j];
		}
	}

	normalize(output, final_norm, HIDDEN_SIZE);
	matmul_optimized(last_logits, output, embed_tokens, 1, HIDDEN_SIZE, EMBED_TOKENS_SIZE);

	Number best_id = 0;
	Float32 best_prob = last_logits[0];

	for(Number i = 1; i < EMBED_TOKENS_SIZE; ++i) {
		if(last_logits[i] > best_prob) {
			best_prob = last_logits[i];
			best_id = i;
		}
	}

	return best_id;
}


// Tokenizer //////////////////////////////////////////////////////////////////////

typedef struct {
	Number32 token;
	Number32 len;
	Byte bytes[];
} TokenVocab;

typedef struct {
	Number32 left;
	Number32 right;
	Number32 merged;
} MergeRule;


TokenVocab* vocab;
Number32    vocab_size;

MergeRule*  merges;
Number32    num_merges;

TokenVocab** vocab_index;
TokenVocab** sorted_vocabs;


Signed_Number compare_strings(Byte* string1, Byte* string2)
{
	Signed_Number difference;

	while(*string1 && *string2) {
		difference = (Signed_Number)*string1 - (Signed_Number)*string2;

		if(difference) {
			return difference;
		}

		++string1;
		++string2;
	}

	return (Signed_Number)*string1 - (Signed_Number)*string2;
}


void copy_bytes(Byte* destination, Byte* source, Number size)
{
	Number system_size;
	Number remind_size;

	system_size = size / sizeof(Number);
	remind_size = size % sizeof(Number);

	while(system_size) {
		*((Number*)destination) = *((Number*)source);

		--system_size;
		destination += sizeof(Number);
		source += sizeof(Number);
	}

	while(remind_size) {
		*destination = *source;

		--remind_size;
		++destination;
		++source;
	}
}


void sort_vocab(TokenVocab** vocab, Number32 size)
{
	if(size < 2) {

	}
	else if(size < 3) {
		TokenVocab** a = vocab;
		TokenVocab** b = a + 1;

		if((Signed_Number32)(*b)->len - (Signed_Number32)(*a)->len <= 0) {

		}
		else {
			TokenVocab* tmp = *a;
			*a = *b;
			*b = tmp;
		}
	}
	else {
		Number32 middle = size / 2;
		Number32 l_size = middle;
		Number32 r_size = size - middle;
		Number32 i;
		Number32 li;
		Number32 ri;

		TokenVocab** l = allocate_memory(l_size * sizeof(TokenVocab*));
		copy_bytes(l, vocab, l_size * sizeof(TokenVocab*));
		sort_vocab(l, l_size);

		TokenVocab** r = allocate_memory(r_size * sizeof(TokenVocab*));
		copy_bytes(r, vocab + middle, r_size * sizeof(TokenVocab*));
		sort_vocab(r, r_size);

		li = 0;
		ri = 0;

		for(i = 0; ; ++i) {
			if(li < l_size && (ri >= r_size || ((Signed_Number32)r[ri]->len - (Signed_Number32)l[li]->len) <= 0)) {
				vocab[i] = l[li];
				++li;
			}
			else if(ri < r_size) {
				vocab[i] = r[ri];
				++ri;
			}
			else {
				break;
			}
		}

		free_memory(r);
		free_memory(l);
	}
}


void init_tokenizer(Byte* filename) {
	Number32 file = open_file(filename);
	Byte* file_data = load_file(file);
	Number32 i;
	Number32 j;

	vocab_size = *(Number32*)(file_data);
	vocab = file_data + 4;

	sorted_vocabs = allocate_memory(vocab_size * sizeof(TokenVocab*));
	vocab_index = allocate_memory(vocab_size * sizeof(TokenVocab*));

	TokenVocab* current_token = vocab;

	for(i=0; i<vocab_size; ++i) {
	//for(i=0; i<10; ++i) {
		//for(j=0; j<current_token->len; ++j) {
		//	print("%c", current_token->bytes[j]);
		//}

		sorted_vocabs[i] = current_token;
		vocab_index[i] = current_token;

		current_token = (Byte*)current_token + sizeof(TokenVocab) + current_token->len;
	}

	sort_vocab(sorted_vocabs, vocab_size);
/*
	for(i=0; i<10; ++i) {
		for(j=0; j<sorted_vocabs[i]->len; ++j) {
			print("%c", sorted_vocabs[i]->bytes[j]);
		}
	}
*/

/*
	read_bytes_from_file(file, (Byte*)&vocab_size, 4);
	read_bytes_from_file(file, (Byte*)&num_merges, 4);

	vocab = allocate_memory(vocab_size * sizeof(TokenVocab));
	sorted_vocabs = allocate_memory(vocab_size * sizeof(TokenVocab*));
	for(Number32 i = 0; i < vocab_size; ++i) {
		read_bytes_from_file(file, (Byte*)&vocab[i].len, 4);
		vocab[i].bytes = allocate_memory(vocab[i].len + 1);
		read_bytes_from_file(file, vocab[i].bytes, vocab[i].len);
		vocab[i].bytes[vocab[i].len] = '\0';

		sorted_vocabs[i] = vocab + i;
	}

	sort_vocab(sorted_vocabs, vocab_size);

	merges = allocate_memory(num_merges * sizeof(MergeRule));
	read_bytes_from_file(file, merges, num_merges * sizeof(MergeRule));

	close_file(file);*/
}


Signed_Number32 find_substring(Byte* text, Number32 text_size, Byte* substring, Number32 substring_size)
{
	Number32 i;
	Number32 j;

	for(i=0; i+substring_size <= text_size; ++i) {
		for(j=0; j<substring_size && j+i<text_size; ++j) {
			if(substring[j] != text[i + j]) {
				break;
			}
		}

		if(j == substring_size) {
			return i;
		}
	}

	return -1;
}


typedef struct {
	struct List* next;
	Number32 type; // 0 - text, 1 - token
	Byte*    value;
	Number32 value_size;
}
List;


List* tokens_list;


Number tokenize(Byte* text, Number32 text_size)
{
	Number32 i;

	tokens_list = allocate_memory(sizeof(List));
	tokens_list->next = 0;
	tokens_list->type = 0;
	tokens_list->value = text;
	tokens_list->value_size = text_size;

	for(i=0; i<vocab_size; ++i) {

		List* current = tokens_list;

		while(current) {
			if(current->type == 0) {
				Signed_Number32 find_index = find_substring(current->value, current->value_size, sorted_vocabs[i]->bytes, sorted_vocabs[i]->len);

				if(find_index >= 0) {
					List* new_token = allocate_memory(sizeof(List));
					new_token->type = 1;
					new_token->value = sorted_vocabs[i]->token;

					List* new_text = allocate_memory(sizeof(List));
					new_text->next = current->next;
					new_text->type = 0;
					new_text->value = current->value + find_index + sorted_vocabs[i]->len;
					new_text->value_size = current->value_size - find_index - sorted_vocabs[i]->len;

					current->value_size = find_index;
					current->next = new_token;
					new_token->next = new_text;
				}
			}

			current = current->next;
		}
	}


	Number num_of_new_tokens = 0;

	List* current_token = tokens_list;

	while(current_token) {
		while(current_token && current_token->type != 1) {
			current_token = current_token->next;
		}

		if(current_token) {
			Number32 token = current_token->value;
			current_token = current_token->next;

			//print("%d ", token);

			tokens[seq_len + num_of_new_tokens] = token;
			++num_of_new_tokens;
		}
	}

	free_memory(tokens_list);

	return num_of_new_tokens;
}


Byte message[2048];
Number32 message_size = 0;


void print_tokens()
{
	Number i;

	for(i=0; i<seq_len; ++i) {
		TokenVocab* vocab_token = vocab_index[tokens[i]];
		
		Number32 j;
		for(j=0; j<vocab_token->len; ++j) {
			print("%c", vocab_token->bytes[j]);
		}
	}
}


void main()
{
	Number32 i;

/*
	//return;

	//((Number16*)(0xB8000))[0] = 1 + (2<<8);

	Number32 f;

	f = open_file("tokenizer");

	//write_character_in_text_display('H');
	//write_character_in_text_display('i');

	print("Hi %d", f);
	return;
*/


	print("Loading...");


	embed_tokens = read_BF16_tensor(EMBED_TOKENS_SIZE*HIDDEN_SIZE, "embed");

	for(i=0; i<NUM_LAYERS; ++i) {
		layer_norm[i] = read_BF16_tensor(HIDDEN_SIZE, "L%dnorm_in", i);
		attn_wq[i] = read_BF16_tensor(Q_HIDDEN_SIZE * HIDDEN_SIZE, "L%dq", i); //transposed !!!
		attn_wk[i] = read_BF16_tensor(HIDDEN_SIZE * HIDDEN_SIZE, "L%dk", i); //transposed !!!
		attn_wv[i] = read_BF16_tensor(HIDDEN_SIZE * HIDDEN_SIZE, "L%dv", i); //transposed !!!
		attn_wo[i] = read_BF16_tensor(HIDDEN_SIZE * Q_HIDDEN_SIZE, "L%do", i); //transposed !!!
		attn_q_norm[i] = read_BF16_tensor(HEAD_DIM, "L%dq_n", i);
		attn_k_norm[i] = read_BF16_tensor(HEAD_DIM, "L%dk_n", i);
		mlp_gate[i] = read_BF16_tensor(INTERMEDIATE_SIZE * HIDDEN_SIZE, "L%dgate", i); //transposed !!!
		mlp_up[i] = read_BF16_tensor(HIDDEN_SIZE * INTERMEDIATE_SIZE, "L%dup", i); //transposed !!!
		mlp_down[i] = read_BF16_tensor(INTERMEDIATE_SIZE * HIDDEN_SIZE, "L%ddown", i); //transposed !!!
		post_attn_norm[i] = read_BF16_tensor(HIDDEN_SIZE, "L%dnorm_out", i);
	}

	final_norm = read_BF16_tensor(HIDDEN_SIZE, "norm");


	tmp0 = create_tensor(HIDDEN_SIZE);

	Q = create_tensor(Q_HIDDEN_SIZE);
	K = create_tensor(HIDDEN_SIZE);
	V = create_tensor(HIDDEN_SIZE);
	O = create_tensor(Q_HIDDEN_SIZE);

	scores = create_tensor(MAX_SEQ_LEN);

	gate = create_tensor(INTERMEDIATE_SIZE);
	up = create_tensor(INTERMEDIATE_SIZE);
	hidden = create_tensor(INTERMEDIATE_SIZE);

	last_logits = create_tensor(EMBED_TOKENS_SIZE);

	output = create_tensor(HIDDEN_SIZE);

	


	Byte start_prompt[] = "<|im_start|>system\n"
		"You are God\n"
		"<|im_end|>\n"
		"<|im_start|>user\n";

	Byte start_message_prompt[] = "<|im_end|>\n"
		"<|im_start|>user\n";

	Byte end_message_prompt[] = "<|im_end|>\n"
		"<|im_start|>assistant\n"
		"<think>\n"
		"</think>";

	Number32 num_of_new_tokens;


	init_tokenizer("tokenizer");

	num_of_new_tokens = tokenize(start_prompt, sizeof(start_prompt)-1);

	seq_len=1;
	for(i=0; i<num_of_new_tokens; ++i) {
		generate_next_token();
		++seq_len;
	}


	print("ok\n\n>");


	Number32 shift_active = 0;

	for(;;) {
		Number32 key_state;
		Byte key_code;

		key_state = read_key_state();
		key_code = key_state & 0xFF;

		if(!key_state) {
			asm("hlt");
		}

		if(!(key_state & 0x80000000)) {
			switch(key_code) {
				case KEY_LEFT_SHIFT:
				case KEY_RIGHT_SHIFT: {
					shift_active = 1;
					break;
				}

				case KEY_BACKSPACE: {
					if(message_size) {
						if(cursor_pos_x == 0) {
							cursor_pos_x = TEXT_DISPLAY_WIDTH - 1;

							if(cursor_pos_y) {
								--cursor_pos_y;
							}
						}
						else {
							--cursor_pos_x;
						}
						
						set_text_display_cursor_position(cursor_pos_x, cursor_pos_y);
						set_character_in_text_display(cursor_pos_x, cursor_pos_y, ' ');

						--message_size;
					}

					break;
				}

				case KEY_ENTER: {
					print("\n");

					if(shift_active) {
						message[message_size] = '\n';
						++message_size;
					}
					else {
						if(message_size) {
							//for(i=0; i<message_size; ++i) {
							//	print("%c", message[i]);
							//}

							text_color = 6;

							num_of_new_tokens = tokenize(start_message_prompt, sizeof(start_message_prompt)-1);

							for(i=0; i<num_of_new_tokens; ++i) {
								generate_next_token();
								++seq_len;
							}

							num_of_new_tokens = tokenize(message, message_size);

							for(i=0; i<num_of_new_tokens; ++i) {
								generate_next_token();
								++seq_len;
							}

							num_of_new_tokens = tokenize(end_message_prompt, sizeof(end_message_prompt)-1);

							for(i=0; i<num_of_new_tokens; ++i) {
								generate_next_token();
								++seq_len;
							}

							for(i=seq_len; i<MAX_SEQ_LEN; ++i) {
								Number32 token = generate_next_token();

								if(token == 151645) {
									break;
								}

								tokens[seq_len] = token;
								++seq_len;

								TokenVocab* vocab_token = vocab_index[token];
								Number32 j;

								for(j=0; j<vocab_token->len; ++j) {
									print("%c", vocab_token->bytes[j]);
								}
							}

							text_color = 7;

							print("\n\n>");
						}
						else {
							//print_tokens();
						}

						message_size = 0;
					}

					break;
				}

				default: {
					Byte character;

					if(shift_active) {
						character = key_to_shifted_char_code[key_code];
					}
					else {
						character = key_to_char_code[key_code];
					}

					if(character) {
						print("%c", character);
					
						message[message_size] = character;
						++message_size;
					}
				}
			}
		}
		else {
			switch(key_code) {
				case KEY_LEFT_SHIFT:
				case KEY_RIGHT_SHIFT: {
					shift_active = 0;
					break;
				}
			}
		}
	}
}