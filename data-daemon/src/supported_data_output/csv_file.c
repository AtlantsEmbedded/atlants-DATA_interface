/**
 * @file csv_file.c
 * @author Frederic Simard, Atlants Embedded (frederic.simard.1@outlook.com)
 * @brief Handles the CSV file function pointers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_output.h"
#include "csv_file.h"


typedef struct csv_file_s{
	csv_options_t options;
	FILE* fp;
}csv_file_t;

/**
 * int csv_init_file(void *param)
 * @brief Sends a keep alive repeatedly while sleeping for the required 
 * time-out period.
 * @param param (unused)
 * @return if successful, EXIT_SUCCESS, otherwise, EXIT_FAILURE
 */
void* csv_init_file(void *param){
	
	/*allocate memory*/
	csv_file_t* csv_file = (csv_file_t*)malloc(sizeof(csv_file_t));
	
	/*copy the options*/
	memcpy(&(csv_file->options),param,sizeof(csv_options_t));
	
	/*open a read/write file*/
	csv_file->fp = fopen("eeg_data.csv","w+");
	
	/*check if opened correctly*/
	if(csv_file->fp==NULL)
		return NULL;
		
	return csv_file;	
}

/**
 * int csv_write_in_file(void *param)
 * @brief Writes the data received in a csv file, skipping a line at the end
 * @param param, must be pointing to a data_t
 * @return if successful, EXIT_SUCCESS, otherwise, EXIT_FAILURE
 */
int csv_write_in_file(void *csv_file_ptr, void *input){
	
	int i;
	data_t *data = (data_t *) input;
	csv_file_t* csv_file = (csv_file_t*)csv_file_ptr;
	
	if(csv_file->fp==NULL)
		return EXIT_FAILURE;
	
	for(i=0;i<data->nb_data;i++){
		fprintf(csv_file->fp,"%.4f;",data->ptr[i]);
	}

	/*skip a line*/
	fprintf(csv_file->fp,"\n");
	
	return EXIT_SUCCESS;
}

/**
 * int csv_close_file(void *param)
 * @brief Close the csv file
 * @param param (unused)
 * @return if successful, EXIT_SUCCESS, otherwise, EXIT_FAILURE
 */
int csv_close_file(void *csv_file_ptr){

	csv_file_t* csv_file = (csv_file_t*)csv_file_ptr;
	
	if(csv_file->fp==NULL)
		return EXIT_FAILURE;
		
	fclose(csv_file->fp);
	free(csv_file);
	return EXIT_SUCCESS;
}
