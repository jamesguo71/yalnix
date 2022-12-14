#ifndef YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_
#define YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_

typedef struct dlnode {
  int key;
  void *data;
  struct dlnode *prev;
  struct dlnode *next;
} dlnode_t;

typedef struct {
  dlnode_t *sentinel_node;
} dllist;

/* Create a new dllist */
dllist *list_new();

/* Free a dllist and all its nodes*/
void list_free(dllist *l);
/*
 * Appends an id into the dllist
 * return SUCCESS if this operation was done, otherwise return ERROR
 */
int list_append(dllist *l, int key, void *data);
/*
 * Finds an id in the list
 * return a pointer to the node if found, otherwise a NULL pointer
 */
dlnode_t *list_find(dllist *list, int key);
/*
 * Delete the node with the specified id from the dllist
 */
void list_delete_key(dllist *list, int key);
/*
 *
 * list_foreach expects its second parameter to be a function pointer that takes the id of each node
 * as input and returns SUCCESS if the operation on the node's id succeeds
 */
void list_foreach(dllist *list, int (*op)(int));
#endif //YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_
