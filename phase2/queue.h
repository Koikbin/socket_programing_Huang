#include <stdlib.h>
typedef struct queue
{
    int size;
    struct thread_node* head;
    struct thread_node* rear;
} queue;

typedef struct thread_node
{
    void* data;
    struct thread_node* next;
} thread_node;

void pop(queue* q){
    if(q->size == 0){
        return;
    }
    thread_node* head = q->head;
    q->head = q->head->next;
    q->size--;
}

void push(queue* q, void* data){
    thread_node* new_node = (thread_node*)malloc(sizeof(thread_node));
    new_node->data = data;
    new_node->next = 0;
    if(q->size == 0){
        q->head = new_node;
        q->rear = new_node;
        q->size++;
        return;
    }
    q->rear->next = new_node;
    q->size++;
}

void* peek(queue q){
    if(q.size == 0){
        return 0;
    }
    return q.head->data;
}

void clear(queue* q){
    while(q->size != 0){
        pop(q);
    }
}