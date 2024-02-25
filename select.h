#include <sys/select.h>
#include <time.h>
#include <stdlib.h>

/**
 * @brief This new structure will handle the file descriptor to be listened to.
 */
typedef struct _select_struct Select;

/**
 * @brief Creates a new structure that will handle the file descriptors to listen to
 * 
 * @return Select* pointer to structure
 */
Select *newSelect();

/**
 * @brief Adds a new file descriptor to the be listened to.
 * 
 * @param s Pointer to Select structure.
 * @param fd File descriptor to add.
 */
void addFD(Select *s, int fd);

/**
 * @brief Removes a specific file descriptor from the list.
 * 
 * @param s Pointer to Select structure.
 * @param fd File descriptor to remove.
 */
void removeFD(Select *s, int fd);

/**
 * @brief Checks if a file descriptor on the list has been flagged to be read.
 * 
 * @param s Pointer to Select structure.
 * @param fd File descriptor to check.
 * @return int Returns 1 if the fd is ready to be read, 0 otherwise.
 */
int checkFD(Select *s, int fd);

/**
 * @brief Frees the momory allocated to the structure.
 * 
 * @param s Pointer to Select structure.
 */
void freeSelect(Select *s);

/**
 * @brief Will wait until any of the file descriptors listed in s are ready to be read
 * then flags them. You can set a timeout in seconds for the program to be stopped waiting.
 * When any of the fds are ready to be read the select function will unblock.
 * 
 * @warning This is possible xone for the program to be blocked.
 * 
 * @param s Pointer to Select structure.
 * @param timeout Number in seconds the OS can wait for any fds before a timeout. Set as -1 to ignore timeouts
 * @return int 0 if timed-out, -1 in case of error or the number of fds ready to be read
 */
int listenSelect(Select *s, int timeout);