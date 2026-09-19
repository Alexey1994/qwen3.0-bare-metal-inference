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
}
Loader_Api;


Loader_Api* loader_api;

void main();
void _start(Loader_Api api)
{
	loader_api = &api;
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
		sector = 0x600;

		for(i=0; i<15; ++i) {
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

	file_data = heap_top;

	while(sector_number) {
		loader_api->read_sector(sector_number);
		sector = 0x600;

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


Number32 tokens[4096];// = {17, 10, 17};
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
	buffer = load_file(file);

	for(i=0; i<size; ++i) {
		((Number32*)tensor)[i] = ((Number16*)buffer)[i] << 16;
	}

	free_memory(buffer);

	return tensor;
}


#include "cpu.c"
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


Number generate_next_token()
{
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


void main()
{
	//((Number16*)(0xB8000))[0] = 1 + (2<<8);

	//Number32 f;

	//f = open_file("tokenizer");

	//write_character_in_text_display('H');
	//write_character_in_text_display('i');

	//print("Hi %d", f);

	print("Loading...");


	Number32 i;


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


	tmp0 = create_tensor(4096 * HIDDEN_SIZE);

	Q = create_tensor(4096 * Q_HIDDEN_SIZE);
	K = create_tensor(4096 * HIDDEN_SIZE);
	V = create_tensor(4096 * HIDDEN_SIZE);
	O = create_tensor(4096 * Q_HIDDEN_SIZE);

	scores = create_tensor(4096);

	gate = create_tensor(4096 * INTERMEDIATE_SIZE);
	up = create_tensor(4096 * INTERMEDIATE_SIZE);
	hidden = create_tensor(4096 * INTERMEDIATE_SIZE);

	last_logits = create_tensor(EMBED_TOKENS_SIZE);

	output = create_tensor(4096 * HIDDEN_SIZE);


	print("ok\n");

	tokens[0] = 17;
	tokens[1] = 10;
	tokens[2] = 17;
	seq_len = 3;


	Number32 token;
	Number seq_len2 = seq_len;
	for(seq_len=1; seq_len<=seq_len2; ++seq_len) {
		token = tokens[seq_len-1];

		//Byte* decoded_text = (token < vocab_size && vocab[token].bytes)
		//	? vocab[token].bytes
		//	: "<unk>";
		
		//print("%s", vocab[token].bytes);

		token = generate_next_token();
	}

	print("%d", token);
/*
	for(; seq_len<4096; ++seq_len) {
		if(token == 151645) {
			break;
		}

		tokens[seq_len-1] = token;

		//Byte* decoded_text = (token < vocab_size && vocab[token].bytes)
		//	? vocab[token].bytes
		//	: "<unk>";
		
		print("%s", vocab[token].bytes);

		token = generate_next_token();

		//printf("next_token: %d %lld us\n", token, (t1 - t0) * 1000000 / frequency);
	}*/
}