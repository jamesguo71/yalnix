//
// Created by Fei on 3/5/22.
//

#ifndef YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_
#define YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_

typedef struct node {
  int id;
  struct node *prev;
  struct node *next;
} node_t;

typedef struct {
  node_t *sentinel_node;
} dllist;

void delete_node(node_t *node);

node_t *first(dllist *l);

node_t *insert_before(node_t *node, int id);

int append(dllist *l, int id);
dllist *list_new();

void list_free(dllist *l);
void list_print(dllist *list);
int delete_id(dllist *list, int id);
#endif //YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_
