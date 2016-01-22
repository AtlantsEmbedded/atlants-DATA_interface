/**
 * @file ipc_status_comm.c
 * @author Fred Simard (fred.simard@atlantsembedded.com)
 * @brief Interprocess communication lib. This will be changed in the near future to make use of sockets.
 *        It handles the status exchanges and signaling between processes, such has hardware present.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "shsem_def.h"
#include "ipc_status_comm.h"

/* arg for semctl system calls. */
union semun {
		int val;                /* value for SETVAL */
		struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
		ushort *array;          /* array for GETALL & SETALL */
		struct seminfo *__buf;  /* buffer for IPC_INFO */
		void *__pad;
};


/**
 * int ipc_comm_init()
 * @brief Init an IPC structure used for signaling
 * @param ipc structure pointer
 * @return EXIT_FAILURE for unknown type, EXIT_SUCCESS for known/success
 */
int ipc_comm_init(ipc_comm_t* ipc_comm){
	
	union semun semopts;    
	
	/*Create or access the semaphore array.*/
	if ((ipc_comm->semid = semget(ipc_comm->sem_key, NB_SEM, IPC_CREAT | 0666)) == -1) {
		perror("semget failed\n");
		return EXIT_FAILURE;
    } 
	
	/*allocate the memory for the pointer to semaphore operations*/
	ipc_comm->sops = (struct sembuf *) malloc(sizeof(struct sembuf));
	
    semopts.val = 0;
    semctl( ipc_comm->semid, INTERFACE_CONNECTED, SETVAL, semopts);
	
	return EXIT_SUCCESS;
}

/**
 * int ipc_tell_hardware_is_on()
 * @brief Inform the application that the hardware is present and ready.
 * @param ipc structure pointer
 * @return EXIT_FAILURE for unknown type, EXIT_SUCCESS for known/success
 */
int ipc_tell_hardware_is_on(ipc_comm_t* ipc_comm){
	
	/*check if the current page is available (semaphore)*/
	ipc_comm->sops->sem_num = INTERFACE_CONNECTED; /*sem that indicates that a page is free to write to*/
	ipc_comm->sops->sem_op = 1; /*increment semaphore*/
	ipc_comm->sops->sem_flg = SEM_UNDO | IPC_NOWAIT; /*undo if fails and non-blocking call*/	
	
	/*post the operation*/
	semop(ipc_comm->semid, ipc_comm->sops, 1);
	
	return EXIT_SUCCESS;
	
}

/**
 * int ipc_comm_cleanup()
 * @brief Cleans up the IPC structure
 * @param ipc structure pointer
 * @return EXIT_FAILURE for unknown type, EXIT_SUCCESS for known/success
 */
int ipc_comm_cleanup(ipc_comm_t* ipc_comm){
	free(ipc_comm->sops);
	semctl(ipc_comm->semid, INTERFACE_CONNECTED, IPC_RMID, 0);
	return EXIT_SUCCESS;
}
