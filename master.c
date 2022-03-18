#include <errno.h>
#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "config.h"

FILE *file;
pid_t childPid;

union semun arg;
int semid;

static void handle_sig(int sig) {
    int errsave, status;
    errsave = errno;
    //Print message
    printf("Program interrupted or time exceeded. Shutting down...\n");

    //End children
    kill(childPid, SIGTERM);
    waitpid(childPid, &status, 0);

    //Remove shared memory
    if (semctl(semid, 0, IPC_RMID, arg) == -1) {
        perror("./master: sigSemctl");
        exit(1);
    }

    printf("Cleanup complete. Have a nice day!\n");

    //Exit program
    errno = errsave;
    exit(0);
}

static int setupinterrupt(void) {
    struct sigaction act;
    act.sa_handler = handle_sig;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGALRM, &act, NULL));
}

static int setupitimer(int inputTime) {
    struct itimerval value;
    value.it_interval.tv_sec = inputTime;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_REAL, &value, NULL));
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sig);

    key_t key = ftok("./README.txt", 's');

    char iNum[3];

    int option, status;
    int s = 100;
    int nprocs = 2;

    //Construct format for "perror"
    char* title = argv[0];
    char report[20] = ": sem";
    char* message;

    //Command line option
    while ((option = getopt(argc, argv, ":t:")) != -1) {
        switch (option) {
        case 't':
            //Maximum time before termination
            s = atoi(optarg);
            break;
        case 'h':
        case '?':
            printf("Usage: %s [-t # (maximum time running)] # (number of processes to run)\n", argv[0]);
            return 1;
        default:
            break;
        }
    }
    
    //Get the argument for number of slaves, if it exists
    if (argv[optind] != NULL) {
        nprocs = atoi(argv[optind]);

        //Check for invalid entries
        if (nprocs > MAX_PROC) {
            printf("Sorry, the maximum allowed processes for this program is %i.\n", MAX_PROC);
            printf("Number of processes has been set to %i...\n\n", MAX_PROC);
            nprocs = MAX_PROC;
        } else if (nprocs < 1) {
            printf("There was an error with your input. Please only use numbers.\n");
            printf("Number of processes has been set to 1...\n\n");
            nprocs = 1;
        }
    }

    //SET TIMER
    if (setupinterrupt() == -1) {
        strcpy(report, ": setupinterrupt");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    if (setupitimer(s) == -1) {
        strcpy(report, ": setitimer");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Create semaphore set
    if ((semid = semget(key, 1, 0666 | IPC_CREAT)) == -1) {
        strcpy(report, ": semget");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Initialize semaphore
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        strcpy(report, ": semctl(Init)");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Fork and Exec slave programs
    for (int i = 0; i < nprocs; i++) {
        childPid = fork();
        if (childPid == -1) {
            strcpy(report, ": childPid");
            message = strcat(title, report);
            perror(message);
            return 1;
        }

        //Wait for child to finish
        if (childPid == 0) {
            sprintf(iNum, "%i", i);
            execl("./slave", iNum, NULL);
        } 
        else do {
            if ((childPid = waitpid(childPid, &status, WNOHANG)) == -1) {
                strcpy(report, ": waitPid");
                message = strcat(title, report);
                perror(message);
                return 1;
            }
        } while (childPid == 0);
    }

    //Remove shared memory
    if (semctl(semid, 0, IPC_RMID, arg) == -1) {
        strcpy(report, ": semctl(RMID)");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    return 0;
}