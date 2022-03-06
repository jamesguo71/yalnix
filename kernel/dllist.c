#include "dllist.h"
#include <ylib.h>
#include <ykernel.h>

int empty(dllist *l)
{
    if (l == NULL) {
        TracePrintf(1, "empty: dllist l is NULL\n");
    }
    return (l->sentinel_node->next == l->sentinel_node);
}

void list_delete_node(dlnode_t *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    free(node);
}

dlnode_t *first(dllist *l) {
    if (l == NULL) {
        TracePrintf(1, "first: dllist l is NULL\n");
    }
    return l->sentinel_node->next;
}

dlnode_t *sentinel(dllist *l) {
    if (l == NULL) {
        TracePrintf(1, "sentinel: dllist l is NULL\n");
    }
    return l->sentinel_node;
}

dlnode_t *list_insert_before(dlnode_t *node, int id)
{
    if (!node) {
        TracePrintf(1, "list_insert_before: dlnode_t is NULL");
    }
    dlnode_t *newnode;
    dlnode_t *prev_node = node->prev;

    newnode = (dlnode_t *) malloc (sizeof(dlnode_t));
    if (newnode == NULL) {
        TracePrintf(1, "list_insert_before failed!\n");
        return NULL;
    }
    newnode->id = id;

    newnode->next = node;
    newnode->prev = prev_node;
    node->prev = newnode;
    prev_node->next = newnode;

    return newnode;
}

int list_append(dllist *l, int id)
{
    if (!l) {
        TracePrintf(1, "list_append: l is NULL");
    }
    if (list_insert_before(l->sentinel_node, id) == NULL) {
        return ERROR;
    }
    return SUCCESS;
}

dllist *list_new()
{
    dllist *d;
    dlnode_t *node;

    d = (dllist *) malloc (sizeof(dllist));
    if (!d) return NULL;
    node = (dlnode_t *) malloc(sizeof(dlnode_t));
    if (!node) {
        free(d);
        return NULL;
    }

    d->sentinel_node = node;
    node->next = node;
    node->prev = node;
    return d;
}

void list_free(dllist *l)
{
    while (!empty(l)) {
        list_delete_node(first(l));
    }
    free(l->sentinel_node);
    free(l);
}

void list_delete_id(dllist *list, int id) {
    for (dlnode_t *s = first(list); s != sentinel(list); s = s->next) {
        if (s->id == id) {
            list_delete_node(s);
            return;
        }
    }
}

void list_foreach(dllist *list, int (*op)(int)) {
    for (dlnode_t *s = first(list); s != sentinel(list); s = s->next) {
        if (op(s->id) != SUCCESS) {
            TracePrintf(1, "[list_foreach] list processing failed for node with id = %d\n", s->id);
            Halt();
        }
    }
}
