#ifndef __SO_PRODUCER_H__
#define __SO_PRODUCER_H__

#include "ring_buffer.h"

void publish_data(so_ring_buffer_t *rb, const char *filename, int num_consumers);

#endif /* __SO_PRODUCER_H__ */