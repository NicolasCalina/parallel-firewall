#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include "ring_buffer.h"

int ring_buffer_init(so_ring_buffer_t *ring, size_t capacity) {
    ring->data = malloc(capacity);

    ring->cap = capacity;
    ring->len = 0;
    ring->write_pos = 0;
    ring->read_pos = 0;
    atomic_init(&ring->producer_done, 0);

    pthread_mutex_init(&ring->rb_mutex, NULL);
    sem_init(&ring->semEmpty, 0, capacity / 256);
    sem_init(&ring->semFull, 0, 0);

    return 1;
}

void ring_buffer_destroy(so_ring_buffer_t *ring) {
    free(ring->data);
    pthread_mutex_destroy(&ring->rb_mutex);
    sem_destroy(&ring->semEmpty);
    sem_destroy(&ring->semFull);
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size ) {
    sem_wait(&ring->semEmpty);
    pthread_mutex_lock(&ring->rb_mutex);
	
    memcpy(ring->data + ring->write_pos, data, size);
    ring->write_pos += size;
    ring->len+= size;

    if (ring->write_pos >= ring->cap) {
        ring->write_pos %= ring->cap;
    }
	//printf("Enqueued packet, write_pos: %zu, len: %zu\n", ring->write_pos, ring->len);

    pthread_mutex_unlock(&ring->rb_mutex);
    sem_post(&ring->semFull);

    return size;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size) {
    sem_wait(&ring->semFull);
    pthread_mutex_lock(&ring->rb_mutex);

    if (ring->len == 0 && ring->producer_done == 1) {
        pthread_mutex_unlock(&ring->rb_mutex);
		sem_post(&ring->semFull);
        return 0;
    }


    memcpy(data, ring->data + ring->read_pos, size);
    ring->read_pos += size;
    ring->len-= size;

    if (ring->read_pos >= ring->cap) {
        ring->read_pos %= ring->cap;
    }

	//printf("Dequeued packet, read_pos: %zu, len: %zu\n", ring->read_pos, ring->len);

    pthread_mutex_unlock(&ring->rb_mutex);
    sem_post(&ring->semEmpty);

    return size;
}

void ring_buffer_stop(so_ring_buffer_t *ring, int num_consumers) {
    atomic_store(&ring->producer_done, 1);

	for (int i = 0; i < num_consumers; i++) {
		sem_post(&ring->semFull);
	}
}