/*
 * ZDic.c
 *
 * main file for ZDic
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#include <PalmOS.h>
#include <PalmOSGlue.h>

#include "Decode.h"

#define CACHE_SIZE		4096

// ���干�ó���
#define LOOK_SIZE		(497)					// ��Ѱ�Ӵ���С
#define WINDOW_SIZE		(15)					// Ԥ���Ӵ���С
#define BUFFER_SIZE		(LOOK_SIZE + WINDOW_SIZE)

// ���ϻ�������С
#define BUFFER_MASK		(BUFFER_SIZE - 1)			// �������ϻ������Ļ�������ֵ����
#define THRESHOLD		2							// �����ٽ�ֵ

#define RUN_ENCODED		0							// �������λԪ��: ֻ��<��ʼλ��>��<�ִ�����>�ֶ�
#define RAW_ENCODED		1							//				 : ֻ��<������Ԫ>�ֶ�

#define R(i)			(((i) + output.start) & BUFFER_MASK)	// ���ϻ������Ļ�������ֵ����

#define ALPHA_BIT		8
#define	ALPHA_SIZE		256

typedef struct {
	int					bit_count;	// remanent bits in current read byte
	int					now;		// current read byte index
	long				total;		// size of buffer.
	const unsigned char	*buffer;	// bytes buffer.
} InputType;

typedef InputType *InputPtr;

typedef struct {
	long				start;		// head index.
} OutputType;

typedef OutputType *OutputPtr;


// get bits number for load var.
static int use_bits(long var)
{
	int bits = 0;
	long test = 1;

	while(test < var)
	{
		test <<= 1;
		bits++;
	}

	return bits;
}


// ��ʼ�����뻺����
static int init_input(InputType *input, const unsigned char *buf, long len)
{
	input->bit_count = 8;
	input->now = 0;
	input->total = len;
	input->buffer = buf;
	
	return 0;
}

// ��ʼ�����������
static int init_output(OutputType *output)
{

	output->start = 0;
	
	return 0;
}


// ��ȡѹ��������֮����ʽ
static __inline long read_bits(int bits, InputType *input)
{
	long word = 0;
	unsigned char bit_buffer/*, temp*/;
	int need_bits;

	while(bits > 0)
	{
		if(input->bit_count == 0)				// λԪ�������Ƿ�Ϊ��
		{
			if(input->now >= input->total - 1)	// �ֽڻ������Ƿ�Ϊ��
			{
				return(-1);						// ȫ����ȡ���
			}
			input->bit_count = 8;
			input->now++;
		}
		bit_buffer = input->buffer[input->now];
		need_bits = bits > input->bit_count ? input->bit_count : bits;
		
/*		temp = (bit_buffer >> (input->bit_count - need_bits));
		temp &= (1 << need_bits) - 1;
		word <<= need_bits;
		word |= (unsigned long)temp;
		bits = bits - input->bit_count;
		input->bit_count -= need_bits;
*/
		word = (word <<= need_bits) | ((bit_buffer >> (input->bit_count - need_bits)) & ((1 << need_bits) - 1));
		bits -= input->bit_count;
		input->bit_count -= need_bits;
		
	}

	return (word);
}


// LZSS ����������Ҫ���븱��ʽ
int LZSS_Decoder(const unsigned char *src, long src_length,
	unsigned char *dst, long *dst_length)
{
	long start, length;
	long start_bit, length_bit;
	long i;
	long write_index, cur_index;
	InputType input;
	OutputType output;

	// ��ʼ����������������
	if(init_input(&input, src, src_length) == -1
		|| init_output(&output) == -1)
	{
		return -1;
	}

	// ����ÿ�������� <��ʼλ��>��<�ִ�����> �������λ��ʾ
	start_bit = use_bits(LOOK_SIZE);
	length_bit = use_bits(WINDOW_SIZE + 1);
	write_index = cur_index = 0;

	while(true)
	{
		switch(read_bits(1, &input))
		{
			case RUN_ENCODED:
			{
				// ����<��ʼλ��>��<�ִ�����>��ԭ��Ԥ���Ӵ��е�����
				start = read_bits(start_bit, &input);
				length = read_bits(length_bit, &input);
				
				if(start == -1 || length == -1)
				{
					goto DECODE_END;
				}

				// �������ȺͿ�ʼλ��
				length += THRESHOLD;
				start--;

				while (BUFFER_SIZE + start < write_index)
					start += BUFFER_SIZE;
				
				start += output.start;

				// ����ǰ����ֹ�����ͬ����
				for(i = 0; i < length; i++)
				{
					dst[write_index++] = dst[start++];
				}

				break;
			}
			
			case RAW_ENCODED:
			{
				// ����<������Ԫ>��ԭ��Ԥ���Ӵ��е�����
				dst[write_index++] = (unsigned char)read_bits(ALPHA_BIT, &input);
				//length = 1;

				break;
			}
			
			default:
				goto DECODE_END;
		}
	}

DECODE_END:	
	*dst_length = write_index;

	return 0;
}