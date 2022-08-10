#ifndef _fifoqueue_h_
#define _fifoqueue_h_

#define NUM_QUEUE 255

struct FifoQueue
{
  char *array[NUM_QUEUE];
  unsigned int tail;
  unsigned int head;
  unsigned int len;
};

void add_item(struct FifoQueue *p, const char *str);
char *get_item_from_top(struct FifoQueue *p);
void remove_item(struct FifoQueue *p);
void cleanup_queue(struct FifoQueue *p);
unsigned int list_of_items_in_queue(struct FifoQueue *p);

#endif