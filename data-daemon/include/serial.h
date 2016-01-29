/**
 * @file serial.h
 * @author Ron Brash (ron.brash@gmail.com)
 * @brief Header for serial functions etc...
 */
int get_serial_fd(void);
void set_serial_fd(int fd);
int setup_serial(unsigned char dev_name[]);
int close_serial();
