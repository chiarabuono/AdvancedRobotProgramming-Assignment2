
#include <stdio.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include "auxfunc.h"
#include <signal.h>
#include "target.hpp"
#include "targ_publisher.hpp"

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

// management target
#define PERIODT 5

FILE *targFile = NULL;
MyTargets targets;

int pid;
int fds[4]; 

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(TARGET);
    }else if(signo == SIGTERM){
        LOGPROCESSDIED(); 
        fclose(targFile);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    }
}

int canSpawnPrev(int x_pos, int y_pos) {
    for (int i = 0; i < targets.number; i++) {
        if (abs(x_pos - targets.x[i]) <= NO_SPAWN_DIST && abs(y_pos - targets.y[i]) <= NO_SPAWN_DIST) return 0;
    }
    return 1;
}

void createTargets() {
    int x_pos, y_pos;

    for (int i = 0; i < targets.number; i++)
    {
        do {
            x_pos = rand() % (WINDOW_LENGTH - 1);
            y_pos = rand() % (WINDOW_WIDTH - 1);
        } while (canSpawnPrev(x_pos, y_pos) == 0);

        targets.x[i] = x_pos;
        targets.y[i] = y_pos;
        }
}

int main(int argc, char *argv[]) {

    fdsRead(argc, argv, fds);
    
    // Opening log file
    targFile = fopen("log/target.log", "a");
    if (targFile == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    pid = writePid("log/passParam.txt", 'a', 1, 't');

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    //Defining signals
    signal(SIGUSR1, sig_handler);
    signal(SIGTERM,sig_handler);

    for(int i = 0; i < MAX_TARGET; i++){
    targets.x[i] = 0;
    targets.y[i] = 0;
    }

    targets.number = 10;
    
    //-------------------------
    // LEGGERE NUM DA PARAMFILE
    //--------------------------


    // Create the publisher
    TargetPublisher targPub;

    // Initialize the publisher
    if (!targPub.init()){      

        //------------------
        //  TO LOG
        //------------------
         
        //std::cerr << "Error initializing the publisher." << std::endl;
        return 1;
    }

    while (1) {

        createTargets();
        targPub.publish(targets);
        sleep(PERIODT);
    }
}
