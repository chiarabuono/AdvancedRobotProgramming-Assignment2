#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include "auxfunc.h"
#include <signal.h>
#include "log.h"
#include "targ_subscriber.hpp"
#include "obst_subscriber.hpp"

// process to whom that asked or received
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define nfds 19

#define HMARGIN 5
#define WMARGIN 5
#define BMARGIN 2
#define SCALERATIO 1

#define PERIODBB 10000  // [us]

int nh, nw;
float scaleh = 1.0, scalew = 1.0;

float second = 1000000;

int pid;

int fds[4][4] = {0};
int mode = PLAY;

WINDOW * win;
WINDOW * map;

FILE *file = NULL;
FILE *logFile = NULL;


Drone_bb prevDrone = {0, 0, 0, 0, 0, 0};

Message status;
Message msg;
inputMessage inputMsg;
inputMessage inputStatus;

TargetSubscriber targSub;
ObstacleSubscriber obstSub;

int pids[6] = {0};  // Initialize PIDs to 0

int collision = 0;
int targetsHit = 0;



void storePreviousPosition(Drone_bb *drone) {

    prevDrone.x = drone ->x;
    prevDrone.y = drone ->y;
}

void resizeHandler(){
    getmaxyx(stdscr, nh, nw);  /* get the new screen size */
    scaleh = ((float)(nh - 2) / (float)WINDOW_LENGTH);
    scalew = (float)nw / (float)WINDOW_WIDTH;
    endwin();
    initscr();
    start_color();
    curs_set(0);
    noecho();
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
}

void mapInit(FILE *file){

    // read initial drone position
    readMsg(fds[DRONE][askrd], &status,
                "[BB] Error reading drone position\n", file);
    LOGDRONEINFO(status.drone);
    
    while (!targSub.hasNewData() || !obstSub.hasNewData()){
        usleep(100000);
    }

    status.targets = targSub.getMyTargets();
    status.obstacles = obstSub.getMyObstacles();

    //Update drone position
    writeMsg(fds[DRONE][recwr], &status, 
            "[BB] Error sending updated map\n", file);
    
    LOGDRONEINFO(status.drone);

    inputStatus.droneInfo = status.drone;
    inputStatus.msg = 'B';
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "Error sending ack", file);
}

void drawDrone(WINDOW * win){
    int row = (int)(status.drone.y * scaleh);
    int col = (int)(status.drone.x * scalew);
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(1));   
    mvwprintw(win, row - 1, col, "|");     
    mvwprintw(win, row, col + 1, "--");
    mvwprintw(win, row, col, "+");
    mvwprintw(win, row + 1, col, "|");     
    mvwprintw(win, row , col -2, "--");
    wattroff(win, COLOR_PAIR(1)); 
    wattroff(win, A_BOLD); // Attiva il grassetto 
}

void drawObstacle(WINDOW * win){
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(2)); 
    for(int i = 0; i < status.obstacles.number; i++){
        mvwprintw(win, (int)(status.obstacles.y[i]*scaleh), (int)(status.obstacles.x[i]*scalew), "0");
    }
    wattroff(win, COLOR_PAIR(2)); 
    wattroff(win, A_BOLD); // Attiva il grassetto 
}

void drawTarget(WINDOW * win) {
    wattron(win, A_BOLD); // Attiva il grassetto
    wattron(win, COLOR_PAIR(3)); 
    for(int i = 0; i < status.targets.number; i++){
        if (status.targets.hit[i] == 0) continue;
        char val_str[2];
        sprintf(val_str, "%d", i + 1); // Converte il valore in stringa
        mvwprintw(win, (int)(status.targets.y[i] * scaleh), (int)(status.targets.x[i] * scalew), "%s", val_str); // Usa un formato esplicito
    } 
    wattroff(win, COLOR_PAIR(3)); 
    wattroff(win, A_BOLD); // Disattiva il grassetto
}

void drawMenu(WINDOW* win) {

    wattron(win, A_BOLD); // Attiva il grassetto

    // Preparazione delle stringhe
    char score_str[10], diff_str[10];
    sprintf(score_str, "%d", inputStatus.score);
    sprintf(diff_str, "%s", difficultyStr);


    // Array con le etichette e i valori corrispondenti
    const char* labels[] = { "Score: ", "Player: "};
    const char* values[] = { score_str, inputStatus.name};

    int num_elements = 5; // Numero di elementi nel menu

    // Calcola la lunghezza totale occupata dalle stringhe
    int total_length = 0;
    for (int i = 0; i < num_elements; i++) {
        total_length += strlen(labels[i]) + strlen(values[i]);
    }

    // Calcola lo spazio rimanente e lo spazio tra gli elementi
    int remaining_space = nw - total_length; // Spazio non occupato dalle stringhe
    int spacing = remaining_space / (num_elements + 1); // Spaziatura uniforme

    // Stampa gli elementi equidistanti
    int current_position = spacing; // Posizione iniziale
    for (int i = 0; i < num_elements; i++) {
        // Stampa l'etichetta e il valore
        mvwprintw(win, 0, current_position, "%s%s", labels[i], values[i]);

        // Aggiorna la posizione corrente
        current_position += strlen(labels[i]) + strlen(values[i]) + spacing;
    }
    wattroff(win, A_BOLD); // Disattiva il grassetto
    // Aggiorna la finestra per mostrare i cambiamenti
    wrefresh(win);
}

int randomSelect(int n) {
    unsigned int random_number;
    int random_fd = open("/dev/urandom", O_RDONLY);
    
    if (random_fd == -1) {
        perror("Error opening /dev/urandom");
        return -1;  // Indicate failure
    }

    if (read(random_fd, &random_number, sizeof(random_number)) == -1) {
        perror("Error reading from /dev/urandom");
        close(random_fd);
        return -1;  // Indicate failure
    }
    
    close(random_fd);  // Close file after successful read
    
    return random_number % n;
}

void detectCollision(Message* status, Drone_bb * prev) {
    
    for (int i = 0; i < status->targets.number; i++) {
        
        if (status->targets.hit[i] && (((prev->x <= status->targets.x[i] + 2 && status->targets.x[i] - 2 <= status->drone.x)  &&
            (prev->y <= status->targets.y[i] + 2 && status->targets.y[i]- 2 <= status->drone.y) )||
            ((prev->x >= status->targets.x[i] - 2 && status->targets.x[i] >= status->drone.x + 2) &&
            (prev->y >= status->targets.y[i] - 2 && status->targets.y[i] >= status->drone.y + 2) ))){
                inputStatus.score += i;
                status->targets.hit[i] = 1;
                collision = 1;
                targetsHit++;   
        }
    }

}

void createNewMap(){

    storePreviousPosition(&status.drone);

    readMsg(fds[DRONE][askrd],  &msg,
        "[BB] Reading ready from [DRONE]", file);

    if(msg.msg == 'R'){

        LOGDRONEINFO(status.drone);

        status.msg = 'M';

        writeMsg(fds[DRONE][recwr], &status, 
                "[BB] Error asking drone position", file);

        storePreviousPosition(&status.drone);

        readMsg(fds[DRONE][askrd], &status,
                "[BB] Error reading drone position", file);
    }
}

void closeAll(){

    for(int j = 0; j < 6; j++){
        if (j != BLACKBOARD && pids[j] != 0){ 
            if (kill(pids[j], SIGTERM) == -1) {
            fprintf(logFile,"Process %d is not responding or has terminated\n", pids[j]);
            fflush(logFile);
            }
            LOGPROCESSDIED(pids[j])
        }
    }
    LOGBBDIED();
    fclose(file);
    fclose(logFile);
    exit(EXIT_SUCCESS);

}

void quit(){
    LOGQUIT();

    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", file);

    while(inputMsg.msg != 'q'){
                
        fprintf(file, "Waiting for quit\n");
        fflush(file);

        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", file);

    }
    
    LOGAMESAVING();
    inputStatus.msg = 'S';
    
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "[BB] Error sending ack", file);
    
    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", file);
    
    if(inputMsg.msg == 'R'){    //input ready, all data are saved
        LOGAMESAVED();
        closeAll();
    }          
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(BLACKBOARD);
    } else if (signo == SIGTERM) {
        LOGBBDIED();
        fclose(file);
        fclose(logFile);
        close(fds[DRONE][recwr]);
        close(fds[DRONE][askrd]);
        close(fds[INPUT][recwr]);
        close(fds[INPUT][askrd]);
        close(fds[OBSTACLE][recwr]);
        close(fds[OBSTACLE][askrd]);
        close(fds[TARGET][recwr]);
        close(fds[TARGET][askrd]);
        exit(EXIT_SUCCESS);
    } else if( signo == SIGWINCH){
        resizeHandler();
    }
}

int main(int argc, char *argv[]) {

    // Log file opening
    file = fopen("log/outputbb.txt", "w");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    logFile = fopen("log/logfile.log", "w");
    if (logFile == NULL) {
        perror("Errore nell'aprire il file di log");
        return 1;
    }
    
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <fd_str[0]> <fd_str[1]> <fd_str[2]> <fd_str[3]>\n", argv[0]);
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        char *fd_str = argv[i + 1];

        int index = 0;

        // Tokenization each value and discard ","
        char *token = strtok(fd_str, ",");
        token = strtok(NULL, ",");

        // FDs ectraction
        while (token != NULL && index < 4) {
            fds[i][index] = atoi(token);
            index++;
            token = strtok(NULL, ",");
        }
    }

    if (!targSub.init()){

        //--------------------
        // LOG
        //----------------------
        ;
    } 
    targSub.run();

    if (!obstSub.init()){

        //--------------------
        // LOG
        //----------------------
        ;
    } 
    obstSub.run();

    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "b%d,", pid);

    if(writeSecure("log/passParam.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in passParan.txt");
        exit(1);
    }
    

    // closing the unused fds to avoid deadlock
    close(fds[DRONE][askwr]);
    close(fds[DRONE][recrd]);
    close(fds[INPUT][askwr]);
    close(fds[INPUT][recrd]);
    close(fds[OBSTACLE][askwr]);
    close(fds[OBSTACLE][recrd]);
    close(fds[TARGET][askwr]);
    close(fds[TARGET][recrd]);

    fd_set readfds;
    struct timeval tv;
    
    //Setting select timeout
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    signal(SIGUSR1, sig_handler);
    signal(SIGWINCH, sig_handler);
    signal(SIGTERM, sig_handler);

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
    scaleh = (float)(nh - 2) / (float)WINDOW_LENGTH;
    scalew = (float)nw / (float)WINDOW_WIDTH;

    // Definizione delle coppie di colori
    init_pair(1, COLOR_BLUE, COLOR_BLACK);     // Testo blu su sfondo nero
    init_pair(2, COLOR_RED , COLOR_BLACK);  // Testo arancione su sfondo nero
    init_pair(3, COLOR_GREEN, COLOR_BLACK);    // Testo verde su sfondo nero

    usleep(500000);

    char datareaded[200];
    if (readSecure("log/passParam.txt", datareaded,1) == -1) {
        perror("Error reading the passParam file");
        exit(1);
    }

    // Parse the data and assign roles
    char *token = strtok(datareaded, ",");
    while (token != NULL) {
        char type = token[0];          // Get the prefix
        int number = atoi(token + 1);  // Convert the number part to int

        if (type == 'i') {
            pids[INPUT] = number;
        } else if (type == 'd') {
            pids[DRONE] = number;
        } else if (type == 'o') {
            pids[OBSTACLE] = number;
        } else if (type == 't') {
            pids[TARGET] = number;
        } else if (type == 'b') {
            pids[BLACKBOARD] = number;
        }else if (type == 'w') {
            pids[WATCHDOG] = number;
        }
        token = strtok(NULL, ",");
    }

    // Write the PID values to the output file
    for (int i = 0; i < 6; i++) {
        fprintf(file, "pid[%d] = %d\n", i, pids[i]);
        fflush(file);
    }

    inputStatus.score = 0;

    readInputMsg(fds[INPUT][askrd], &inputStatus, 
                "Error reading input", file);

    inputStatus.msg = 'A';
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "Error sending ack", file);


    mapInit(file);

    while (1) {
        
        if (targetsHit >= status.targets.number) {

                int y_center = (nh - 2) / 2;
                int x_center = nw / 2;

                const char *msg1 = "Congratulations! You Won!";
                const char *msg2 = "Press q to quit";

                mvwprintw(map, y_center, x_center - strlen(msg1) / 2, "%s", msg1);
                mvwprintw(map, y_center + 1, x_center - strlen(msg2) / 2, "%s", msg2);
                wrefresh(map);

                quit();
        }

        if (targSub.hasNewData() || obstSub.hasNewData()){

            if(targSub.hasNewData()){
                status.targets = targSub.getMyTargets();
            }
            if(obstSub.hasNewData()){
                status.obstacles = obstSub.getMyObstacles();
            }
            createNewMap();
        }

        // Update the main window
        werase(win);
        werase(map);
        box(map, 0, 0);

        drawMenu(win);
        drawDrone(map);
        drawObstacle(map);
        drawTarget(map);
        wrefresh(win);
        wrefresh(map);
        
        //FDs setting for select
        FD_ZERO(&readfds);
        FD_SET(fds[DRONE][askrd], &readfds);
        FD_SET(fds[INPUT][askrd], &readfds);

        int fdsQueue [4];
        int ready = 0;

        int sel = select(nfds, &readfds, NULL, NULL, &tv);
        
        if (sel == -1) {
            perror("Select error");
            break;
        } 

        if (FD_ISSET(fds[DRONE][askrd], &readfds)) {
            fdsQueue[ready] = fds[DRONE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[INPUT][askrd], &readfds)) {
            fdsQueue[ready] = fds[INPUT][askrd];
            ready++;
        }
        if (FD_ISSET(fds[OBSTACLE][askrd], &readfds)) {
            fdsQueue[ready] = fds[OBSTACLE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[TARGET][askrd], &readfds)) {
            fdsQueue[ready] = fds[TARGET][askrd];
            ready++;
        }

        if(ready > 0){
            unsigned int rand = randomSelect(ready);
            int selected = fdsQueue[rand];
            detectCollision(&status, &prevDrone);

            if (selected == fds[DRONE][askrd]){
                LOGPROCESSELECTED(DRONE);
         
                readMsg(fds[DRONE][askrd], &msg,
                                "[BB] Error reading drone position", file);

                LOGDRONEINFO(status.drone);

                if(msg.msg == 'R'){
                    if (collision) {
 
                        collision = 0;
                        
                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", file);

                        storePreviousPosition(&status.drone);

                        readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", file);
                        LOGDRONEINFO(status.drone);

                    }else{
                        
                        // fprintf(file, "[BB] Asking drone position to [DRONE]\n");
                        // fflush(file);
                        
                        status.msg = 'A';

                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", file);

                        status.msg = '\0';

                        storePreviousPosition(&status.drone);

                        readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", file);
                        LOGDRONEINFO(status.drone);

                        // fprintf(file, "Drone updated position: %d,%d\n", status.drone.x, status.drone.y);
                        // fflush(file);
                    }
                }
                  
            } else if (selected == fds[INPUT][askrd]){
                
                LOGPROCESSELECTED(INPUT);
                
                readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", file);

                LOGINPUTMESSAGE(inputMsg);

                if(inputMsg.msg == 'P'){
                    LOGDRONEINFO(status.drone)
                    mode = PAUSE;
                    LOGSTUATUS(mode);

                    inputStatus.msg = 'A';

                    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error sending ack", file);

                    fprintf(file, "Sended ack\n");
                    fflush(file);
                  
                    inputMsg.msg = 'A';

                    while(inputMsg.msg != 'P' && inputMsg.msg != 'q'){
                        
                        fprintf(file, "Waiting for play\n");
                        fflush(file);

                        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", file);

                    }
                    if(inputMsg.msg == 'P'){
                        mode = PLAY;
                        LOGSTUATUS(mode);
                    }else if(inputMsg.msg == 'q'){
                        fprintf(file, "Quit\n");
                        fflush(file);
                        
                        inputStatus.msg = 'S';
                        LOGAMESAVING();
                        
                        writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                    "[BB] Error sending ack", file);
                        
                        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                    "[BB] Error reading input", file);
                        
                        if(inputMsg.msg == 'R'){    //input ready, all data are saved

                            LOGAMESAVED();
                            closeAll();
                        }           
                    }
                    continue;

                }else if (inputMsg.msg == 'q'){
                    LOGAMESAVING();
                    inputStatus.msg = 'S';
                    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error sending ack", file);
                    
                    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", file);
                    
                    if(inputMsg.msg == 'R'){    //input ready, all data are saved

                        LOGAMESAVED();
                        closeAll();
                    }           
                }

                readMsg(fds[DRONE][askrd], &msg,
                                "[BB] Error reading drone ready", file);

                if(msg.msg == 'R'){

                    fprintf(file, "drone ready\n"); 
                    fflush(file);

                    status.msg = 'I';
                    strncpy(status.input, inputMsg.input, sizeof(inputStatus.input));
                    
                    writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", file);     
                }

                storePreviousPosition(&status.drone);

                readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", file);
                LOGDRONEINFO(status.drone);

                inputStatus.droneInfo = status.drone;
                
                writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error asking drone position", file); 
            }else{
                LOGPROCESSELECTED(999);
            }
        }
        usleep(PERIODBB);
    }

    return 0;
}