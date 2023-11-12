#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "blue_utils.h"

static device_node_t *head;
static device_node_t *tail;

device_node_t *get_blue_devices() {
  return head;
}

void add_blue_device(char *name, char *address) {
  device_node_t *current = head;

  while (current) {
    printf("Checking device: %s\n", current->device.address);
    if (strcmp(current->device.address, address) == 0) {
      printf("Device already added: %s\n", address);
      return;
    }
    printf("No match\n");
    current = current->next;
  }
  printf("New device: %s\n", name);

  device_node_t *new_node = malloc(sizeof(device_node_t));
  strcpy(new_node->device.name, name);
  strcpy(new_node->device.address, address);
  new_node->next = NULL;

  printf("Adding device: %s\n", name);

  if (!head) {
    head = new_node;
    tail = new_node;
  } else {
    tail->next = new_node;
    tail = new_node;
  }
  printf("Added device: %s\n", address);
}

void clear_all_blue_devices() {
  device_node_t *current = head;
  device_node_t *next;

  while (current) {
    next = current->next;
    free(current);
    current = next;
  }

  head = NULL;
  tail = NULL;
}