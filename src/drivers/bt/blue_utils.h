#ifndef __BLUE_UTILS_H__
#define __BLUE_UTILS_H__

struct device_node;
typedef struct device_node device_node_t;

typedef struct {
  char name[240];
  char address[18];
} blue_device_t;

typedef struct device_node
{
  blue_device_t device;
  device_node_t *next;
} device_node_t;

device_node_t *get_blue_devices();
void add_blue_device(char *name, char *address);
void clear_all_blue_devices();

#endif