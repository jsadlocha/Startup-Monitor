#ifndef _chardriver_h_
#define _chardriver_h_

#define DEVICE_NAME "startupmon"
#define CLASS_NAME "startupmon"

int create_char_device(void);
void remove_char_device(void);
void add_item_to_queue(const char *str);

#endif