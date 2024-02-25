#include "select.h"

// Define the structure for a node in the linked list
struct Node {
    int data;
    struct Node* next;
};

typedef struct _select_struct{
    fd_set rfds;
    fd_set dirty_rfds;
    struct Node *head;
    int max;
}Select;

// Function to create a new node
struct Node* createNode(int data) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to add a new node to the end of the list
void addNode(struct Node** head, int data) {
    struct Node* newNode = createNode(data);
    if (*head == NULL) {
        *head = newNode;
        return;
    }
    struct Node* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;
}

// Function to remove a node from the list
void removeNode(struct Node** head, int data) {
    struct Node* temp = *head;
    struct Node* prev = NULL;

    if (temp != NULL && temp->data == data) {
        *head = temp->next;
        free(temp);
        return;
    }

    while (temp != NULL && temp->data != data) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        //printf("Element not found in the list.\n");
        return;
    }

    prev->next = temp->next;
    free(temp);
}

// Function to get the maximum value from the list
int getMax(struct Node* head) {
    if (head == NULL) {
        //printf("List is empty.\n");
        return -1;
    }
    int max = head->data;
    while (head != NULL) {
        if (head->data > max) {
            max = head->data;
        }
        head = head->next;
    }
    return max;
}

// Function to free the memory allocated for the list
void freeList(struct Node* head) {
    struct Node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

Select *newSelect(){
    Select *new = (Select*)calloc(1, sizeof(Select));
    FD_ZERO(&(new->rfds)); FD_ZERO(&(new->dirty_rfds));
    return new;
}

void addFD(Select *s, int fd){
    addNode(&(s->head), fd);
    FD_SET(fd, &(s->rfds));
    s->max = getMax(s->head);
    return;
}

void removeFD(Select *s, int fd){
    FD_CLR(fd, &(s->rfds));
    removeNode(&(s->head), fd);
}

int checkFD(Select *s, int fd){
    if(FD_ISSET(fd, &(s->dirty_rfds)) != 0){
        return 1;
    }else return 0;
}

void freeSelect(Select *s){
    freeList(s->head);
    free(s);
}

int listenSelect(Select *s, int timeout){
    struct timeval tout;
    s->dirty_rfds = s->rfds;
    if(timeout == -1){
        return select(s->max +1, &(s->dirty_rfds), NULL, NULL, NULL);
    }
    tout.tv_sec = timeout;
    tout.tv_usec = 0;

    return select(s->max +1, &(s->dirty_rfds), NULL, NULL, &tout);
}