#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

int main(int argc, char *argv[]) {
    FILE *file;
    char logFile[15] = "./logfile.";
    int number = atoi(argv[0]);

    key_t key = ftok("./README.txt", 's');
    union semun arg;
    int semid;

    char onlyTime[10];

    //Construct format for "perror"
    char* title = argv[0];
    char report[20] = ": sem";
    char* message;

    //Get semaphore set
    if ((semid = semget(key, 1, 0)) == -1) {
        strcpy(report, ": semget");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Queue into critical section using semaphores
    for (int i = 0; i < 5; i++) {
        struct sembuf sb = {0, -1, 0};
        //Wait until semaphore is 0
        if (arg.val > 0) {
            sb.sem_op = 0;
            if(semop(semid, &sb, 1) == -1) {
                strcpy(report, ": semop(wait)");
                message = strcat(title, report);
                perror(message);
                return 1;
            }
        }

        //Decrement semaphore
        sb.sem_op = -1;
        if(semop(semid, &sb, 1) == -1) {
            strcpy(report, ": semop(--)");
            message = strcat(title, report);
            perror(message);
            return 1;
        }

        //Make logfile
        strcat(logFile, argv[0]);

        //Log entry time
        file = fopen(logFile, "a");
        fprintf(file, "Entered critical section at %s\n", onlyTime);
        fclose(file);

        //sleep for random amount of time (between 0 and 5 seconds)
        sleep(rand() % 5);

        //critical_section();
        time_t currentTime;
        time(&currentTime);
        strncpy(onlyTime, ctime(&currentTime)+11, 8);

        //Print to cstest
        file = fopen("./cstest", "a");
        fprintf(file, "%s Queue %i File modified by process number %i\n", onlyTime, number, i);
        fclose(file);

        //sleep for random amount of time (between 0 and 5 seconds)
        sleep(rand() % 5);

        time(&currentTime);
        strncpy(onlyTime, ctime(&currentTime)+11, 8);

        //Log exit time
        file = fopen(logFile, "a");
        fprintf(file, "Left critical section at %s\n", onlyTime);
        fclose(file);  

        //Increment semaphore (Free resource)
        sb.sem_op = 1;
        if(semop(semid, &sb, 1) == -1) {
            strcpy(report, ": semop(--)");
            message = strcat(title, report);
            perror(message);
            return 1;
        }
    }

    return 0;
}