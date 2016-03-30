/**
 * @file muse.c
 * @author Ron Brash (ron.brash@gmail.com) | Fred Simard (fred.simard@atlantsembedded.com)
 *         Atlants Embedded, 2016
 * @brief Implements the OpenBCI interface for the IntelliPi.
 * 
 * 
 * OpenBCI serial protocol described here:
 * http://docs.openbci.com/software/02-OpenBCI_Streaming_Data_Format
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "serial.h"
#include "hardware.h"
#include "debug.h"
#include "main.h"
#include "openbci.h"
#include "xml.h"
#include "data_output.h"

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>

#define STANDARD_HEADER 0xA0

#define DEFAULT_EEG_SCALE 4.5/24/(pow(2,23) - 1)
#define DEFAULT_ACCEL_SCALE 0.002/pow(2,4)
#define NB_EEG_CHANNELS 8
#define NB_ACCEL_CHANNELS 3

#define EEG_CHAN_START_IDX 2
#define EEG_CHAN_INCREMENT 3

#define ACCEL_CHAN_START_IDX 26
#define ACCEL_CHAN_INCREMENT 2


char parse_openbci_packet(unsigned char* packet, float eeg_data[NB_EEG_CHANNELS], float acc_data[NB_ACCEL_CHANNELS]);
int interpret16bitAsInt32(char byteArray[2]);
int interpret24bitAsInt32(char byteArray[3]);

/**
 * openbci_init_hardware()
 * @brief Initializes muse hardware related variables
 */
int openbci_init_hardware(void *param __attribute__ ((unused)))
{
	return (0);
}

/**
 * openbci_init_hardware()
 * @brief Initializes muse hardware related variables
 */
int openbci_connect_dev(void *param)
{
	param_t *param_ptr = (param_t *) param;
	return setup_serial((unsigned char *)param_ptr->ptr);
}

/**
 * openbci_cleanup()
 * @brief Openbci cleanup function
 */
int openbci_cleanup(void *param __attribute__ ((unused)))
{
	param_t param_stop_transmission = { OPENBCI_HALT_TRANSMISSION, 1 };
	openbci_send_pkt(&param_stop_transmission);
	close_serial();
	return (0);
}

/** 
 * openbci_translate_pkt
 * @brief translate MUSE packet
 */
int openbci_translate_pkt(void *packet __attribute__ ((unused)),void *output __attribute__ ((unused)))
{
	// Unused for now
	param_t *packet_ptr __attribute__ ((unused)) = (param_t *) packet;
	return (0);
}

/**
 * send_keep_alive_pkt(void)
 * @brief Sends a keep alive repeatedly while sleeping for the required 
 * time-out period.
 */
int openbci_send_keep_alive_pkt(void *param __attribute__ ((unused)))
{
	// Not used / Not required
	return (0);
}

/**
 * openbci_send_pkt()
 * @brief Sends a keep alive repeatedly while sleeping for the required 
 * time-out period.
 * @param param
 */
int openbci_send_pkt(void *param)
{
	param_t *param_ptr = (param_t *) param;
	int ret __attribute__ ((unused));
	ret = write(get_serial_fd(), param_ptr->ptr, param_ptr->len);
	return (0);
}

/**
 * openbci_process_pkt()
 * @brief Processes the packet
 * @param param
 */
int openbci_process_pkt(void *packet, void* output)
{
	int i;
	int eeg_idx = 2;
	static char show_nxt = 0;
	param_t *packet_ptr = (param_t *) packet;
	output_interface_array_t *output_intrface_array = (output_interface_array_t *) output;
	
	static unsigned char packet_nb; 
	static float data[NB_EEG_CHANNELS + NB_ACCEL_CHANNELS];	
	
	data_t data_struct;
	//data_struct.nb_data = NB_EEG_CHANNELS + NB_ACCEL_CHANNELS;
	data_struct.nb_data = NB_EEG_CHANNELS + NB_ACCEL_CHANNELS;
	data_struct.ptr = data;
	
	fprintf(stdout, "Bytes read = %d\n", packet_ptr->len);
	hexdump((unsigned char *)packet_ptr->ptr, packet_ptr->len);
		
		/*
	if(show_nxt){
		fprintf(stdout, "Bytes read = %d\n", packet_ptr->len);
		hexdump((unsigned char *)packet_ptr->ptr, packet_ptr->len);
		show_nxt = 0;
	}else if(packet_ptr->ptr[1]==0xaf){
		fprintf(stdout, "Bytes read = %d\n", packet_ptr->len);
		hexdump((unsigned char *)packet_ptr->ptr, packet_ptr->len);
		show_nxt = 1;
		return (0);
	}
	*/
	
	if (_TRANS_PKT_FC) {
		
		/*switch-case packet based on header*/
		switch(packet_ptr->ptr[0]){
			
			/*standard packet*/
			case STANDARD_HEADER:
				
				packet_nb = packet_ptr->ptr[1];
				printf("packet number:%d\n",packet_nb);
				fflush(stdout);
				
				/*depacket eeg-acc data*/
				parse_openbci_packet(&(packet_ptr->ptr[eeg_idx]), &(data[0]), &(data[8]));
				/*not need to translate*/
				for(i=0;i<output_intrface_array->nb_output;i++){
					COPY_DATA_IN(output_intrface_array->output_interface[i], &data_struct);
				}
			break;
			
		}
	}

	return (0);
}

/**
 * openbci_read_pkt()
 * @brief Begins the communication with the OpenBCI.
 * 			- Reset the OpenBCI
 * 			- Wait for the $$$
 *          - Start communication
 *                x process each packet
 */
int openbci_read_pkt(void* output)
{

	int bytes_read = 0;

	param_t param_start_transmission = { OPENBCI_START_TRANSMISSION, 1 };
	param_t param_stop_transmission = { OPENBCI_HALT_TRANSMISSION, 1 };
	param_t param_reset_transmission = { OPENBCI_RESET, 1 };
	param_t param_process_pkt = { 0 };
	char buf[255] = { 0 };

	int fd = get_serial_fd();
	int check = 0;
	int num, offset = 0, bytes_expected = 130;

	/********************************/
	/* OpenBCI comms initialization */
	/********************************/

	// If running lets stop the transmission and reset the device
	openbci_send_pkt(&param_stop_transmission);
	openbci_send_pkt(&param_reset_transmission);
	sleep(2);
	// If you care about the $$$ prompt
	do {
		num = read(fd, buf + offset, 255);

		if (buf[num - 1] == '$') {
			check++;
			break;
		}

		offset += num;
		param_process_pkt.ptr = buf;
		param_process_pkt.len = num;
		PROCESS_PKT_FC(&param_process_pkt, output);

	} while (check > 0);

	sleep(1);

	printf("OpenBCI restart check!\n");
	fflush(stdout);

	/********************************/
	/* Everything checks, start!    */
	/********************************/

	// Now restart the communication
	memset(buf, 0, 255);
	openbci_send_pkt(&param_start_transmission);
	
	int samples = 0;
	
	unsigned char header_found = 0x00;
	unsigned char next_packet = 0x00;
	unsigned char this_byte = 0x00;
	unsigned char prev_byte = 0x00;
	
	bytes_expected = DATA_PACKET_LENGTH;
	do {
		
		while(!header_found){
			prev_byte = this_byte;
			if(read(fd, &this_byte, 1)){			
				if(this_byte == next_packet && prev_byte == 0xA0){
					header_found = 0x01;
				}
			}
			
		}
		
		buf[0] = prev_byte;
		buf[1] = this_byte;
		offset = 2;
		this_byte = 0x00;
		prev_byte = 0x00;
		unsigned char take_next = 0;
		
		do {
			if(read(fd, &this_byte, 1)){
				
				if(this_byte==0xFF && take_next==0){
					take_next = 1;
				}else{
					buf[offset]=this_byte;
					offset+=1;
					take_next = 0;
				}
				
			}
		} while ((this_byte!=0xc0 && offset < 255) || offset < 20);


		if (bytes_read < 0) {
			fprintf(stdout, "Error reading socket: %d\n", bytes_read);
			continue;
		}

		param_process_pkt.ptr = buf;
		param_process_pkt.len = offset;

		PROCESS_PKT_FC(&param_process_pkt,output);

		header_found = 0x00;
		next_packet++;

	} while(1);
	printf("Samples taken: %d\n", samples);
	return (0);
}


/**
 * char parse_openbci_packet(unsigned char* packet, float eeg_data[NB_EEG_CHANNELS], float acc_data[NB_ACCEL_CHANNELS])
 * @brief parse the data from the OpenBCI packet
 * @param packet, the buffer containing the packet
 * @param eeg_data (out), eeg data samples
 * @param acc_data (out), accelerometer data samples
 * @return EXIT_SUCCESS 
 */
char parse_openbci_packet(unsigned char* packet, float eeg_data[NB_EEG_CHANNELS], float acc_data[NB_ACCEL_CHANNELS]){
	
	int i;
	int idx = 0;
	
	/*	
	 * Bytes 3-5: Data value for EEG channel 1
	 * Bytes 6-8: Data value for EEG channel 2
	 * Bytes 9-11: Data value for EEG channel 3
	 * Bytes 12-14: Data value for EEG channel 4
	 * Bytes 15-17: Data value for EEG channel 5
	 * Bytes 18-20: Data value for EEG channel 6
	 * Bytes 21-23: Data value for EEG channel 6
	 * Bytes 24-26: Data value for EEG channel 8
	 */	
	for(i=0;i<NB_EEG_CHANNELS;i++){
			
		eeg_data[i] = (float)interpret24bitAsInt32(&(packet[idx]))*DEFAULT_EEG_SCALE;
		idx += EEG_CHAN_INCREMENT;
	}

	/*
	 * Bytes 27-28: Data value for accelerometer channel X
	 * Bytes 29-30: Data value for accelerometer channel Y
	 * Bytes 31-32: Data value for accelerometer channel Z
	 */
	//idx = ACCEL_CHAN_START_IDX;
	for(i=0;i<NB_ACCEL_CHANNELS;i++){
		
		if(!(packet[idx] == 0 && packet[idx+1] == 0)){
			//acc_data[i] = (float)interpret16bitAsInt32(&(packet[idx]))*DEFAULT_ACCEL_SCALE;
			acc_data[i] = 0;
		}
		//idx += ACCEL_CHAN_INCREMENT;
	}
		
	return EXIT_SUCCESS;
}


/**
 * int interpret24bitAsInt32(char byteArray[3])
 * @brief interpret the 24 bits as a 32 bits array
 * @param byteArray, array of 3 bytes (24 bits)
 * @return 32 bits integer
 */
int interpret24bitAsInt32(char byteArray[3]) {     
	int newInt = (  
		((0xFF & byteArray[0]) << 16) |  
		((0xFF & byteArray[1]) << 8) |   
		(0xFF & byteArray[2])  
    );
      
	if ((newInt & 0x00800000) > 0) {  
		newInt |= 0xFF000000;  
	} else {  
		newInt &= 0x00FFFFFF;  
	}  
	return newInt;  
}  


/**
 * int interpret16bitAsInt32(char byteArray[2])
 * @brief interpret the 16 bits as a 32 bits array
 * @param byteArray, array of 2 bytes (16 bits)
 * @return 32 bits integer
 */
int interpret16bitAsInt32(char byteArray[2]) {
    int newInt = (
      ((0xFF & byteArray[0]) << 8) |
       (0xFF & byteArray[1])
      );
    if ((newInt & 0x00008000) > 0) {
          newInt |= 0xFFFF0000;
    } else {
          newInt &= 0x0000FFFF;
    }
    return newInt;
  }
  
