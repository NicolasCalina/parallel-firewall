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

void consumer_thread(so_consumer_ctx_t *ctx) {
    char consume[PKT_SZ];

    while (1) { 
        ssize_t size = ring_buffer_dequeue(ctx->producer_rb, consume, PKT_SZ);
        if (size == 0) {
            break;
        }
        struct so_packet_t *pkt = (struct so_packet_t *)consume;

        int procces_pkt = process_packet(pkt);
        unsigned long hash_packet = packet_hash(pkt);
        unsigned long packet_timestamp = pkt->hdr.timestamp;

        fprintf(ctx->out_file, "%s %016lx %lu\n", RES_TO_STR(procces_pkt), hash_packet, packet_timestamp);
    }
}

int create_consumers(pthread_t *tids, int num_consumers, so_ring_buffer_t *rb, const char *out_filename) {
    so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
    ctx->producer_rb = rb;
    ctx->out_file = fopen(out_filename, "w");
    if (ctx->out_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&tids[i], NULL, (void *(*)(void *))consumer_thread, ctx);
    }

    return num_consumers;
}