/*
Code based on work from rosettacode.org
*/

#include "hull.h"
 
bool ccw(const hull_point *a, const hull_point *b, const hull_point *c) {
    return (b->x - a->x) * (c->y - a->y)
         > (b->y - a->y) * (c->x - a->x);
}
 
int comp(const void *lhs, const void *rhs) {
    hull_point lp = *((hull_point *)lhs);
    hull_point rp = *((hull_point *)rhs);
    if (lp.x < rp.x) return -1;
    if (rp.x < lp.x) return 1;
    return 0;
}
 
void free_node(hull_node *ptr) {
    if (ptr == NULL) {
        return;
    }
 
    free_node(ptr->next);
    ptr->next = NULL;
    free(ptr);
}
 
hull_node* push_back(hull_node *ptr, hull_point data) {
    hull_node *tmp = ptr;
 
    if (ptr == NULL) {
        ptr = (hull_node*)malloc(sizeof(hull_node));
        ptr->data = data;
        ptr->next = NULL;
        return ptr;
    }
 
    while (tmp->next != NULL) {
        tmp = tmp->next;
    }
 
    tmp->next = (hull_node*)malloc(sizeof(hull_node));
    tmp->next->data = data;
    tmp->next->next = NULL;
    return ptr;
}
 
hull_node* pop_back(hull_node *ptr) {
    hull_node *tmp = ptr;
 
    if (ptr == NULL) {
        return NULL;
    }
    if (ptr->next == NULL) {
        free(ptr);
        return NULL;
    }
 
    while (tmp->next->next != NULL) {
        tmp = tmp->next;
    }
 
    free(tmp->next);
    tmp->next = NULL;
    return ptr;
}
 
hull_node* convex_hull(int len, hull_point p[]) {
    hull_node *h = NULL;
    hull_node *hptr = NULL;
    size_t hLen = 0;
    int i;
 
    qsort(p, len, sizeof(hull_point), comp);
 
    /* lower hull */
    for (i = 0; i < len; ++i) {
        while (hLen >= 2) {
            hptr = h;
            while (hptr->next->next != NULL) {
                hptr = hptr->next;
            }
            if (ccw(&hptr->data, &hptr->next->data, &p[i])) {
                break;
            }
 
            h = pop_back(h);
            hLen--;
        }
 
        h = push_back(h, p[i]);
        hLen++;
    }
 
    /* upper hull */
    for (i = len - 1; i >= 0; i--) {
        while (hLen >= 2) {
            hptr = h;
            while (hptr->next->next != NULL) {
                hptr = hptr->next;
            }
            if (ccw(&hptr->data, &hptr->next->data, &p[i])) {
                break;
            }
 
            h = pop_back(h);
            hLen--;
        }
 
        h = push_back(h, p[i]);
        hLen++;
    }
 
    pop_back(h);
    return h;
}