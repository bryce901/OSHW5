//Bryce Purnell
//2268534
//purne104@mail.chapman.edu

#define STR_LENGTH 32
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

//holds data for process
typedef struct  {
    char* theName;
    int WCET;
    int period;
    int timeLeft;
} proc;

// array for the processes running
typedef struct {
    proc* processArrayData;
    int number;
} proc_holder;

void *outputFile(void*);
proc* processCreate(proc*, char*, int, int);
int simmulation(int);
int checkArray(int);
int leastCommonMultiple();
int theLCM;
int amountOfPeriods;
sem_t* mainSephamoreArray;
int runnableCheck();
sem_t* semaphoreArray;
struct proc* processArrayData;
proc_holder processVector;
int isRunning = 1;
int maxPeriod();
char* filepathScheduler;
char* taskFilePath;
pthread_t* threadIDs;
FILE* outputFilePath;
int createProcObj(char*, proc_holder*);
void deleteProcObj(proc_holder*);
void createSemaphore();
int main(int argc, char *argv[]) {

    //error check
    if(argc != 4) {
        printf("ERROR wrong usage");
        return -1;
    }
    else if (atoi(argv[1]) < 1) {
        printf("Argument %s isn't positive. Make it positive.\n", argv[1]);
        return -1;
    }

    amountOfPeriods = atoi(argv[1]);
    taskFilePath = argv[2];
    filepathScheduler = argv[3];

    createProcObj(taskFilePath, &processVector);
    createSemaphore();

    //creates list of threads
    threadIDs = malloc(sizeof(pthread_t)*processVector.number);
    theLCM = leastCommonMultiple();

    if(runnableCheck()<=1) {
        int i;
        for(i = 0; i < processVector.number; ++i) {
            //gives temp id
            int* temp = malloc(sizeof(int));
            *temp = i;
            pthread_create(&threadIDs[i],NULL,outputFile,temp);
        }

        //opens the file
        if((outputFilePath = fopen(filepathScheduler, "w+")) == NULL) {
            printf("file can't be opened\n");
            return -1;
        }

        if(simmulation(amountOfPeriods) == -1) remove(filepathScheduler);

        //joins threads
        for (i = 0; i < processVector.number; ++i) {
            pthread_join(threadIDs[i],NULL);
        }
    }
    else
        printf("can't be scheduled\n");


    //frees and deletes objs
    deleteProcObj(&processVector);
    free(threadIDs);
    free(semaphoreArray);
    return 0;
}

//takes in a number of hyperperiods and outputs to the file
int simmulation(int times) {
    int* theStack = malloc(sizeof(int)*processVector.number);
    int top_s= -1;
   	int j;
    for(j = 0; j < theLCM; ++j) {
        fprintf(outputFilePath, "%d  ", j);
    }
    fprintf(outputFilePath, "\n");

    
    while(times-->0) {
	    int q;
        //least common multiple forms one hyperperiod
        for(q = 0; q < theLCM; ++q) {
            int i;
            for(i = 0; i < processVector.number; ++i) {

                //process starts again
                if(q%processVector.processArrayData[i].period == 0) {
                    // if the process did its last burst
                    if(processVector.processArrayData[i].timeLeft == 0) {
                        theStack[++top_s] = i;
                        //then resets timeLeft
                        processVector.processArrayData[i].timeLeft = processVector.processArrayData[i].WCET;

						int k;
						for(k = top_s; k > 0; --k) {
							if(processVector.processArrayData[theStack[k]].period > processVector.processArrayData[theStack[k-1]].period) {
								int temp = theStack[k];
						        theStack[k] = theStack[k-1];
								theStack[k-1] = temp;
			    			}
                        }
		    		}
                    
                    //scheduler failed if the process needs to go but it hasn't done it's last burst
		    		else {
            			printf("This process can't be scheduled\n");
                		isRunning = 0;
                		return -1;
                	}
				}
			}

            //stack isn't empty
			if(top_s != -1) {
                //thread saves the name to the file
				sem_post(&semaphoreArray[theStack[top_s]]);
				sem_wait(mainSephamoreArray);
				processVector.processArrayData[theStack[top_s]].timeLeft--;
                //once the burst is over it removes it from the stack
				if(processVector.processArrayData[theStack[top_s]].timeLeft == 0) top_s--;
			}
            //placeholder for no process in the stack
			else {
				fprintf(outputFilePath, "__ ");
			}
		}
        fprintf(outputFilePath, "\n");
    }

    fclose(outputFilePath);
	isRunning = 0;

	int i;
	for (i = 0; i < processVector.number; ++i) {
        sem_post(&semaphoreArray[i]);
    }

    free(theStack);
    return 0;
}
void createSemaphore() {
	mainSephamoreArray = malloc(sizeof(sem_t));
	if (sem_init(mainSephamoreArray, 0, 0) == -1) {
		printf("%s\n", strerror(errno));
	}
	//gets memory for sephamores
	semaphoreArray = malloc(sizeof(sem_t)*processVector.number);

	int i;
	for (i = 0; i < processVector.number; ++i) {
		if (sem_init(&semaphoreArray[i], 0, 0) == -1) {
			printf("%s\n", strerror(errno));
		}
	}
}
void deleteProcObj(proc_holder* processVector) {
	int i;
	for (i = 0; i < processVector->number; ++i) {
		free(processVector->processArrayData[i].theName);
	}
	free(processVector->processArrayData);
}


void *outputFile(void* param) {
    int threadID = *((int *) param);
    while(isRunning) {
        sem_wait(&semaphoreArray[threadID]);
        if(isRunning) {

			fprintf(outputFilePath, "%s ", processVector.processArrayData[threadID].theName);
            sem_post(mainSephamoreArray);
        }
	}

    free(param);
    pthread_exit(0);
}



//returns pointer from proc object
proc* processCreate(proc* processArrayData, char* theName, int WCET, int period) {
    processArrayData->theName = theName;
    processArrayData->WCET = WCET;
    processArrayData->period = period;
    processArrayData->timeLeft = 0;
    return processArrayData;
}

//finds the max period
int maxPeriod() {
	int maxPeriod = 0;
	int i;
	for (i = 0; i < processVector.number; ++i) {
		if (processVector.processArrayData[i].period > maxPeriod) maxPeriod = processVector.processArrayData[i].period;
	}
	return maxPeriod;
}

//reads from file and creates objects
int createProcObj(char* fp, proc_holder* processVector) {
    FILE* outputFilePath;

    if((outputFilePath = fopen(fp, "r+")) == NULL) {
        printf("File doesn't exist. Input existing file.\n");
        return -1;
    }

    //counts the amount processes in the file
    char tempName[STR_LENGTH];
    int WCET, period;
    processVector->number = 0;
    int ret;
    while(1) {
        ret = fscanf(outputFilePath, "%s %d %d", tempName, &WCET, &period);
        if(ret == 3)
            ++processVector->number;
        else if(errno != 0) {
            perror("scanf:");
            break;
        } else if(ret == EOF) {
            break;
        } else {
            printf("no match found\n");
        }
    }

    //as the name suggests it goes to the begining of the file
    rewind(outputFilePath);

    processVector->processArrayData = malloc(sizeof(proc)*processVector->number);
    processVector->number=0;

    //creating procs
    while(1) {
        char* n = malloc(sizeof(char)*STR_LENGTH);
        ret = fscanf(outputFilePath, "%s %d %d", n, &WCET, &period);
        if(ret == 3)
            processCreate(&processVector->processArrayData[processVector->number++], n, WCET, period);
        else if(errno != 0) {
            perror("scanf:");
            break;
        } else if(ret == EOF) {
            break;
        } else {
            printf("no match found\n");
        }
    }

    fclose(outputFilePath);
    return 0;
}


//checks to see if the processes are schedulable
int runnableCheck() {
	int sum = 0;
	int k;
	for (k = 0; k < processVector.number; ++k) {
		sum += processVector.processArrayData[k].WCET / processVector.processArrayData[k].period;
	}
	return sum;
}
//makes the least common multiple and returns the multiple
int leastCommonMultiple() {
	int theLCM = maxPeriod();
	while (checkArray(theLCM) != 1) ++theLCM;
	return theLCM;
}

//checks if the array is divisible by the number provided
int checkArray(int leastCommonMultiple) {
    int i;
    for(i = 0; i < processVector.number; ++i) {
        if(leastCommonMultiple%(processVector.processArrayData[i].period != 0)) return 0;
    }
    return 1;
}

