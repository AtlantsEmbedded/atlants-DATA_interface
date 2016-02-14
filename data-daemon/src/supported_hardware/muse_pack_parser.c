/**
 * @file muse_pack_parser.c
 * @author Frédéric Simard, Atlans embedded (fred.simard@atlantsembedded.com)
 * @date June, 2015
 * @brief This file implements the function required to parse a packet
 *        sent by the muse. The following packets types can be parsed:
 *        - compressed EEG
 *        - uncompressed EEG
 * 
 * For most part, the algorithm used for compressed EEG is described here:
 * https://sites.google.com/a/interaxon.ca/muse-developer-site/muse-communication-protocol/compressed-eeg-packets
 * 
 * And the general description of the Muse bluetooth protocol:
 * https://sites.google.com/a/interaxon.ca/muse-developer-site/muse-communication-protocol/data-streaming-protocol
 * 
 * Parts of the decompression algorithm were adapted from:
 * https://github.com/DavidVivancos/MuseDotNet
 * Thanks dude!
 */
#include "muse_pack_parser.h"

#define MEDIAN_OFFSET 1 /*byte offset of the medians in the compressed packet*/
#define BITLENGTH_OFFSET 6 /*byte offset of the bit length in the compressed packet*/
#define DELTAS_OFFSET 8 /*byte offset of the deltas in the compressed packet*/

/*utils to display bits*/
void print_char_bits(unsigned char v);
void print_int_bits(unsigned int v);

/*sub routines to parse the compressed packets*/
int compute_quantization(int quantizations);
void compressed_parse_medians(unsigned char* medians_header, int* quantizations, int* medians);
int compressed_parse_bit_length(unsigned char* bit_length_header);
int compressed_parse_deltas(unsigned char* bits_header, int* median, int* quantization, int* deltas);

void shift_one_bit(char* cur_byte, int* byteshift, int* bitshift, unsigned char* bits_header);


/**
 * int preparse_packet(char* raw_packet_header, char **packet_headers)
 * 
 * @brief find the beginning of soft packets in the raw packet
 * @param raw_packet_header, pointer to the beginning of the packet
 * @param (out)soft_packet_headers, pointers to the beginning of the soft packets
 * @return number of packets found
 */ 
 
 
#define COMP_BITLENGTH_OFFSET 6
 

int preparse_packet(unsigned char* raw_packet_header, int packet_length, int *soft_packet_headers, int *soft_packet_types)
{
	int position = 0;
	int nb_packets = 0;
	int bitlength = 0;
	
	/*until we reach the end of the raw packet*/
	while((position<packet_length) & (nb_packets<MAX_NB_SOFT_PACKETS)){
		
		soft_packet_headers[nb_packets] = position;
		
		/*check the packet type, figure out its length and jump to the next*/
		switch(get_packet_type(raw_packet_header[position]))
		{
			/*sync packet is 4 bytes*/
			case MUSE_SYNC_PKT:
				position += 4;
				soft_packet_types[nb_packets] = MUSE_SYNC_PKT;
			break;
			
			/*uncompressed size is 1+(2)+5, the 2 only if flag is true*/
			case MUSE_UNCOMPRESS_PKT:
			
				if(get_flag_value(raw_packet_header[position])){
					position += 8;
					printf("Samples were dropped!\n");
				}
				else{
					position += 6;
				}
					
				soft_packet_types[nb_packets] = MUSE_UNCOMPRESS_PKT;
			break;
			
			/*error size is 5*/
			case MUSE_ERR_PKT:
				position += 5;
				soft_packet_types[nb_packets] = MUSE_ERR_PKT;
			break;
			
			/*compressed size is encoded in the packet*/
			case MUSE_COMPRESSED_PKT:
			
				bitlength = compressed_parse_bit_length(&(raw_packet_header[COMP_BITLENGTH_OFFSET+position]));
				
				position+=8;
				position+=(int)(bitlength/8);
				if(bitlength%8)
					position+=1;
				
				soft_packet_types[nb_packets] = MUSE_COMPRESSED_PKT;
			break;
			
			/*battery size is 9*/
			case MUSE_BATT_PKT:
				position += 9;
				soft_packet_types[nb_packets] = MUSE_BATT_PKT;
			break;
			
			/*uncompressed size is 1+(2)+4, the 2 only if flag is true*/
			case MUSE_ACC_PKT:
				if(get_flag_value(raw_packet_header[position]))
					position += 7;
				else
					position += 5;
					
				soft_packet_types[nb_packets] = MUSE_ACC_PKT;
			break;
			
			case MUSE_DRLREF_PKT:
				position += 4;
				soft_packet_types[nb_packets] = MUSE_DRLREF_PKT;
			break;
			
			case MUSE_INVALID:
			default:
				printf("Pre-Parsing error!\n");
				return 0;
		}
	
		nb_packets = nb_packets+1;
		
	}
	
	return nb_packets;
}

/**
 * inline int get_packet_type(unsigned char first_byte)
 * 
 * @brief extracts the packet type from the byte
 * @param first_byte, first byte of the packet
 * @return packet type shifted to 0xF
 */ 
inline int get_packet_type(unsigned char first_byte)
{
	return (first_byte>>4)&0x0F;
}


/**
 * inline int get_flag_value(unsigned char first_byte)
 * 
 * @brief extracts the dropped packet flag value
 * @param first_byte, first byte of the packet
 * @return binary value of the flag
 */ 
int get_flag_value(unsigned char first_byte)
{
	return (first_byte&0x08)>0;
}


/**
 * int parse_compressed_packet(unsigned char* packet_header, char* deltas)
 * 
 * @brief extracts the dropped packet flag value
 * @param packet_header, address to the first byte of the packet
 * @param (out)deltas, array containing the deltas extracted from the packet must be 16*4 sizeof(int) in size
 * @return success or fail
 */ 
int parse_compressed_packet(unsigned char* packet_header, int* deltas)
{
	int medians[4];
	int quantizations[4];
	int expected_bits_length = 0;
	int parsed_bits_length = 0;
	
	/*parse out the medians and quantizations*/
	compressed_parse_medians(&(packet_header[MEDIAN_OFFSET]), quantizations, medians);
	/*parse out the bitlength*/
	expected_bits_length = compressed_parse_bit_length(&(packet_header[BITLENGTH_OFFSET]));
	/*parse out the deltas*/
	parsed_bits_length = compressed_parse_deltas(&(packet_header[DELTAS_OFFSET]), medians, quantizations, deltas);
	
	/*error, the number of bits parser doesn't equal the declarations (usually a major bug)*/
	if(expected_bits_length!=parsed_bits_length){
	
		printf("error, parsing compressed packet\n");
		printf("expected_bits_length: %i\n",expected_bits_length);
		printf("parsed_bits_length: %i\n",parsed_bits_length);
	}
	
	return EXIT_SUCCESS;
}

/**
 * void compressed_parse_medians(char* medians_header, int* quantizations, int* medians)
 * 
 * @brief extracts the medians and quantizations from bytes [1-5]
 * @param medians_header, the byte array
 * @param (out)quantizations, quantizations array
 * @param (out)medians, medians array
 */ 
void compressed_parse_medians(unsigned char* medians_header, int* quantizations, int* medians)
{
	/*medians occupy 6 bits and quantization 4 bits for a total of 10 bits*/
	/*Here's a mapping of the bits, but refer to documentation for details*/
	/*byte[0]:QQMM MMMM*/ 
	/*byte[1]:MMMM MMQQ*/
	/*byte[2]:MMMM QQQQ*/
	/*byte[3]:MMQQ QQMM*/
	/*byte[4]:QQQQ MMMM*/
	
	/*extract first median and first quantization bits*/
	medians[0] = (int)medians_header[0]&0x3f;
	quantizations[0] = (int)((medians_header[0]&0xC0)>>6 | (medians_header[1]&0x03)<<2);
	
	/*extract second median and second quantization bits*/
	medians[1] = (int)(medians_header[1]&0xfc)>>2;
	quantizations[1] = (int)(medians_header[2]&0x0f);
	
	/*extract third median and third quantization bits*/
	medians[2] = (int)((medians_header[2]&0xf0)>>4 | (medians_header[3]&0x03)<<4);
	quantizations[2] = (int)(medians_header[3]&0x3c)>>2;
	
	/*extract fourth median and fourth quantization bits*/
	medians[3] = (int)((medians_header[3]&0xc0)>>6 | (medians_header[4]&0x0f)<<2);
	quantizations[3] = (int)(medians_header[4]&0xf0)>>4;
	
	/*Convert the bit values of the quantizations to integers*/
	quantizations[0] = compute_quantization(quantizations[0]);
	quantizations[1] = compute_quantization(quantizations[1]);
	quantizations[2] = compute_quantization(quantizations[2]);
	quantizations[3] = compute_quantization(quantizations[3]);
	
}

/**
 * int compute_quantization(int quantizations)
 * 
 * @brief compute the quantization value from bits
 * @param quantizations, byte array
 * @return quantization value
 */ 
int compute_quantization(int quantizations)
{
	/*For quantization, each bit which is true*/
	/*acts has a multiplication factor.*/
	/*The power it carries depend on its position in the byte*/
	/*if bit:power->4:16, 3:8, 2:4, 1:2*/

	int temp = 1;		
	
	/*power of 2*/	
	if(quantizations&0x01)
		temp *= 2;
	
	/*power of 4*/
	if(quantizations&0x02)
		temp *= 4;
	
	/*power of 8*/
	if(quantizations&0x04)
		temp *= 8;
	
	/*power of 16*/
	if(quantizations&0x08)
		temp *= 16;
		
	/*return the result*/
	return temp;
}

/**
 * int compressed_parse_bit_length(char* bit_length_header)
 * 
 * @brief compute the number of bits in the delta stream, from byte values
 * @param bit_length_header, the byte array
 * @return length in bits
 */ 
int compressed_parse_bit_length(unsigned char* bit_length_header)
{
	return (int)(bit_length_header[0]<<8 | (bit_length_header[1]&0xFF));
}


/**
 * int compressed_parse_deltas(char* bits_header, int* median, int* quantization, int* deltas)
 * 
 * @brief extract and compute the deltas from the bit stream
 * https://sites.google.com/a/interaxon.ca/muse-developer-site/muse-communication-protocol/compressed-eeg-packets
 * @param bits_header, the byte array
 * @param median, the medians array
 * @param quantization, the quantization array
 * @param (out)deltas, the deltas array
 * @return length parsed in bits
 */ 
int compressed_parse_deltas(unsigned char* bits_header, int* median, int* quantization, int* deltas)
{
	/*Variables that navigates in loops and in char array*/
	/*loops iterator*/
	int i,j,k,d=0;
	/*location in the byte*/
	int bitshift = 0;
	/*location in the stream of bytes*/
	int byteshift = 0;
	int elias_n = 0;
	/*Byte currently parsed*/
	char cur_byte = 0;
	
	/*Variables for the unitary code*/
	/*condition for the loop counting the number of ones*/
	char quotient_parsed = 0;
	/*parsed values*/
	int quotient_value = 0;
	int remainder_value = 0;
	int sign_value = 0;
	
	/*get the first byte of the stream*/
	cur_byte = bits_header[byteshift];
	
	/*for all 4 channels*/
	for(i=0;i<4;i++){
		
		/*if median is 0, then all deltas are 0s*/
		if(median[i] == 0){
			/*all data for set is 0 (16 diffs in set)*/
			/*skip 16 * 3 bits, Quotient 0, remainder 0, sign 1*/
			/*move to next median/channel*/
			for(j=0;j<16;j++){
				deltas[j+i*16]=0;
			}
			/*shift in the stream*/
			byteshift += 6;
		}
		else
		{
			
			/*for all deltas*/
			for(j=0;j<16;j++){				
					
				/**************************/
				/* Suppose Unary Encoding */
				/**************************/
				/******************/
				/* Quotient       */
				/******************/
				/*parse out the quotient*/
				quotient_parsed = 0;
				quotient_value = 0;
				
				/*count the number of ones before we reach a 0*/
				while(quotient_parsed==0 && quotient_value < 15){
					
					/*check if leftmost bit is a one*/
					if((cur_byte&0x80)){
						/*yes, add one to quotient*/
						quotient_value++;
					}
					else
					{
						/*else, the quotient is parsed*/
						quotient_parsed = 1;
					}
					
					shift_one_bit(&cur_byte,&byteshift,&bitshift, bits_header);
					
				}
				
				/***************************/
				/* Check if ELIAS Encoding */
				/***************************/
				if(quotient_value==15){
					
					/*yes! reset quotient value*/
					quotient_value = 0;
					elias_n = 0;
					
					/*count the number of zeros -> n*/
					while(!(cur_byte&0x80)){
						elias_n++;
						
						shift_one_bit(&cur_byte,&byteshift,&bitshift, bits_header);
					}
					
					/*we found the first one (value of 2^n)*/
					quotient_value += pow(2,elias_n);
					
					shift_one_bit(&cur_byte,&byteshift,&bitshift, bits_header);
						
					/*consider the n following bits*/
					for(k=elias_n;k>0;k--){
						
						if(cur_byte&0x80){
							quotient_value += pow(2,k-1);
						}
						
						shift_one_bit(&cur_byte,&byteshift,&bitshift, bits_header);
					}
				}
				
				/***********************************/
				/* Remainder                       */
				/***********************************/
				
				remainder_value = 0;
				
				/*determine the number of bits used*/
				int maxreminderbits = (int)floor(log2((double)median[i]));// (int)(Math.Log(median[ch]) / Math.Log(2));
				/*The maximium value for not using an extra bit*/
				int max1less = (int)pow(2.0, (double)(maxreminderbits+1)) - median[i];   
				
				/*if median is equal to 1, the equation above didn't provided the correct result*/
				/*this fix it*/
				if(median[i]==1){
					maxreminderbits = 1;
				}
				
				/*Push in the minimum number of bits*/
				for (d = 0; d < maxreminderbits; d++){
					
					/*pack in a bit if needed*/
					remainder_value = remainder_value<<1;
					if(cur_byte&0x80){				
						remainder_value = remainder_value | 0x01;
					}
					
					shift_one_bit(&cur_byte,&byteshift,&bitshift, bits_header);
					
				}
				
				/************************/
				/* Remainder (extra bit)*/
				/************************/
				
				/*check if another remainder bit is required*/
				if (remainder_value >= max1less)  
				{
					/*yes, loop once more*/
					maxreminderbits++;
					
					for (; d < maxreminderbits; d++){
						
						/*packin one more bit*/
						remainder_value = remainder_value << 1;
						if(cur_byte&0x80){
							remainder_value = remainder_value | 0x01;
								
						}
				
						shift_one_bit(&cur_byte,&byteshift,&bitshift, bits_header);
						
					}
					
					/*readjust remainder*/
					remainder_value = remainder_value - max1less;
				}
				
				/************************/
				/* Sign                 */
				/************************/
				
				/*if bit is 1, positive*/
				if(cur_byte&0x80){
					sign_value = -1;
				}
				/*if bit is 0, negative*/
				else{
					sign_value = 1;
				}
							
				shift_one_bit(&cur_byte,&byteshift,&bitshift, bits_header);
				
				/*Compute and store this delta*/
				deltas[j+i*16] = (quotient_value * median[i] + remainder_value) * sign_value * quantization[i];
				
			}
		}
	}
	
	/*compute the number of bits parsed*/
	return byteshift*8+bitshift;
}


/**
 * void parse_uncompressed_packet(char* values_header, int* values)
 * 
 * @brief parse out the eeg signal values out of an uncompressed packet
 * @param values_header, pointer to the beginning of the values in the packet
 * @param (out)values, eeg values parsed out of the packet
 */ 
void parse_uncompressed_packet(unsigned char* values_header, int* values)
{
	/*values are represented in the packet the following way:*/
	/*XXXX XXXX*/
	/*YYYY YYXX*/
	/*XXXX YYYY*/
	/*YYXX XXXX*/
	/*YYYY YYYY*/
	//int i;
	
	memset((void*)values,0,sizeof(int)*4);
		
	/*For each, mask the necessary bits, shift them in place and OR them into an integer*/
	values[0] = (unsigned int)(((values_header[1]&0x03)<<8)|(values_header[0]&0xFF));
	values[1] = (unsigned int)(((values_header[2]&0x0F)<<6)|((values_header[1]&0xFC)>>2));
	values[2] = (unsigned int)(((values_header[3]&0x3F)<<4)|(values_header[2]&0xF0)>>4);
	values[3] = (unsigned int)(((values_header[4]&0xFF)<<2)|((values_header[3]&0xC0)>>6));

}

void shift_one_bit(char* cur_byte, int* byteshift, int* bitshift, unsigned char* bits_header){
	
	/*shift the byte of one space*/
	(*cur_byte) = (*cur_byte)<<1;
	/*count the shift*/
	(*bitshift)++;
		
	/*check if we are due to load the next byte*/
	if((*bitshift) >= 8){
		/*increase byte count*/
		(*byteshift)+=1;
		/*reset the shift*/
		(*bitshift) = 0;
		/*get the next byte*/
		(*cur_byte) = bits_header[*byteshift];
	}
}


/**
 * void print_char_bits(unsigned char v)
 * 
 * @brief prints the bits of a char onscreen
 * @param v, char to be printed
 */ 
void print_char_bits(unsigned char v) {
  int i; // for C89 compatability
  for(i = 7; i >= 0; i--) putchar('0' + ((v >> i) & 1));
}
/**
 * void printbits(unsigned char v)
 * 
 * @brief prints the bits of an int onscreen
 * @param v, char to be printed
 */ 
void print_int_bits(unsigned int v) {
  int i; // for C89 compatability
  for(i = 31; i >= 0; i--) putchar('0' + ((v >> i) & 1));
}
