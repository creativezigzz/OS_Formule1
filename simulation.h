//
// Created by sevro on 13-10-21.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>
#include <sys/sem.h>

#ifndef OS_FORMULE1_SIMULATION_H
#define OS_FORMULE1_SIMULATION_H

//Race Simulator
//racecarNumber = car number running in the circuit
//raceTime = max time limit within a circuit
//car = list containing all the cars
void Simulation(int racecarNumber, double raceTime, struct car drivers[20]) {
    int shmId;
    int semId;
    struct sembuf operation;
    int readerNum;
    struct car *circuit; //Structure containing all cars currently participating in the race
    shmId = shmget(666, 20 * sizeof(struct car), IPC_CREAT | 0666);
    if (shmId == -1) { //Error during shared memory creation
        printf("Error shmId = -1 ");
        exit(1);
    }
    circuit = shmat(shmId, 0, 0);
    if (circuit == (struct car *) -1) { //Error during shared memory access
        printf("Error shmat = -1");
        exit(1);
    }
    void startSem() { //Launches semaphore
        key_t semKey;
        semId = semget(semKey, 2, IPC_CREAT | 0666);
        if (semKey < 0) {
            printf("Error semid\n");
            exit(0);
        }
        semctl(semId, 0, SETVAL, 1);
        semctl(semId, 1, SETVAL, 1);
    }
    void wait(int i) { //Pauses thread
        operation.sem_num = i;
        operation.sem_op = -1;
        operation.sem_flg = SEM_UNDO;
        semop(semId, &operation, 1);
    }
    void post(int i) { //Enlève le thread de la liste d'attente
        operation.sem_num = i;
        operation.sem_op = 1;
        operation.sem_flg = SEM_UNDO;
        semop(semId, &operation, 1);
    }
    void commencerLecture() { //Début section critique
        wait(0);
        readerNum++;
        if (readerNum == 1) {
            wait(1);
        }
        post(0);
    }
    void arreterLecture() { //Fin section critique
        wait(0);
        readerNum--;
        if (readerNum == 0) {
            post(1);
        }
        post(0);
    }
    startSem();
//Cette méthode permet de sort les voitures prise sur internet et modifie pour notre projet
    void sort() {
        struct car temp; //Structure temporaire pour stocker les voitures en cours de tri
        commencerLecture(); //Section critique début
        memcpy(drivers, circuit, racecarNumber * sizeof(struct car));
        arreterLecture(); //Section critique fin
        for (int a = 0; a < racecarNumber; a++) {
            for (int b = 0; b < racecarNumber - 1; b++) {
                if (drivers[b].bestLap > drivers[b + 1].bestLap) {
                    temp = drivers[b + 1];
                    drivers[b + 1] = drivers[b];
                    drivers[b] = temp;
                }
            }//Fin for b
        }//Fin for a
    }//Fin sort
//Affichage à l'écran des différents temps quand appelle
    void affichage() {
        sort();
        double bestS1 = 99.0;
        double bestS2 = 99.0;
        double bestS3 = 99.0;
        double bestLap = 999.0;
        int j;
        system("clear"); //Nettoyer l'affichage entre chaque écran

//Affichage écran
        printf("|N°\t|S1\t|S2\t|S3\t|Tour\t\t|Best Tour\t|PIT\t|OUT\t|\n");
        printf("\n");
        for (j = 0; j < racecarNumber; j++) {
            printf("|%d\t", drivers[j].ID); //Affiche l'id du pilote
            if (drivers[j].S1 == 0) { //Affiche le temps S1
                printf("|NULL\t");
            } else {
                printf("|%.3f\t", drivers[j].S1);
            }
            if (drivers[j].S2 == 0) { //Affiche le temps S2
                printf("|NULL\t");
            } else {
                printf("|%.3f\t", drivers[j].S2);
            }
            if (drivers[j].S3 == 0) { //Affiche le temps S3
                printf("|NULL\t");
            } else {
                printf("|%.3f\t", drivers[j].S3);
            }
            if (drivers[j].tour < 100.000) { //Affiche le temps du tour
                printf("|%.3f\t\t", drivers[j].tour);
            } else {
                printf("|%.3f\t", drivers[j].tour);
            }
            if (drivers[j].bestLap < 100.000) { //Affiche le meilleur temps
                printf("|%.3f\t\t", drivers[j].bestLap);
            } else {
                printf("|%.3f\t", drivers[j].bestLap);
            }
            if (drivers[j].estPit != 0) { //Affiche le nombre de pit du pilote
                printf("|%d\t", drivers[j].estPit);
            } else {
                printf("|0\t");
            }
            if (drivers[j].isOut == 1) { //Affiche si le pilote est out
                printf("|X\t|\n");
            } else {
                printf("|\t|\n");

            }
        }//Fin affichage
//Définir les meilleurs temps
//TO-DO Regler le probleme des best temps = 0
        for (j = 0; j < racecarNumber; j++) {
            if (bestS1 > drivers[j].S1) {
                bestS1 = drivers[j].S1;
            }
            if (bestS2 > drivers[j].S2) {
                bestS2 = drivers[j].S2;
            }
            if (bestS3 > drivers[j].S3) {
                bestS3 = drivers[j].S3;
            }
            if (bestLap > drivers[j].bestLap) {
                bestLap = drivers[j].bestLap;
            }
        }
//Afficher les meilleurs temps
        printf("Best S1 : %.3f \n", bestS1);
        printf("Best S2 : %.3f \n", bestS2);
        printf("Best S3 : %.3f \n", bestS3);
        printf("Best tour : %.3f \n", bestLap);
    }
//Faire tourner les voitures
    for (int i = 0; i < racecarNumber; i++) {
        if (fork() == 0) { //Creation processus fils
            circuit = shmat(shmId, 0, 0);
            if (circuit == (struct car *) -1) { //Erreur lors de l'accès à la mémoire partagée
                printf("Erreur shmat = -1");
                exit(1);
            }
            circuit[i].totalTime = 0;
            circuit[i].ID = drivers		[i].ID;
            circuit[i].bestLap = 999;
            circuit[i].isOut = 0;
            circuit[i].estPit = 0;
            while (circuit[i].isOut == 0 && circuit[i].totalTime < raceTime) {
                circuit[i].tour = 0;
                if (out() == 1) { //Si la voiture se crash on met les secteurs à 0 et on termine le processus
                    circuit[i].isOut = 1;
                    circuit[i].S1 = 0;
                    circuit[i].S2 = 0;
                    circuit[i].S3 = 0;

                    exit(0);
                } else {
//Calcul temps S1 et mise en mémoire
                    wait(1);
                    circuit[i].S1 = secteur();
                    circuit[i].totalTime += circuit[i].S1;
                    circuit[i].tour += circuit[i].S1;
                    post(1);
//Calcul temps S2 et mise en mémoire
                    wait(1);
                    circuit[i].S2 = secteur();
                    circuit[i].totalTime += circuit[i].S2;
                    circuit[i].tour += circuit[i].S2;
                    post(1);
                    if (pit() == 1) { //Calcul pit
                        circuit[i].estPit += 1;
                        usleep(50000);
                        wait(1);
                        circuit[i].S3 = ((double) ramdom(minPit, maxPit) / (double) 1000) + secteur();
//Ajout du temps dans pit
                        post(1);
                    } else {
                        wait(1);
                        circuit[i].S3 = secteur();
                        usleep(50000);
                        post(1);
                    }
//Calcul temps S3 et mise en mémoire
                    wait(1);
                    circuit[i].tour += circuit[i].S3;
                    circuit[i].totalTime += circuit[i].S3;
                    circuit[i].bestLap = min(circuit[i].bestLap, circuit[i].tour);
                    post(1);
                    usleep(50000);
                }
            }
            exit(0);
        }
    }
//Gère le rafraîchissement de l'affichage
    for (int compteur = 0; compteur < ((int) raceTime / 130 * 3); compteur++) {
        affichage();
        usleep(90000);
    }
    if (shmdt(circuit) == -1) { //Erreur lors de la libération de la mémoire partagée

        printf("Erreur shmdt");
        exit(1);
    }
}

#endif //OS_FORMULE1_SIMULATION_H
