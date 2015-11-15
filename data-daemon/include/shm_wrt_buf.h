#ifndef SHM_BUF_WRITER_H
#define SHM_BUF_WRITER_H
/**
 * @file shm_buf_writer.h
 * @author Frederic Simard, Atlants Embedded (frederic.simard.1@outlook.com)
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
 
 
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
	
typedef struct shm_wrt_s{
	
	shm_mem_options_t shm_options; /*buffer options*/
	int semid; /*id of the shared memory array*/
	int shmid; /*id of semaphore set*/
	int samples_count; /*keeps track of the number of samples that have been written in the page*/
	int current_page; /*keeps track of the page to be written into*/
	char page_opened; /*flags indicate if the page is being written into*/
	char* shm_buf; /*pointer to the beginning of the shared buffer*/
	struct sembuf *sops; /*pointer to operations to perform*/
	
}shm_wrt_t;
 
void* shm_wrt_init(void *param);
int shm_wrt_write_in_buf(void *param, void *input);
int shm_wrt_cleanup(void *param);


#endif
