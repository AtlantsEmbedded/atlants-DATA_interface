/**
 * @file data_output.c
 * @author Frederic Simard, Atlants Embedded (fred.simard@atlantsembedded.com)
 * @brief Configures the data output interface. Sets the function pointers according
 *        to user options.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <csv_file.h>

#include "data_output.h"

#include "shm_wrt_buf.h"
#include "xml.h"

void init_shm_mem_options(appconfig_t *config, shm_mem_options_t* shm_mem_options);
void init_csv_output_options(appconfig_t *config, csv_output_options_t* csv_output_options);

/**
 * int init_data_output(char *output_type)
 * @brief Setup function pointers for the data output
 * @param output_type, identifies the type of output to init
 * @return EXIT_FAILURE for unknown type, EXIT_SUCCESS for known/success
 */
void* init_data_output(appconfig_t *config){

	_INIT_DATA_OUTPUT_FC = NULL;
	_COPY_DATA_IN = NULL;
	_TERMINATE_DATA_OUTPUT_FC = NULL;
		
	/*output to colum separated values (CSV) file (only for debug purpose for now)*/
	if(config->output_format == CSV_OUTPUT) {
		
		csv_output_options_t csv_output_options;
		
		/*set function pointers accordingly*/
		_INIT_DATA_OUTPUT_FC = &csv_init_file;
		_COPY_DATA_IN = &csv_write_in_file;
		_TERMINATE_DATA_OUTPUT_FC = &csv_close_file;
		
		/*init and return*/
		init_csv_output_options(config, &csv_output_options);
		return INIT_DATA_OUTPUT_FC((void*)&csv_output_options);
	}
	/*output to shared memory*/
	else if(config->output_format == SHM_OUTPUT) {
		
		shm_mem_options_t shm_mem_options;
		
		/*set function pointers accordingly*/
		_INIT_DATA_OUTPUT_FC = &shm_wrt_init;
		_COPY_DATA_IN = &shm_wrt_write_in_buf;
		_TERMINATE_DATA_OUTPUT_FC = &shm_wrt_cleanup;
		
		/*init and return*/
		init_shm_mem_options(config, &shm_mem_options);
		return INIT_DATA_OUTPUT_FC((void*)&shm_mem_options);
		
		
	}
	/*Error, wrong type of output*/
	else{
		fprintf(stderr, "Unknown output type\n");
		return NULL;
	}
	return NULL;
}


void init_shm_mem_options(appconfig_t *config, shm_mem_options_t* shm_mem_options){
	
	/*Copy info from xml to dataoutput options structure*/
	shm_mem_options->shm_key = config->shm_key;
	shm_mem_options->sem_key = config->sem_key;
	shm_mem_options->nb_data_channels = config->nb_data_channels;
	shm_mem_options->window_size = config->window_size;
	shm_mem_options->nb_pages = config->nb_pages;
	shm_mem_options->page_size = shm_mem_options->window_size*shm_mem_options->nb_data_channels*sizeof(float);
	shm_mem_options->buffer_size = shm_mem_options->page_size*shm_mem_options->nb_pages;
	
}



void init_csv_output_options(appconfig_t *config, csv_output_options_t* csv_output_options){
	
	strcpy(csv_output_options->filename,"eeg_data.csv");
	csv_output_options->nb_data_channels = config->nb_data_channels;
	csv_output_options->data_type = FLOAT_DATA;
	
}
