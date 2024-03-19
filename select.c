#include "select.h"

// Define the structure for a node in the linked list
struct Node {
    int data;
    struct Node* next;
};

typedef struct _select_struct{
    fd_set rfds;            // Set of file descriptors to monitor for read readiness
    fd_set dirty_rfds;      // Set of file descriptors that are ready for read operations
    struct Node *head;      // Head pointer of the linked list containing file descriptors
    int max;                // Maximum file descriptor value in the linked list
} Select;

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

// Function to initialize a new Select structure
Select *newSelect(){
    Select *new = (Select*)calloc(1, sizeof(Select));
    FD_ZERO(&(new->rfds));     // Initialize the set of file descriptors to zero
    FD_ZERO(&(new->dirty_rfds));   // Initialize the set of dirty file descriptors to zero
    return new;
}

// Function to add a file descriptor to the Select structure
void addFD(Select *s, int fd){
    addNode(&(s->head), fd);   // Add the file descriptor to the linked list
    FD_SET(fd, &(s->rfds));    // Add the file descriptor to the set of file descriptors
    s->max = getMax(s->head);  // Update the maximum file descriptor value
    return;
}

// Function to remove a file descriptor from the Select structure
void removeFD(Select *s, int fd){
    FD_CLR(fd, &(s->rfds));    // Remove the file descriptor from the set of file descriptors
    removeNode(&(s->head), fd);    // Remove the file descriptor from the linked list
}

// Function to check if a file descriptor is ready for read operations
int checkFD(Select *s, int fd){
    if(FD_ISSET(fd, &(s->dirty_rfds)) != 0){
        return 1;   // Return 1 if the file descriptor is ready
    }else return 0; // Otherwise, return 0
}

// Function to free memory allocated for the Select structure
void freeSelect(Select *s){
    freeList(s->head);  // Free memory allocated for the linked list of file descriptors
    free(s);    // Free memory allocated for the Select structure
}

// Function to listen for file descriptors with a specified timeout
int listenSelect(Select *s, int timeout){
    struct timeval tout;
    s->dirty_rfds = s->rfds;    // Copy the set of file descriptors to the set of dirty file descriptors
    if(timeout == -1){
        return select(s->max +1, &(s->dirty_rfds), NULL, NULL, NULL); // Listen indefinitely if timeout is -1
    }
    tout.tv_sec = timeout;  // Set the timeout value in seconds
    tout.tv_usec = 0;       // Set the timeout value in microseconds to zero

    return select(s->max +1, &(s->dirty_rfds), NULL, NULL, &tout);  // Listen for file descriptors with the specified timeout
}