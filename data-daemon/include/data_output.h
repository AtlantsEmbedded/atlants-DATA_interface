#ifndef DATA_OUTPUT_H
#define DATA_OUTPUT_H
/**
 * @file data_output.c
 * @author Frederic Simard, Atlants Embedded (fred.simard@atlantsembedded.com)
 * @brief Configures the data output interface. Sets the function pointers according
 *        to user options.
 */

#define INIT_DATA_OUTPUT_FC(param) \
		_INIT_DATA_OUTPUT_FC(param)
		
#define COPY_DATA_IN(param, input) \
		_COPY_DATA_IN(param, input)
		
#define TERMINATE_DATA_OUTPUT_FC(param) \
		_TERMINATE_DATA_OUTPUT_FC(param)

/*defines the function pointers associated with the data output*/
typedef int (*functionPtr_t) (void *);
typedef void* (*initfunctionPtr_t) (void *);
typedef int (*inputfunctionPtr_t) (void *,void *);
initfunctionPtr_t _INIT_DATA_OUTPUT_FC;
inputfunctionPtr_t _COPY_DATA_IN;
functionPtr_t _TERMINATE_DATA_OUTPUT_FC;

/*defines the data structure that contains the data to be sent*/
typedef struct data_s {
	float *ptr;
	int nb_data;
} data_t;


/*Structure containing the configuration of the hardware*/
typedef struct csv_options_s {
	
	char* filename;	
	int nb_data_channels;

} csv_options_t;


/*Structure containing the configuration of the hardware*/
typedef struct shm_mem_options_s {

	/*definitions defining key to shared memory and IPC semaphore*/
	int shm_key;
	int sem_key;

	/*definitions affecting output buffer memory requirement and usage*/
	int nb_data_channels;
	int window_size;
	int nb_pages;
	int page_size;
	int buffer_size;
	
} shm_mem_options_t;


/*Structure containing the reference to all output interface*/
/*primarily use if you need to output to SHM and CSV at the same time*/
typedef struct output_interface_array_s {
	int nb_output;
	void** output_interface;
}output_interface_array_t;


/*init function to setup funciton pointers*/
void* init_data_output(char output_type, void* options);


#endif
