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

//Variables globales pour les temps
int sectorCompter; //Pour générer des temps aléatoires à chaque secteur en fonction du nombre d'appel à la fonction
int maxTime = 50000; //Temps max pour un secteur
int minTime = 40000; //Temps min pour un secteur
int minPit = 20000; //Temps min en pit
int maxPit = 25000; //Temps max en pit
//Définition d'une structure voiture
struct car {
    int ID; //ID du pilote
    double S1; //Temps S1
    double S2; //Temps S2
    double S3; //Temps S3
    double tour; //Temps tour = somme des temps
    double bestLap; //Temps meilleur tour
    int isOut; //Statut de out ou non (0=non, 1=oui)
    int estPit; //Nombre de passage en pit
    double totalTime; //Temps total de la course de cette voiture
} car[20];

//Cette méthode permet de générer un nombre aléatoire
//Utilisée pour le temps d'un secteur, les pits et crashs
int ramdom(int a, int b) {
    srand(time(NULL) * (getpid()) * sectorCompter); //Changer le temps de la fonction actuel pour éviter que les tempsse ressemblent
    return (rand() % (b - a) + a);
}

//Cette méthode permet de générer un temps de secteur
//Utilisée pour faire le temps des différents secteurs
double secteur() {
    if (!sectorCompter) { //Initialiser le compteur de secteur
        sectorCompter = 0;
    }
    sectorCompter++;
    usleep(30000); //Léger temps d'attente pour ralentir l'affichage
    return ((double) ramdom(minTime, maxTime) / (double) 1000);
}

//Cette méthode permet de générer un int pour mettre la voiture en pit ou non
int pit() {
    if (ramdom(0, 20) == 1) {
        return 1;
    } else {
        return 0;
    }
}

//Cette méthode permet de gérer le out ou non de la voiture
int out() {
    if (ramdom(0, 500) == 1) {
        return 1; //Voiture sera out
    } else {
        return 0; //Voiture non out
    }
}

//Cette méthode permet de savoir quel est le temps le plus petit
double min(double a, double b) {
    if (a > b) return b;
    if (a < b) return a;
    return a;
}


//Lance la simulation de la course
//nbreVoiture = nombre de voiture qui doivent tourner
//raceTime = définit le temps max que peut durer une course
//voitures = le tableau contenant les voitures
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

//Créer les fichiers pour sauvegarder les résultats
int exportTXT(int step, int nbrVoiture, struct car drivers[20]) {
    FILE *fichier = NULL;
//Pour définir les noms des fichiers
    switch (step) {
        case 1 :
            fichier = fopen("Essai1.txt", "w+");
//printf("Ecriture essai 1");
            break;
        case 2 :
            fichier = fopen("Essai2.txt", "w+");
//printf("Ecriture essai 2");
            break;
        case 3 :
            fichier = fopen("Essai3.txt", "w+");
//printf("Ecriture essai 3");
            break;
        case 4 :
            fichier = fopen("Qualif1.txt", "w+");
//printf("Ecriture qualif 1");
            break;
        case 5 :
            fichier = fopen("Qualif2.txt", "w+");
//printf("Ecriture qualif 2");
            break;
        case 6 :
            fichier = fopen("Qualif3.txt", "w+");
//printf("Ecriture qualif 3");
            break;
        case 7 :
            fichier = fopen("Course.txt", "w+");
//printf("Ecriture course");
            break;
    }
    if (fichier != NULL) {
//Début de l'écriture
        fprintf(fichier, "|N°\t|S1\t\t|S2\t\t|S3\t\t|Tour\t\t|Best\t\t|PIT\t|OUT\t|\n");
        fprintf(fichier, "\n");
        for (int j = 0; j < nbrVoiture; j++) {
            fprintf(fichier, "|%d\t", drivers[j].ID); //Imprime le N°
            if (drivers[j].S1 == 0) { //Imprime le temps S1

                fprintf(fichier, "|NULL\t");
            } else {
                fprintf(fichier, "|%.3f\t", drivers[j].S1);
            }
            if (drivers[j].S2 == 0) { //Imprime le temps S2
                fprintf(fichier, "|NULL\t");
            } else {
                fprintf(fichier, "|%.3f\t", drivers[j].S2);
            }
            if (drivers[j].S3 == 0) { //Imprime le temps S3
                fprintf(fichier, "|NULL\t");
            } else {
                fprintf(fichier, "|%.3f\t", drivers[j].S3);
            }
            if (drivers[j].tour == 0) { //Imprime le temps du tour
                fprintf(fichier, "|NULL\t\t");
            } else if (drivers[j].tour < 100.000) {
                fprintf(fichier, "|%.3f\t\t", drivers[j].tour);
            } else {
                fprintf(fichier, "|%.3f\t", drivers[j].tour);
            }
            if (drivers[j].bestLap < 100.000) { //Imprime le meilleur temps
                fprintf(fichier, "|%.3f\t\t", drivers[j].bestLap);
            } else {
                fprintf(fichier, "|%.3f\t", drivers[j].bestLap);
            }
            if (drivers[j].estPit != 0) { //Imprime le nombre de pit du pilote
                fprintf(fichier, "|%d\t", drivers[j].estPit);
            } else {
                fprintf(fichier, "|0\t");
            }
            if (drivers[j].isOut == 1) { //Imprime si le pilote est out
                fprintf(fichier, "|X\t|\n");
            } else {
                fprintf(fichier, "|\t|\n");
            }
        }//Fin ecriture
        fclose(fichier);

    }
    return 0;
}

//Main
int main(void) {
    int id[20] = {7, 99, 5, 16, 8, 20, 4, 55, 10, 26, 44, 77, 11, 18, 23, 33, 3, 27, 63, 88}; //Id des pilotes
    int lapTime = 130; //Temps moyen pour un tour afin de déterminer le temps de la course
    int totalLap = 45; //Nombres de tour
    struct car tri[20];
    for (int i = 0; i < 20; i++) { //Attribuer les id au tableau de voiture
        tri[i].ID = id[i];
    }
//Essai 1
//printf("\t\nEssai 1");
    sleep(1);
    Simulation(20, 5400, tri);
    exportTXT(1, 20, tri);
    sleep(2);
    system("clear");
//Essai 2
//printf("\t\nEssai2");
    sleep(1);
    Simulation(20, 5400, tri);
    exportTXT(2, 20, tri);
    sleep(2);
    system("clear");
//Essai 3
//printf("\t\nEssai 3");
    sleep(1);
    Simulation(20, 3600, tri);
    exportTXT(3, 20, tri);
    sleep(2);
    system("clear");
//Qualif 1 toutes les voitures
//printf("\t\nQualif 1");
    sleep(1);
    Simulation(20, 1080, tri);
    exportTXT(4, 20, tri);
    sleep(2);
    system("clear");
//Qualif 2 les 15 premières voitures

//printf("\t\nQualif 2");
    sleep(1);
    Simulation(15, 900, tri);
    exportTXT(5, 15, tri);
    sleep(2);
    system("clear");
//Qualif 3 les 10 premières voitures
//printf("\t\nQualif 3");
    sleep(1);
    Simulation(10, 720, tri);
    exportTXT(6, 10, tri);
    sleep(2);
    system("clear");
//Course
//printf("\t\nCourse");
    sleep(1);
    Simulation(20, (lapTime * totalLap), tri);
    exportTXT(7, 20, tri);
    sleep(2);
    system("clear");
}