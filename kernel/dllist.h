#ifndef YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_
#define YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_

typedef struct dlnode {
  int id;
  struct dlnode *prev;
  struct dlnode *next;
} dlnode_t;

typedef struct {
  dlnode_t *sentinel_node;
} dllist;

dllist *list_new();
void list_free(dllist *l);
int list_append(dllist *l, int id);
void list_delete_id(dllist *list, int id);
void list_foreach(dllist *list, int (*op)(int));
#endif //YALNIX_FRAMEWORK_YALNIX_KERNEL_DLLIST_H_
