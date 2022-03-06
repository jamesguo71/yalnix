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

dllist *list_new();
void list_free(dllist *l);
int list_append(dllist *l, int id);
void list_delete_id(dllist *list, int id);
void list_foreach(dllist *list, int (*op)(int));
#endif //YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_
