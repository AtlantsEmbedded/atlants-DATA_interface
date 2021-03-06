/**
 * @file hardware.c
 * @author Ron Brash (ron.brash@gmail.com), Fred Simard (fred.simard@atlantsembedded.com) | Atlants Embdedded, 2015
 * @brief Initializes generic hardware interface which will assign
 * functions to function pointers to provide a reusable interface 
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "socket.h"
#include "hardware.h"
#include "debug.h"
#include "main.h"
#include "muse.h"
#include "fake_muse.h"
#include "openbci.h"

/**
 * init_hardware()
 * @brief Setup function pointers for hardware interface
 * @param hardware_type
 * @return -1 for unknown type, 0 for known/success
 */
int init_hardware(char *hardware_type)
{
	_INIT_HARDWARE_FC = NULL;
	_KEEP_ALIVE_FC = NULL;
	_SEND_PKT_FC = NULL;
	_RECV_PKT_FC = NULL;
	_TRANS_PKT_FC = NULL;
	_PROCESS_PKT_FC = NULL;
	_DEVICE_CONNECTION_FC = NULL;
	_DEVICE_CLEANUP_FC = NULL;
	
	/*Muse device*/
	if (strcmp(hardware_type, "MUSE") == 0) {
		
		_INIT_HARDWARE_FC = &muse_init_hardware;
		_KEEP_ALIVE_FC = &muse_send_keep_alive_pkt;
		_SEND_PKT_FC = &muse_send_pkt;
		_RECV_PKT_FC = &muse_read_pkt;
		_TRANS_PKT_FC = &muse_translate_pkt;
		_PROCESS_PKT_FC = &muse_process_pkt;
		_DEVICE_CONNECTION_FC = &muse_connect_dev;
		_DEVICE_CLEANUP_FC = &muse_cleanup;
		
	/*OpenBCI device*/
	} else if (strcmp(hardware_type, "OPENBCI") == 0) {

		_INIT_HARDWARE_FC = &openbci_init_hardware;
		_KEEP_ALIVE_FC = &openbci_send_keep_alive_pkt;
		_SEND_PKT_FC = &openbci_send_pkt;
		_RECV_PKT_FC = &openbci_read_pkt;
		_TRANS_PKT_FC = &openbci_translate_pkt;
		_PROCESS_PKT_FC = &openbci_process_pkt;
		_DEVICE_CONNECTION_FC = &openbci_connect_dev;
		_DEVICE_CLEANUP_FC = &openbci_cleanup;

	/*Fake Muse device*/
	} else if (strcmp(hardware_type, "FAKE_MUSE") == 0) {

		_INIT_HARDWARE_FC = &fake_muse_init_hardware;
		_KEEP_ALIVE_FC = &fake_muse_send_keep_alive_pkt;
		_SEND_PKT_FC = &fake_muse_send_pkt;
		_RECV_PKT_FC = &fake_muse_read_pkt;
		_TRANS_PKT_FC = &fake_muse_translate_pkt;
		_PROCESS_PKT_FC = &fake_muse_process_pkt;
		_DEVICE_CONNECTION_FC = &fake_muse_connect_dev;
		_DEVICE_CLEANUP_FC = &fake_muse_cleanup;

	} else {
		fprintf(stderr, "Unknown hardware type\n");
		return (-1);
	}
	
	/*init the hardware and return*/
	return INIT_HARDWARE_FC();
}
