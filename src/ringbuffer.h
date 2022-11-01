#ifndef _ringbuffer_h_
#define _ringbuffer_h_

#define NUM_QUEUE 255

struct ring_buffer
{
  char *array[NUM_QUEUE];
  unsigned int tail;
  unsigned int head;
  unsigned int len;
};

void add_item(struct ring_buffer *p, const char *str);
char *get_item_from_top(struct ring_buffer *p);
void remove_item(struct ring_buffer *p);
void cleanup_queue(struct ring_buffer *p);
unsigned int list_of_items_in_queue(struct ring_buffer *p);

#endif