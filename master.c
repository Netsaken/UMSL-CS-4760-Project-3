#include <errno.h>
#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>

#include "config.h"

FILE *file;
pid_t childPid;

int *sharedInts;
bool *sharedBools;

int shmid_int;
int shmid_bool;

static void handle_sig(int sig) {
    int errsave, status;
    errsave = errno;
    //Print message
    printf("Program interrupted or time exceeded. Shutting down...\n");

    //End children
    kill(childPid, SIGTERM);
    waitpid(childPid, &status, 0);

    //Detach shared memory
    if (shmdt(sharedInts) == -1) {
        perror("./master: sigShmdt1");
        exit(1);
    }

    if (shmdt(sharedBools) == -1) {
        perror("./master: sigShmdt2");
        exit(1);
    }

    //Remove shared memory
    if (shmctl(shmid_int, IPC_RMID, 0) == -1) {
        perror("./master: sigShmctl1");
        exit(1);
    }

    if (shmctl(shmid_bool, IPC_RMID, 0) == -1) {
        perror("./master: sigShmctl2");
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

    key_t keyInt = ftok("./README.txt", 'g');
    key_t keyBool = ftok("./README.txt", 's');
    char iNum[3];

    int option, status;
    int s = 100;
    int n = 2;

    //Construct format for "perror"
    char* title = argv[0];
    char report[20] = ": shm";
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
    
    //Get the argument for number of options, if it exists
    if (argv[optind] != NULL) {
        n = atoi(argv[optind]);

        //Check for invalid entries
        if (n > MAX_PROC) {
            printf("Sorry, the maximum allowed processes for this program is %i.\n", MAX_PROC);
            printf("Number of processes has been set to %i...\n\n", MAX_PROC);
            n = MAX_PROC;
        } else if (n < 1) {
            printf("There was an error with your input. Please only use numbers.\n");
            printf("Number of processes has been set to 1...\n\n");
            n = 1;
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

    //Get shared memory
    shmid_int = shmget(keyInt, sizeof(sharedInts), IPC_CREAT | 0666);
    if (shmid_int == -1) {
        strcpy(report, ": shmget1");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    shmid_bool = shmget(keyBool, sizeof(sharedBools), IPC_CREAT | 0666);
    if (shmid_bool == -1) {
        strcpy(report, ": shmget2");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Attach shared memory
    sharedInts = shmat(shmid_int, NULL, 0);
    if (sharedInts == (void *) -1) {
        strcpy(report, ": shmat1");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    sharedBools = shmat(shmid_bool, NULL, 0);
    if (sharedBools == (void *) -1) {
        strcpy(report, ": shmat2");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Reset arrays
    for (int x = 0; x < 20; x++) {
        sharedInts[x] = 0;
    }
    for (int x = 0; x < 20; x++) {
        sharedBools[x] = 0;
    }

    //Fork and Exec slave programs
    for (int i = 0; i < n; i++) {
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

    //Detach shared memory
    if (shmdt(sharedInts) == -1) {
        strcpy(report, ": shmdt1");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    if (shmdt(sharedBools) == -1) {
        strcpy(report, ": shmdt2");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    //Remove shared memory
    if (shmctl(shmid_int, IPC_RMID, 0) == -1) {
        strcpy(report, ": shmctl1");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    if (shmctl(shmid_bool, IPC_RMID, 0) == -1) {
        strcpy(report, ": shmctl2");
        message = strcat(title, report);
        perror(message);
        return 1;
    }

    return 0;
}