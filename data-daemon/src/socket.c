/**
 * @file socket.c
 * @author Ron Brash (ron.brash@gmail.com)
 * @brief Contains all socket inforamtion for BLE connection for device 
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

static int sock;

/**
 * get_socket_fd()
 * @param Returns socket to be used to connect to device
 * @return sock
 */ 
int get_socket_fd(void) {
	return sock;
}

/**
 * set_socket_fd()
 * @param Sets socket to be used to connect to device
 */ 
void set_socket_fd(int fd) {
	sock = fd;
}

/**
 * setup_socket(char addr_mac[])
 * @brief Sets up socket file descriptor
 * @return 0 for success, -1 for error
 */ 
int setup_socket(unsigned char addr_mac[]) {

	struct sockaddr_rc addr = { 0 };
	//struct sockaddr_rc laddr = { 0 };
	int status;
	int fd = 0;
	//char* my_addr = "5C:F3:70:74:9A:01";

	fd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	
	addr.rc_family = AF_BLUETOOTH;
	//addr.rc_channel = addr_mac[15]%2+1;
	addr.rc_channel = 1;
	str2ba((char *)addr_mac, &addr.rc_bdaddr);
	
	//laddr.rc_family = AF_BLUETOOTH;
	//laddr.rc_channel = addr_mac[15]%2+1;
	//str2ba((char *)my_addr, &laddr.rc_bdaddr);
	
	//if (bind(fd, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
	//	perror("Can't bind RFCOMM socket");
	//		close(fd);
	//	return -1;
	//}
	
	status = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	
	if (status != 0) {
		return (-1);
	}
	
	set_socket_fd(fd);
	return (0);
}
/**
 * close_sockets()
 * @brief Closes file descriptor for socket communication
 */
void close_sockets(void)
{
	close(get_socket_fd());
	fprintf(stdout, "Close sockets\n");
}
