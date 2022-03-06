#include "../kernel/dllist.h"

int main() {
    dllist *list = list_new();
    for (int i = 0; i < 10; i++)
        append(list, i);
    list_print(list);
    delete_id(list, 2);
    delete_id(list, 8);
    list_print(list);
    append(list, 11);
    append(list, 100);
    list_print(list);
}