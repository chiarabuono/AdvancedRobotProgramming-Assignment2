#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "auxfunc.h"
#include <signal.h>
#include <math.h>
#include "obstacle.hpp"
#include "obst_publisher.hpp"

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

// management target
#define PERIODO 3

MyObstacles obstacles;
FILE *obstFile = NULL;

int pid;
int fds[4];

void sig_handler(int signo) {
    if (signo == SIGUSR1)
    {
        handler(OBSTACLE);
    }else if(signo == SIGTERM){
        LOGPROCESSDIED() 
        fclose(obstFile);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

int canSpawnPrev(int x_pos, int y_pos) {
    for (int i = 0; i < obstacles.number; i++) {
        if (abs(x_pos - obstacles.x[i]) <= NO_SPAWN_DIST && abs(y_pos - obstacles.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

void createObstacles() {

    int x_pos, y_pos;

    for (int i = 0; i < obstacles.number; i++){
    
        do {
            x_pos = rand() % (WINDOW_LENGTH - 1);
            y_pos = rand() % (WINDOW_WIDTH - 1);
        } while (canSpawnPrev(x_pos, y_pos) == 0);

        obstacles.x[i] = x_pos;
        obstacles.y[i] = y_pos;
    }
}

int main(int argc, char *argv[]) {
   
    fdsRead(argc, argv, fds);

    // Opening log file
    obstFile = fopen("log/obstacle.log", "a");
    if (obstFile == NULL) {
        perror("Errore nell'apertura del obstFile");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a', 1, 'o');

    // Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    signal(SIGUSR1, sig_handler);
    signal(SIGTERM, sig_handler);

    for( int i = 0; i < MAX_OBSTACLES; i++){
        obstacles.x[i] = 0;
        obstacles.y[i] = 0;
    }

    obstacles.number = 10;
    
    // Create the publisher
    ObstaclePublisher myPublisher;

    // Initialize the publisher
    if (!myPublisher.init()){      

        //------------------
        //  TO LOG
        //------------------
         
        //std::cerr << "Error initializing the publisher." << std::endl;
        return 1;
    }

    while (1) {

        createObstacles();
        myPublisher.publish(obstacles);
        sleep(PERIODO);
    }
}


