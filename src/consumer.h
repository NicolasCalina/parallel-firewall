/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include "ring_buffer.h"
#include "packet.h"
#include <pthread.h>


typedef struct consumed_packet {
	struct so_packet_t pkt;
	unsigned long hash;
	unsigned long timestamp;
} consumed_packet;

typedef struct consumed_packet_list {
	consumed_packet *packets;
	size_t size;
	size_t capacity;
	pthread_mutex_t list_mutex;
	int sort_finished;
} consumed_packet_list;

typedef struct so_consumer_ctx_t {
    struct so_ring_buffer_t *producer_rb;
    FILE *out_file;
	consumed_packet_list *list;
} so_consumer_ctx_t;

int create_consumers(pthread_t *tids,
                     int num_consumers,
                     so_ring_buffer_t *rb,
                     const char *out_filename,
					 pthread_t *sort_thread,
					 consumed_packet_list *list);

void consumed_packets_list_init(consumed_packet_list *list);
void consumed_packet_list_add(consumed_packet_list *list, struct so_packet_t *pkt, unsigned long hash, unsigned long timestamp);
void consumed_packets_list_destroy(consumed_packet_list *list);
void consumed_packets_list_print(consumed_packet_list *list, FILE *out_file);
void *consumed_packets_sort(consumed_packet_list *list);

#endif /* __SO_CONSUMER_H__ */