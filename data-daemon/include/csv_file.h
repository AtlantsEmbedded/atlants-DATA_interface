#ifndef CSV_FILE_H
#define CSV_FILE_H
/**
 * @file csv_file.h
 * @author Frederic Simard, Atlants Embedded (frederic.simard.1@outlook.com)
 * @brief Handles the CSV file function pointers
 */

void* csv_init_file(void *param);
int csv_write_in_file(void *csv_file_ptr, void *input);
int csv_close_file(void *csv_file_ptr);


#endif
