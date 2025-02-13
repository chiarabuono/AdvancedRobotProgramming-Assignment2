#ifndef TARGET_HPP
#define TARGET_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auxfunc2.hpp"

// Macro di configurazione
#define MAX_LINE_LENGTH 100
#define USE_DEBUG 1

// Variabili globali
extern FILE *targFile;

#define LOGNEWMAP(status) {                                                      \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                        \
                                                                                 \
    fprintf(targFile, "%s New target generated.\n", date);                             \
    for (int t = 0; t < MAX_TARGET; t++) {                                       \
        fprintf(targFile, "(%d, %d) ",                                  \
                status.targets.x[t], status.targets.y[t]); \
    }                                                                            \
    fprintf(targFile, "\n");                                                      \
    fflush(targFile);                                                             \
}


#define LOGPROCESSDIED() { \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        raise(SIGTERM);                                                                  \
    }                                                                            \
                                                                                 \
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date));                                           \
    fprintf(targFile, "%s Process dead. Obstacle is quitting\n", date);                              \
    fflush(targFile);                                                             \
}

#define LOGSUBSCRIPTION(current_count_change) {
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
    }\
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date)); \
    if (current_count_change == 1) { \
        fprintf(targFile, "%s Subscription matched\n", date); \
    } else if (current_count_change == -1) {    \
        fprintf(tagFile, "%s Subscription un-matched\n", date); \
    } else { \
        fprintf(targFile, "%s %d is not a valid value for SubscriptionMatchedStatus current count change\n", date, current_count); \
    } \
    fflush(targFile); \    
}


#define LOGPUBLISHERMATCHING(current_count_change) {
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
    }\
    char date[50];                                                               \
    getFormattedTime(date, sizeof(date)); \
    if (current_count_change == 1) { \
        fprintf(targFile, "%s Target Publisher matched\n", date); \
    } else if (current_count_change == -1) {    \
        fprintf(tagFile, "%s Target Publisher unmatched.\n", date); \
    } else { \
        fprintf(targFile, "%s %d is not a valid value for PublicationMatchedStatus current count change\n", date, current_count); \
    } \
    fflush(targFile); \    
}
#if USE_DEBUG
#define LOGPUBLISHNEWTARGET(targets) {     \
    if (!targFile) {                                                              \
        perror("Log file not initialized.\n");                                   \
        }\
    char date[50]; \
    getFormattedTime(date, sizeof(date)); \
    fprintf(targFile, "%s Target published correctly:\n", date); \
    for (int t = 0; t < targets.targets_number(); t++) {                                       \
        fprintf(targFile, "(%d, %d) ", targets_x()[t], targets_y()[t]); \
    }                                                                            \
    fprintf(targFile, "\n");                                                      \
    fflush(targFile);                                                             \
}
#else
#define LOGPUBLISHNEWTARGET(targets) {}
#endif


#endif // TARGET_H