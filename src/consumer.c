// SPDX-License-Identifier: BSD-3-Clause
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

void consumed_packets_list_init(consumed_packet_list *list) {
	list->packets = malloc(sizeof(consumed_packet) * 20000);
	list->size = 0;
	list->capacity = 20000;
	list->sort_finished = 0;
	pthread_mutex_init(&list->list_mutex, NULL);
}

void consumed_packet_list_add(consumed_packet_list *list, struct so_packet_t *pkt, unsigned long hash, unsigned long timestamp) {
    pthread_mutex_lock(&list->list_mutex);

    list->packets[list->size].pkt = *pkt;
    list->packets[list->size].hash = hash;
    list->packets[list->size].timestamp = timestamp;
    list->size++;

    size_t i = list->size - 1;
    while (i > 0 && list->packets[i].timestamp < list->packets[i - 1].timestamp) {
        consumed_packet aux = list->packets[i];
        list->packets[i] = list->packets[i - 1];
        list->packets[i - 1] = aux;
        i--;
    }

    pthread_mutex_unlock(&list->list_mutex);
}

void consumed_packets_list_destroy(consumed_packet_list *list) {
	free(list->packets);
	pthread_mutex_destroy(&list->list_mutex);
}

void consumed_packets_list_print(consumed_packet_list *list, FILE *out_file) {
	pthread_mutex_lock(&list->list_mutex);
	for (size_t i = 0; i < list->size; i++) {
		fprintf( out_file, "%s %016lx %lu\n", RES_TO_STR(process_packet(&list->packets[i].pkt)), list->packets[i].hash, list->packets[i].timestamp);
	}
	pthread_mutex_unlock(&list->list_mutex);
}

void *consumed_packets_sort_thread(void *arg) {
	consumed_packet_list *list = (consumed_packet_list *)arg;

	pthread_mutex_lock(&list->list_mutex);
	for (size_t i = 0; i < list->size; i++) {
		for (size_t j = i + 1; j < list->size; j++) {
			if (list->packets[i].timestamp > list->packets[j].timestamp) {
				consumed_packet aux = list->packets[i];
				list->packets[i] = list->packets[j];
				list->packets[j] = aux;
			}
		}
	}
	list->sort_finished = 1;
	pthread_mutex_unlock(&list->list_mutex);

	return NULL;
}

void *consumer_thread(void *arg) {
	so_consumer_ctx_t *ctx = (so_consumer_ctx_t *)arg;
	char consume[PKT_SZ];

	while (1) {
		ssize_t size = ring_buffer_dequeue(ctx->producer_rb, consume, PKT_SZ);
		if (size == 0) {
			break;
		}
		struct so_packet_t *pkt = (struct so_packet_t *)consume;

		int process_pkt = process_packet(pkt);
		unsigned long hash_packet = packet_hash(pkt);
		unsigned long packet_timestamp = pkt->hdr.timestamp;

		consumed_packet_list_add(ctx->list, pkt, hash_packet, packet_timestamp);

	}

	return NULL;
}

int create_consumers(pthread_t *tids, int num_consumers, so_ring_buffer_t *rb, const char *out_filename, pthread_t *sort_thread, consumed_packet_list *list) {
	so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
	ctx->producer_rb = rb;
	ctx->out_file = fopen(out_filename, "w");
	if (ctx->out_file == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	consumed_packets_list_init(list);
	ctx->list = list;

	for (int i = 0; i < num_consumers; i++) {
		pthread_create(&tids[i], NULL, consumer_thread, ctx);
	}

	pthread_create(sort_thread, NULL, consumed_packets_sort_thread, ctx->list);

	return num_consumers;
}
