//Créé et développé par : Gabrielle Cruz et Simon Périquet

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

//Simulation de la course

//Variable raceCarNumber  => Numéro de la voiture qui roule sur le circuit

//Variable raceTime => La limite en  temps maximum sur un circuit

//Variable car => Liste contenant toutes les voitures


void Simulation(int raceCarNumber, double raceTime, struct car drivers[20]) {
    int shmId;

    int semId;

    struct sembuf operation;

    int readerNum;

    //Structure contenant toutes les voitures participants actuellement à la course
    struct car *circuit;

    shmId = shmget(666, 20 * sizeof(struct car), IPC_CREAT | 0666);

    if (shmId == -1) { //Erreur lors de la création de la mémoire partagée
        printf("Error while creation : shmId = -1 ");
        exit(1);
    }

    circuit = shmat(shmId, 0, 0);

    if (circuit == (struct car *) -1) { //Erreur lors de l'accès à la mémoire partagée
        printf("Error while accesing :  shmat = -1");
        exit(1);
    }

    void startSem() { //Lance la semaphore
        key_t semKey;

        semId = semget(semKey, 2, IPC_CREAT | 0666);

        if (semKey < 0) {
            printf("Error semid\n");

            exit(0);
        }

        semctl(semId, 0, SETVAL, 1);

        semctl(semId, 1, SETVAL, 1);
    }

    void wait(int i) { //Met en pause le Thread
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

//Cette méthode permet de trier les voitures selon leur position dans la course
    void sort() {
        struct car temp; //Structure temporaire pour stocker les voitures en cours de tri

        commencerLecture(); //Section critique début

        memcpy(drivers, circuit, raceCarNumber * sizeof(struct car));

        arreterLecture(); //Section critique fin

        for (int a = 0; a < raceCarNumber; a++) {

            for (int b = 0; b < raceCarNumber - 1; b++) {

                if (drivers[b].bestLap > drivers[b + 1].bestLap) {

                    temp = drivers[b + 1];

                    drivers[b + 1] = drivers[b];

                    drivers[b] = temp;
                }
            }//Fin for b
        }//Fin for a
    }//Fin sort

//Méthode pour afficher à l'écran les différents meilleurs temps quand ceux-ci vont être appelés
    void affichage() {
        sort();

        double bestS1 = 99.0;

        double bestS2 = 99.0;

        double bestS3 = 99.0;

        double bestLap = 999.0;

        int j;
        system("clear"); //Nettoyer l'affichage entre chaque écran

    //Affichage à l'écran
        printf("|N°\t|S1\t|S2\t|S3\t|Tour\t\t|Best Tour\t|PIT\t|OUT\t|\n");

        printf("\n");

        for (j = 0; j < raceCarNumber; j++) {

            //Affiche l'id du pilote
            printf("|%d\t", drivers[j].ID);

            //Affiche le temps S1
            (drivers[j].S1 == 0) ? printf("|NULL\t") : printf("|%.3f\t", drivers[j].S1);

            //Affiche le temps S2
            (drivers[j].S2 == 0) ? printf("|NULL\t") : printf("|%.3f\t", drivers[j].S2);

            //Affiche le temps S3
            (drivers[j].S3 == 0) ? printf("|NULL\t") : printf("|%.3f\t", drivers[j].S3);

            //Affiche le temps du tour
            (drivers[j].tour < 100.000) ? printf("|%.3f\t\t", drivers[j].tour) : printf("|%.3f\t", drivers[j].tour);

            //Affiche le meilleur temps
            (drivers[j].bestLap < 100.000) ? printf("|%.3f\t\t", drivers[j].bestLap) : printf("|%.3f\t", drivers[j].bestLap);

            //Affiche le nombre de pit du pilote
            (drivers[j].estPit != 0) ? printf("|%d\t", drivers[j].estPit) : printf("|0\t");


            //Affiche si le pilote est out
            (drivers[j].isOut == 1) ? printf("|X\t|\n") :  printf("|\t|\n");

            //Fin affichage
        }



    //Définir les meilleurs temps

        for (j = 0; j < raceCarNumber; j++) {

            bestS1 = (bestS1 > drivers[j].S1) ? drivers[j].S1 : bestS1;

            bestS2 = (bestS2 > drivers[j].S2) ? drivers[j].S2 : bestS2;

            bestS3 =  (bestS3 > drivers[j].S3) ? drivers[j].S3 : bestS3;

            bestLap = (bestLap > drivers[j].bestLap) ? drivers[j].bestLap : bestLap;

        }

    //Afficher les meilleurs temps

        printf("Best S1 : %.3f \n", bestS1);

        printf("Best S2 : %.3f \n", bestS2);

        printf("Best S3 : %.3f \n", bestS3);

        printf("Best tour : %.3f \n", bestLap);
    }

    //Faire tourner les voitures

    for (int i = 0; i < raceCarNumber; i++) {

        if (fork() == 0) { //Creation processus fils

            circuit = shmat(shmId, 0, 0);

            if (circuit == (struct car *) -1) { //Erreur lors de l'accès à la mémoire partagée

                printf("Erreur shmat = -1");

                exit(1);
            }

            circuit[i].totalTime = 0;

            circuit[i].ID = drivers[i].ID;

            circuit[i].bestLap = 999;

            circuit[i].isOut = 0;

            circuit[i].estPit = 0;

            while (circuit[i].isOut == 0 && circuit[i].totalTime < raceTime) {

                circuit[i].tour = 0;

                //Si la voiture se crash on met les secteurs à 0 et on termine le processus
                if (out() == 1) {

                    circuit[i].isOut = 1;

                    circuit[i].S1 = 0;

                    circuit[i].S2 = 0;

                    circuit[i].S3 = 0;

                    exit(0);

                }
                //Calcul temps S1 et mise en mémoire
                else {

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

                    //Calcul pit
                    if (pit() == 1) {

                        circuit[i].estPit += 1;

                        usleep(50000);

                        wait(1);

                        circuit[i].S3 = ((double) ramdom(minPit, maxPit) / (double) 1000) + secteur();

                        //Ajout du temps dans pit
                        post(1);

                    }
                    else {

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
