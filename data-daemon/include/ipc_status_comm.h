#ifndef IPC_STATUS_COMM_H
#define IPC_STATUS_COMM_H

typedef struct ipc_comm_s{
	/*to be set before initialization*/
	int sem_key;
	/*will be set during initialization*/
	int semid;
	struct sembuf *sops;
}ipc_comm_t;

int ipc_comm_init(ipc_comm_t* ipc_comm);
int ipc_tell_hardware_is_on(ipc_comm_t* ipc_comm);
int ipc_comm_cleanup(ipc_comm_t* ipc_comm);




#endif
