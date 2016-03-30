/**
 * @file main.c
 * @author Ron Brash (ron.brash@gmail.com), Frederic Simard (fred.simard@atlantsembedded.com) | Atlants Embedded 2015
 * @brief Handles the data interface layer which acts as an abstraction
 * interface for BLE/Bluetooth (or other) data and converts data into a usable format
 * for other applications. The data can be outputed to a shared memory buffer and or and CSV file
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "main.h"
#include "socket.h"
#include "serial.h"
#include "app_signal.h"
#include "hardware.h"
#include "data_output.h"
#include "xml.h"
#include "ipc_status_comm.h"
#include "debug.h"

#define CONFIG_NAME "config/data_config.xml"

#define DEBUG 0

unsigned char system_alive = 0x01;
void app_cleanup(void);

/**
 * which_config(int argc, char **argv)
 * @brief return which config to use
 * @param argc
 * @param argv
 * @return string of config
 */
inline char *which_config(int argc, char **argv)
{
	if (argc == 2) {
		return argv[1];
	} else {
		return CONFIG_NAME;
	}
}

ipc_comm_t ipc_comm;
output_interface_array_t output_interface_array;
void* dataout_interface;

/**
 * main()
 * @brief Application main running loop
 * Init signal handler
 * Read xml configuration
 * Init interprocess communication
 * Pair with the hardware
 * Init data output
 */
int main(int argc __attribute__ ((unused)), char **argv __attribute__ ((unused)))
{
	param_t param_ptr = { 0 };
	pthread_t readT, writeT;
	int iret1 __attribute__ ((unused)), iret2 __attribute__ ((unused)), ret = 0, attempts = 0;

	/*Set up ctrl c signal handler*/
	(void)signal(SIGINT, ctrl_c_handler);

	/*read the config from the xml*/
	appconfig_t *config = (appconfig_t *) xml_initialize(which_config(argc, argv));
	if (config == NULL) {
		printf("Error initializing XML configuration\n");
		return (-1);
	}
	
	/*init inter-process status communication channel*/
	ipc_comm.sem_key=config->sem_key;
	ipc_comm_init(&ipc_comm);

	/*init the hardware*/
	if (init_hardware((char *)config->device) < 0) {
		printf("Error initializing hardware");
		return (-1);
	}

	/*init the data output*/
	dataout_interface = init_data_output(config);
	if (dataout_interface==NULL){
		printf("Error initializing data output");
		return (-1);
	}
	
	/*initialize the output array*/
	output_interface_array.nb_output = 1;
	output_interface_array.output_interface = (void**)malloc(sizeof(void*)*1);
	output_interface_array.output_interface[0] = dataout_interface;
	
	/*will try to pair indefinitely*/
	param_ptr.ptr = (void *)get_appconfig()->remote_addr;
	for (;;) {
		
		printf("Data interface->Searching for hardware...\n");
		
		attempts++;
		
		if ((ret = DEVICE_CONNECTION_FC(&param_ptr)) == 0){
			break;
		}
		sleep(1);
	}
	printf("Data interface->Hardware found...\n");
	
	/*tell app that hardware is present*/
	ipc_tell_hardware_is_on(&ipc_comm);
	
	/*if everything is fine so far*/
	if (ret == 0) {

		/*init the thread that picks up the bluetooth packets*/
		iret1 = pthread_create(&readT, NULL, (void *)_RECV_PKT_FC, (void*)&output_interface_array);

		/*if keep_alive*/
		if (get_appconfig()->keep_alive) {
			/*init the thread that implements the watchdog*/
			iret2 = pthread_create(&writeT, NULL, (void *)_KEEP_ALIVE_FC, NULL);
			pthread_join(writeT, NULL);
		}
		pthread_join(readT, NULL);

	} else {
		printf("Unable to connect to hardware\n");
	}

	return 0;
}


void app_cleanup(void)
{
	
	printf("Cleaning up!\n");
	fflush(stdout);
	/*clean up*/
	ipc_comm_cleanup(&ipc_comm);
	DEVICE_CLEANUP_FC();
	TERMINATE_DATA_OUTPUT_FC(dataout_interface);
	free(output_interface_array.output_interface);
}
