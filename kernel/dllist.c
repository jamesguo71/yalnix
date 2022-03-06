#include "dllist.h"

#include <ylib.h>
#include <ykernel.h>

int empty(dllist *l);
void delete_node(node_t *node);
node_t *first(dllist *l);
node_t *sentinel(dllist *l);
node_t *insert_before(node_t *node, int id);
int append(dllist *l, int id);

dllist *list_new();
void list_free(dllist *l);

// Doubly linked list, modified from http://web.eecs.utk.edu/~bvanderz/teaching/cs140Fa10/notes/Dllists/
int empty(dllist *l)
{
    if (l == NULL) {
        TracePrintf(1, "empty: dllist l is NULL\n");
    }
    return (l->sentinel_node->next == l->sentinel_node);
}

void delete_node(node_t *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    free(node);
}

node_t *first(dllist *l) {
    if (l == NULL) {
        TracePrintf(1, "first: dllist l is NULL\n");
    }
    return l->sentinel_node->next;
}

node_t *sentinel(dllist *l) {
    if (l == NULL) {
        TracePrintf(1, "sentinel: dllist l is NULL\n");
    }
    return l->sentinel_node;
}

node_t *insert_before(node_t *node, int id)
{
    if (!node) {
        TracePrintf(1, "insert_before: node_t or pcb is NULL");
    }
    node_t *newnode;
    node_t *prev_node = node->prev;

    newnode = (node_t *) malloc (sizeof(node_t));
    if (newnode == NULL) {
        TracePrintf(1, "insert_before failed!\n");
        return NULL;
    }
    newnode->id = id;

    newnode->next = node;
    newnode->prev = prev_node;
    node->prev = newnode;
    prev_node->next = newnode;

    return newnode;
}

int append(dllist *l, int id)
{
    if (!l) {
        TracePrintf(1, "append: l is NULL");
    }
    if (insert_before(l->sentinel_node, id) == NULL) {
        return ERROR;
    }
    return SUCCESS;
}

dllist *list_new()
{
    dllist *d;
    node_t *node;

    d = (dllist *) malloc (sizeof(dllist));
    node = (node_t *) malloc(sizeof(node_t));
    d->sentinel_node = node;
    node->next = node;
    node->prev = node;
    return d;
}

void list_free(dllist *l)
{
    while (!empty(l)) {
        delete_node(first(l));
    }
    free(l->sentinel_node);
    free(l);
}

void list_print(dllist *list) {
    for (node_t *s = first(list); s != NULL; s = s->next) {
        TracePrintf(1, "%d\n", s->id);
    }
}

int delete_id(dllist *list, int id) {
    for (node_t *s = first(list); s != NULL; s = s->next) {
        if (s->id == id) {
            delete_node(s);
            return 1;
        }
    }
    return 0;
}