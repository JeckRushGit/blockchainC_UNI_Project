#include "header.h"
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}
size_t num_utenti_attivi;
blocco * libro_mastro;
int * indice_libro_mastro;
id_t id_libro_mastro;
id_t id_indice_libro_mastro;
int * vettore_pid_nodi;
int * vettore_pid_utenti;
int * copia_vettore_pid_utenti;
id_t id_vettore_pid_nodi;
id_t id_vettore_pid_utenti;
id_t id_semaforo_attesa_code;
id_t id_semaforo_attesa_creazione_utenti;
id_t id_semaforo_morte_utente;
id_t id_coda_messaggi_rifiutati;
id_t id_semaforo_libro_mastro;
id_t id_semaforo_indice_libro_mastro;
int SO_NODES_NUM,SO_USERS_NUM;
int SO_BUDGET_INIT;
int fine_tempo = 0 , fine_libro = 0, fine_utenti = 0; 

void handler_segnale(int segnale);


int main(int argc,char * argv[]){

    
    int SO_SIM_SEC;
    int pid_nodo,pid_utente;
    size_t i;
    struct sigaction sa;
    sigset_t maschera;
    parametri parametri_master;
    struct sembuf so;


    /*HANDLING DI SIGINT E SIGALRM*/
    sigemptyset(&maschera);
    sa.sa_mask = maschera;
    sa.sa_handler = handler_segnale;
    sigaction(SIGALRM,&sa,NULL);
    sigaction(SIGINT,&sa,NULL);

    /*OTTENGO I PARAMETRI DI CUI HO BISOGNO*/
    parametri_master = parametri_init();
    SO_NODES_NUM = parametri_master.SO_NODES_NUM;
    SO_USERS_NUM = parametri_master.SO_USERS_NUM;
    SO_SIM_SEC = parametri_master.SO_SIM_SEC;
    SO_BUDGET_INIT = parametri_master.SO_BUDGET_INIT;

    /*CREO IL VETTORE PID NODI*/
    id_vettore_pid_nodi = shmget(CHIAVE_VETTORE_PID_NODI,sizeof(int) * SO_NODES_NUM,IPC_CREAT | 0600);
    vettore_pid_nodi = (int *)shmat(id_vettore_pid_nodi,NULL,0);

    /*CREO IL VETTORE PID UTENTI*/
    id_vettore_pid_utenti = shmget(CHIAVE_VETTORE_PID_UTENTI,sizeof(int) * SO_USERS_NUM,IPC_CREAT | 0600);
    vettore_pid_utenti = (int *)shmat(id_vettore_pid_utenti,NULL,0);

    /*CREO ED INIZIALIZZO IL SEMAFORO PER L'ATTESA CREAZIONE CODE*/
    id_semaforo_attesa_code = creazione_semaforo_singolo(CHIAVE_SEM_ATTESA_CODE,1);

    /*CREO ED INIZIALIZZO IL SEMAFORO PER L'ATTESA UTENTI */
    id_semaforo_attesa_creazione_utenti =  creazione_semaforo_singolo(CHIAVE_SEM_ATTESA_CREAZIONE_UTENTI,1);

    /*CREO ED INIZIALIZZO IL SEMAFORO PER LA MORTE DEI PROCESSI*/
    id_semaforo_morte_utente = creazione_semaforo_singolo(CHIAVE_SEM_MORTE_UTENTI,1);

    /*CREO LA CODA DI MESSAGGI DI TRANSAZIONI RIFIUTATE*/
    id_coda_messaggi_rifiutati = msgget(CHIAVE_CODA_DI_MESSAGGI_TRANSAZIONI_RIFIUTATE,IPC_CREAT | 0600);

    /*CREO IL SEGMENTO DI MEMORIA CONDIVISA DEL LIBRO MASTRO*/
    id_libro_mastro = shmget(CHIAVE_LIBRO_MASTRO,(SO_REGISTRY_SIZE) * (SO_BLOCK_SIZE) * (sizeof(transazione)),IPC_CREAT | 0600);
    libro_mastro = (blocco *)shmat(id_libro_mastro,NULL,0);

    /*CREO L'INDICE DEL LIBRO MASTRO CONDIVISO*/
    id_indice_libro_mastro = shmget(CHIAVE_INDICE_LIBRO_MASTRO,sizeof(int),IPC_CREAT | 0600);
    indice_libro_mastro = (int *)shmat(id_indice_libro_mastro,NULL,0);
    *indice_libro_mastro = 0;

    /*INIZIALIZZO LA COPIA DEL VETTORE PID UTENTI*/
    copia_vettore_pid_utenti = calloc(SO_USERS_NUM,sizeof(int));

    /*CREO ED INIZIALIZZO I SEMAFORI PER IL LIBRO MASTRO E IL SUO INDICE*/
    id_semaforo_libro_mastro = creazione_semaforo_singolo(CHIAVE_SEM_LIBRO_MASTRO,1);
    id_semaforo_indice_libro_mastro = creazione_semaforo_singolo(CHIAVE_SEM_INDICE_LIBRO_MASTRO,1);


    /*CREO I NODI*/
    for(i = 0; i < SO_NODES_NUM ; i++){
        pid_nodo = fork();
        switch (pid_nodo){
        case 0:
            /*PROCESSO NODO*/
            execv("nodo",argv);
            printf("Questo messaggio non dovrebbe mai comparire\n");
            break;
        case -1:
            /*ERRORE NELLA FORK*/
            fprintf(stderr,"Errore nella creazione dei nodi\n");
            break;
        default:
            /*PROCESSO MASTER*/
            vettore_pid_nodi[i] = pid_nodo;
            msgget(pid_nodo,IPC_CREAT | 0600);
            break;
        }

    }

    so.sem_num = 0;
    so.sem_op = -1;
    semop(id_semaforo_attesa_code,&so,1);
    /*I NODI POSSONO PARTIRE*/

    for(i = 0; i < SO_USERS_NUM ; i++){
        pid_utente = fork();
        switch (pid_utente){
        case 0:
            /*PROCESSO UTENTE*/
            execv("utente",argv);
            printf("Questo messaggio non dovrebbe mai comparire\n");
            break;
        case -1:
            /*ERRORE NELLA FORK*/
            fprintf(stderr,"Errore nella creazione degli utenti\n");
            break;
        default:
            vettore_pid_utenti[i] = pid_utente;
            break;
        }
    }
    memcpy(copia_vettore_pid_utenti,vettore_pid_utenti,(SO_USERS_NUM) * sizeof(int));
    so.sem_num = 0;
    so.sem_op = -1;
    semop(id_semaforo_attesa_creazione_utenti,&so,1);
    /*GLI UTENTI POSSONO PARTIRE*/
    
    alarm(SO_SIM_SEC);
    while(TRUE){
        
        num_utenti_attivi = 0;
        so.sem_num = 0;
        so.sem_op = -1;
        semop(id_semaforo_morte_utente,&so,1);
        
        for(i = 0 ; i < SO_USERS_NUM ; i++){
            if(vettore_pid_utenti[i] != 0){
                num_utenti_attivi++;
            }
        }
        printf("Ci sono %ld utenti attivi e %d nodi attivi\n",num_utenti_attivi,SO_NODES_NUM);

        if(num_utenti_attivi <= 1){
            fine_utenti = 1;
            raise(SIGINT);
        }

        so.sem_num = 0;
        so.sem_op = 1;
        semop(id_semaforo_morte_utente,&so,1);
        
        so.sem_num = 0;
        so.sem_op = -1;
        semop(id_semaforo_libro_mastro,&so,1);
        
        if(*indice_libro_mastro < SO_REGISTRY_SIZE-1){
            stampa_budget_processi(vettore_pid_nodi,copia_vettore_pid_utenti,libro_mastro,indice_libro_mastro,SO_USERS_NUM,SO_BUDGET_INIT,SO_NODES_NUM);
            
        }
        else{
            fine_libro = 1;
            raise(SIGINT);
        }
        so.sem_num = 0;
        so.sem_op = 1;
        semop(id_semaforo_libro_mastro,&so,1);


        sleep(1);
    }
}



void handler_segnale(int segnale){
    size_t i,n,j;
    id_t id_coda_transazioni_in;
    int pid_utente = 0, pid_nodo = 0,quantita_pid_nodo_tmp = 0;
    int quantita_pid_utente = SO_BUDGET_INIT;

    if(segnale == SIGALRM){
        fine_tempo = 1;
        kill(getpid(),SIGINT);
    }
    if(segnale == SIGINT){
        
        for(i = 0; i < SO_USERS_NUM ; i++){
            if(vettore_pid_utenti[i] != 0){
                kill(vettore_pid_utenti[i],SIGINT);
            }
        }  
        for(n = 0 ; n < SO_USERS_NUM ; n++){
            pid_utente = copia_vettore_pid_utenti[n];
            quantita_pid_utente = SO_BUDGET_INIT;
            for(i = 0 ; i < *indice_libro_mastro ; i++){
                for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
                    if(libro_mastro[i][j].sender == pid_utente){
                        quantita_pid_utente = quantita_pid_utente - libro_mastro[i][j].quantita;
                    }
                    else if(libro_mastro[i][j].receiver == pid_utente){
                        quantita_pid_utente = quantita_pid_utente + libro_mastro[i][j].quantita - libro_mastro[i][j].reward;
                    }
                }
            }
            fprintf(stdout,"L'utente %d aveva budget : %d\n",pid_utente,quantita_pid_utente);
        }
        
        for(n = 0 ; n < SO_NODES_NUM ; n++){
            pid_nodo = vettore_pid_nodi[n];
            quantita_pid_nodo_tmp = 0;
            for(i = 0 ; i < *indice_libro_mastro ; i++){
                if(libro_mastro[i][(SO_BLOCK_SIZE-1)].receiver == pid_nodo){
                    quantita_pid_nodo_tmp = quantita_pid_nodo_tmp + libro_mastro[i][(SO_BLOCK_SIZE-1)].quantita;
                }
            }
            fprintf(stdout,"Il nodo  %d aveva budget : %d\n",pid_nodo,quantita_pid_nodo_tmp);
        }
        fprintf(stdout,"Sono morti %d processi utente\n",SO_USERS_NUM-(int)num_utenti_attivi);
        fprintf(stdout,"Ci sono %d blocchi nel libro mastro\n",*indice_libro_mastro);
        if(fine_tempo == 1){
            fprintf(stdout,"\n\n\n!!!!!Sono trascorsi SO_SIM_SEC secondi!!!!!\n\n\n\n");
        }
        if(fine_libro == 1){
            fprintf(stdout,"\n\n\n!!!!!La capacità del libro mastro si è esaurita!!!!!\n\n\n\n");
        }
        if(fine_utenti == 1){
            fprintf(stdout,"\n\n\n!!!!!Non ci sono abbastanza utenti vivi!!!!!\n\n\n\n");
        }
        for(i = 0 ; i < SO_NODES_NUM; i++){
            kill(vettore_pid_nodi[i],SIGINT);
        }
        sleep(1);
        for(i = 0 ; i < SO_NODES_NUM; i++){
            id_coda_transazioni_in = msgget(vettore_pid_nodi[i],0600);
            msgctl(id_coda_transazioni_in,IPC_RMID,0);
        }

        shmdt(libro_mastro);
        shmctl(id_libro_mastro,IPC_RMID,0);
        shmdt(indice_libro_mastro);
        shmctl(id_indice_libro_mastro,IPC_RMID,0);
        semctl(id_semaforo_attesa_code,0,IPC_RMID,0);
        semctl(id_semaforo_attesa_creazione_utenti,0,IPC_RMID,0);
        semctl(id_semaforo_morte_utente,IPC_RMID,0);
        semctl(id_semaforo_libro_mastro,IPC_RMID,0);
        semctl(id_semaforo_indice_libro_mastro,IPC_RMID,0);
        shmdt(vettore_pid_utenti);
        shmdt(vettore_pid_nodi);
        shmctl(id_vettore_pid_nodi,IPC_RMID,0);
        shmctl(id_vettore_pid_utenti,IPC_RMID,0);
        msgctl(id_coda_messaggi_rifiutati,IPC_RMID,0);
        free(copia_vettore_pid_utenti);
        TEST_ERROR;
        kill(getpid(),SIGTERM);
    }
}


