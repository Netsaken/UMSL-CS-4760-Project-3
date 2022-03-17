#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

int main(int argc, char *argv[]) {
    FILE *file;
    char logFile[15] = "./logfile.";
    key_t keyInt = ftok("./README.txt", 'g');
    key_t keyBool = ftok("./README.txt", 's');

    int *number;
    bool *choosing;

    int i = atoi(argv[0]);
    int higher = 0;
    char onlyTime[10];

    //Construct format for "perror"
    char* title = argv[0];
    char report[20] = ": shm";
    char* message;

    //Get shared memory
    int shmid_int = shmget(keyInt, sizeof(number), IPC_CREAT | 0666);
    if (shmid_int == -1) {
        strcpy(report, ": shmget1");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    int shmid_bool = shmget(keyBool, sizeof(choosing), IPC_CREAT | 0666);
    if (shmid_bool == -1) {
        strcpy(report, ": shmget2");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Attach shared memory
    number = shmat(shmid_int, NULL, 0);
    if (number == (void *) -1) {
        strcpy(report, ": shmat1");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    choosing = shmat(shmid_bool, NULL, 0);
    if (choosing == (void *) -1) {
        strcpy(report, ": shmat2");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Conduct Bakery Algorithm
    do {
        //Queue up
        choosing[i] = true;

        for (int k = 0; k < 20; k++) {
            if (higher < number[k]) {
                higher = number[k];
            }
        }
        number[i] = 1 + higher;
        
        choosing[i] = false;

        for (int j = 0; j < MAX_PROC; j++) {
            while (choosing[j]) {
               ; // WAIT if j happens to be choosing
            }
            while ( (number[j] != 0) 
            && (number[j] < number[i] || (number[j] == number[i] && j < i)) ) {
               ;
            }
        }

        //sleep for random amount of time (between 0 and 5 seconds)
        sleep(rand() % 5);

        //critical_section();
        time_t currentTime;
        time(&currentTime);
        strncpy(onlyTime, ctime(&currentTime)+11, 8);

        //Make logfile
        strcat(logFile, argv[0]);

        //Log entry time
        file = fopen(logFile, "a");
        fprintf(file, "Entered critical section at %s\n", onlyTime);
        fclose(file);

        //Print to cstest
        file = fopen("./cstest", "a");
        fprintf(file, "%s Queue %i File modified by process number %i\n", onlyTime, number[i], i);
        fclose(file);

        //sleep for random amount of time (between 0 and 5 seconds)
        sleep(rand() % 5);

        time(&currentTime);
        strncpy(onlyTime, ctime(&currentTime)+11, 8);

        //Log exit time
        file = fopen(logFile, "a");
        fprintf(file, "Left critical section at %s\n", onlyTime);
        fclose(file);  

        //Set queue number to 0 and exit critical section
        number[i] = 0;

        // Detach shared memory
        if (shmdt(number) == -1) {
            strcpy(report, ": shmdt1");
            message = strcat(title, report);
            perror(message);
            return 1;
        }

        if (shmdt(choosing) == -1) {
            strcpy(report, ": shmdt2");
            message = strcat(title, report);
            perror(message);
            return 1;
        }
    } while (1);

    return 0;
}