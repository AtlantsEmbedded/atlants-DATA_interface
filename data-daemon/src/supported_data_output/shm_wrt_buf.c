/**
 * @file shm_buf_writer.c
 * @author Frederic Simard, Atlants Embedded (fred.simard@atlantsembedded.com)
 * @brief This file implements the shared memory data output system.
 *        The shared memory is meant to be shared between at least two processes and
 *        takes the form of a circular buffer with several pages. When one page is done
 *        writing a semaphore is set to inform the reader that a page is available to read.
 *        only when the reader confirms that the data has been read is the current process
 *        allowed to write in it.
 * 
 *        When no page is available to write, the current process drops the sample. The number
 *        of pages should be kept as small as possible to prevent processing old data while
 *        dropping newest...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "data_output.h"
#include "shsem_def.h"
#include "shm_wrt_buf.h"

/* arg for semctl system calls. */
union semun {
		int val;                /* value for SETVAL */
		struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
		ushort *array;          /* array for GETALL & SETALL */
		struct seminfo *__buf;  /* buffer for IPC_INFO */
		void *__pad;
};

/**
 * int shm_wrt_init(void *param)
 * @brief Setups the shared memory output (memory allocation, linking and semaphores)
 * @param param, unused
 * @return initialized shm output, NULL otherwise
 */
void* shm_wrt_init(void *param){
	
	union semun semopts;    
	
	/*re-cast param for readability*/
	shm_wrt_t* shm_wrt = (shm_wrt_t*)malloc(sizeof(shm_wrt_t));
	
	/*copy the options*/	        
	memcpy((void*)&(shm_wrt->shm_options),param,sizeof(shm_mem_options_t));	           
		        
    /*initialise the shared memory array*/
	if((shm_wrt->shmid = shmget(shm_wrt->shm_options.shm_key, shm_wrt->shm_options.buffer_size, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        return NULL;
    }
		
    /*Now we attach it to our data space*/
    if ((shm_wrt->shm_buf = shmat(shm_wrt->shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        return NULL;
    }
    
    /*Access the semaphore array*/
	if ((shm_wrt->semid = semget(shm_wrt->shm_options.sem_key, NB_SEM, IPC_CREAT | 0666)) == -1) {
		perror("semget failed\n");
		return NULL;
    } 

	/*set semaphores initial value to 0*/
    semopts.val = 0;
    semctl( shm_wrt->semid, 0, SETVAL, semopts);
	
	/*allocate the memory for the pointer to semaphore operations*/
	shm_wrt->sops = (struct sembuf *) malloc(sizeof(struct sembuf));
	
	/*give initial values to static variables*/
	shm_wrt->samples_count = 0;
	shm_wrt->current_page = 0;
	shm_wrt->page_opened = 0x00;

	return (void*)shm_wrt;
}

/**
 * int shm_wrt_write_in_buf(void *param)
 * @brief Writes the data received to the shared memory. This function makes sure that:
 *        - the page is available
 *        - the data is written at the right place in the page
 *        - the page is changed once its filled
 *        - informs the reader that a page has been filled
 * @param param, refers to a data_t pointer, which contains the data to be written
 * @return EXIT_FAILURE for unknown type, EXIT_SUCCESS for known/success
 */
int shm_wrt_write_in_buf(void *param, void* input){
	
	int write_ptr;
	
	/*re-cast param for readability*/
	shm_wrt_t* shm_wrt = (shm_wrt_t*)param;
	
	data_t* data = (data_t *) input; 
	
	/*check if the page is not opened*/
	if(!shm_wrt->page_opened){
		/*if not opened*/
		/*check if the current page is available (semaphore)*/
		shm_wrt->sops->sem_num = PREPROC_IN_READY; /*sem that indicates that a page is free to write to*/
		shm_wrt->sops->sem_op = -1; /*decrement semaphore*/
		shm_wrt->sops->sem_flg = IPC_NOWAIT; /*undo if fails and non-blocking call*/	
		if(semop(shm_wrt->semid, shm_wrt->sops, 1) == 0){
			/*yes, open the page*/
			shm_wrt->page_opened = 0x01;
		}
	}
	
	/*if the page is opened*/
	if(shm_wrt->page_opened){
		
		/*compute the write location*/
		write_ptr = shm_wrt->shm_options.page_size*shm_wrt->current_page+shm_wrt->shm_options.nb_data_channels*shm_wrt->samples_count;
		
		/*write data*/
		memcpy((void*)&(shm_wrt->shm_buf[write_ptr]),(void*)data->ptr, data->nb_data*sizeof(float));
				
		shm_wrt->samples_count++;
		
		/*check if the page is full*/
		if(shm_wrt->samples_count>=shm_wrt->shm_options.window_size){
		
			/*close the page*/
			shm_wrt->page_opened = 0x00;
			/*change the page*/
			shm_wrt->current_page = (shm_wrt->current_page+1)%shm_wrt->shm_options.nb_pages;
			/*reset nb of samples read*/
			shm_wrt->samples_count = 0; 
			
			/*post the semaphore*/
			shm_wrt->sops->sem_num = INTERFACE_OUT_READY;  /*sem that indicates that a page has been written to*/
			shm_wrt->sops->sem_op = 1; /*increment semaphore of one*/
			shm_wrt->sops->sem_flg = IPC_NOWAIT; /*undo if fails and non-blocking call*/
			semop(shm_wrt->semid, shm_wrt->sops, 1);
		}
		
	}
	else{
		/*else drop the sample (do nothing)*/
	}
	
	return EXIT_SUCCESS;
}

/**
 * int shm_cleanup((void *param)
 * @brief Clean up the shared memory: detach, deallocate mem and sem
 * @param param, unused
 * @return EXIT_FAILURE for unknown type, EXIT_SUCCESS for known/success
 */
int shm_wrt_cleanup(void *param){
	
	/*re-cast param for readability*/
	shm_wrt_t* shm_wrt = (shm_wrt_t*)param;
	
	/* Detach the shared memory segment. */
	shmdt(shm_wrt->shm_buf);
	/* Deallocate the shared memory segment. */
	shmctl(shm_wrt->shmid, IPC_RMID, 0);
	/* Deallocate the semaphore array. */
	semctl(shm_wrt->semid, 0, IPC_RMID, 0);
	
	return EXIT_SUCCESS;
}
