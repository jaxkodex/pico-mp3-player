#include "events.h"

#include "pico/util/queue.h"

static queue_t event_queue;

void q_init() {
  queue_init(&event_queue, sizeof(event_t), 7);
}

void q_emit(event_t *event) {
  queue_try_add(&event_queue, event);
}

void q_next_event(event_t *event) {
  queue_try_remove(&event_queue, event);
}