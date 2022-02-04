#include "header.h"
#define TEST_ERROR    if (errno && errno != ENOMSG ) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}

int flag = 0;

void handler_segnale(int segnale);

int main(int argc,char * argv[]){
    blocco * libro_mastro;
    int * indice_libro_mastro;
    long SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC;
    long attesa_transazione = 0;
    int successo = 0;
    parametri parametri_utente;
    int SO_BUDGET_INIT;
    int retry = 0;
    int SO_RETRY;
    int SO_NODES_NUM,SO_USERS_NUM;
    int v = 0,t = 0;
    int mypid;
    int quantita_rifiutata = 0;
    int budget;
    size_t tmp = 0;
    size_t num_transazioni_rifiutate;
    size_t num_transazioni_effettuate = 0;
    id_t id_vettore_pid_utenti;
    id_t id_vettore_pid_nodi;
    id_t id_semaforo_morte_utente;
    id_t id_coda;
    id_t id_coda_di_messaggi_transazioni_rifiutate;
    id_t id_libro_mastro;
    id_t id_indice_libro_mastro;
    id_t id_semaforo_libro_mastro;
    id_t id_semaforo_indice_libro_mastro;
    int * vettore_pid_utenti;
    int * vettore_pid_nodi;
    id_t id_semaforo_attesa_creazione_utenti;
    transazione nuova_transazione;
    mymsg nuovo_messaggio,messaggio_rifiutato;
    sigset_t maschera,maschera_origin;
    struct sigaction sa;
    struct sembuf so;
    struct timespec inizio,fine;
    struct msqid_ds msgds;
    struct timespec req;

    /*OTTENGO I PARAMETRI*/
    parametri_utente = parametri_init();
    SO_BUDGET_INIT = parametri_utente.SO_BUDGET_INIT;
    SO_NODES_NUM = parametri_utente.SO_NODES_NUM;
    SO_USERS_NUM = parametri_utente.SO_USERS_NUM;
    SO_RETRY = parametri_utente.SO_RETRY;
    SO_MIN_TRANS_GEN_NSEC = parametri_utente.SO_MIN_TRANS_GEN_NSEC;
    SO_MAX_TRANS_GEN_NSEC = parametri_utente.SO_MAX_TRANS_GEN_NSEC;

    sigemptyset(&maschera);
    sigaddset(&maschera,SIGUSR1);
    sa.sa_mask = maschera;
    sa.sa_handler = handler_segnale;
    sa.sa_flags = 0;
    sigaction(SIGUSR1,&sa,NULL);




    mypid = getpid();
    srand(mypid);

    attesa_transazione = estrazione_long_casuale(SO_MIN_TRANS_GEN_NSEC,SO_MAX_TRANS_GEN_NSEC);
    
    req.tv_sec = attesa_transazione / 1000000000;
    req.tv_nsec = attesa_transazione - (req.tv_sec * 1000000000);




    /*OTTENGO L'ID DEL SEMAFORO ATTESA CREAZIONE UTENTI*/
    id_semaforo_attesa_creazione_utenti = semget(CHIAVE_SEM_ATTESA_CREAZIONE_UTENTI,0,0600);

    /*OTTENGO L'ID DEL SEMAFORO MORTE UTENTE*/
    id_semaforo_morte_utente = semget(CHIAVE_SEM_MORTE_UTENTI,0,0600);

    /*OTTENGO L'ID DELLA CODA DI MESSAGGI CON TRANSAZIONI RIFIUTATE*/
    id_coda_di_messaggi_transazioni_rifiutate = msgget(CHIAVE_CODA_DI_MESSAGGI_TRANSAZIONI_RIFIUTATE,0600);


    /*OTTENGO L'ID DEL VETTORE_PID_NODI E FACCIO L'ATTACH*/
    id_vettore_pid_nodi = shmget(CHIAVE_VETTORE_PID_NODI,0,0600);
    vettore_pid_nodi = (int *)shmat(id_vettore_pid_nodi,NULL,0);

    /*OTTENGO L'ID DEL LIBRO MASTRO E DEL SUO INDICE*/
    id_libro_mastro = shmget(CHIAVE_LIBRO_MASTRO,0,0600);
    id_indice_libro_mastro = shmget(CHIAVE_INDICE_LIBRO_MASTRO,0,0600);
    /*FACCIO L'ATTACH*/
    libro_mastro = (blocco *)shmat(id_libro_mastro,NULL,0);
    indice_libro_mastro = (int *)shmat(id_indice_libro_mastro,NULL,0);
    /*OTTENGO L'ID DEI SEMAFORI LIBRO MASTRO E INDICE LIBRO MASTRO*/
    id_semaforo_libro_mastro = semget(CHIAVE_SEM_LIBRO_MASTRO,0,0600);
    id_semaforo_indice_libro_mastro = semget(CHIAVE_SEM_INDICE_LIBRO_MASTRO,0,0600);


    /*ASPETTO CHE TUTTI I PID UTENTE SIANO NEL VETTORE*/
    so.sem_num = 0;
    so.sem_op = 0;
    semop(id_semaforo_attesa_creazione_utenti,&so,1);
    /*ORA L'UTENTE PUò PARTIRE*/

    
    /*OTTENGO L'ID DEL VETTORE_PID_UTENTI E FACCIO L'ATTACH*/
    id_vettore_pid_utenti = shmget(CHIAVE_VETTORE_PID_UTENTI,0,0600);
    vettore_pid_utenti = (int *)shmat(id_vettore_pid_utenti,NULL,0);
    /*CIAO*/

    clock_gettime(CLOCK_REALTIME,&inizio);
    while(TRUE){
        sigprocmask(SIG_BLOCK,&maschera,&maschera_origin);
        if(num_transazioni_effettuate == 0){
                        /*PRIMA TRANSAZIONE (NON FACCIO IL CALCOLO DEL BUDGET)*/
            /*IMPOSTO IL VALORE DI TIMESTAMP*/
            clock_gettime(CLOCK_REALTIME,&fine);
            /*CREO LA TRANSAZIONE*/
            nuova_transazione = creazione_transazione(SO_BUDGET_INIT,inizio,fine,mypid,vettore_pid_utenti,parametri_utente,id_semaforo_morte_utente);
            /*CREO IL MESSAGGIO CON LA TRANSAZIONE APPENA CREATA*/
            nuovo_messaggio = crea_messaggio(nuova_transazione);
            /*INVIO IL MESSAGGIO AD UN NODO ESTRATTO A CASO ATTRAVERSO LA CODA*/
            invio_transazione_da_utente_a_nodo(nuovo_messaggio,SO_NODES_NUM,vettore_pid_nodi);
            TEST_ERROR;
            v = v + nuovo_messaggio.transazione.quantita;
            num_transazioni_effettuate++;
        }
        else{
            if(flag == 0){
                        /*DALLA 2 A n TRANSAZIONI */
                num_transazioni_rifiutate = 0;
                bzero(&nuova_transazione,sizeof(transazione));
                bzero(&nuovo_messaggio,sizeof(mymsg));
                bzero(&messaggio_rifiutato,sizeof(mymsg));
                
                msgctl(id_coda_di_messaggi_transazioni_rifiutate,IPC_STAT,&msgds);
                num_transazioni_rifiutate = msgds.msg_qnum;
                clock_gettime(CLOCK_REALTIME,&fine);
                /*CONTROLLO SE CI SONO TRANSAZIONI RIFIUTATE*/
                quantita_rifiutata = 0;
                if(num_transazioni_rifiutate > 0){
                    while(errno != ENOMSG){
                        bzero(&messaggio_rifiutato,sizeof(mymsg));
                        msgrcv(id_coda_di_messaggi_transazioni_rifiutate,&messaggio_rifiutato,sizeof(transazione),mypid,IPC_NOWAIT);
                        quantita_rifiutata = quantita_rifiutata + messaggio_rifiutato.transazione.quantita;
                        
                    }
                    /*printf("Sono %d e il totale rifiutato è : %d\n",mypid,quantita_rifiutata);*/
                }
                errno = 0;

                TEST_ERROR;
                so.sem_num = 0;
                so.sem_op = -1;
                semop(id_semaforo_libro_mastro,&so,1);
                
                budget = calcolo_budget(libro_mastro,indice_libro_mastro,mypid,quantita_rifiutata,&t,&v,parametri_utente);
                /*printf("Sono %d e il mio budget è : %d\n",mypid,budget);*/

                so.sem_num = 0;
                so.sem_op = 1;
                semop(id_semaforo_libro_mastro,&so,1);
                TEST_ERROR;
                if(budget >= 2){
                    nuova_transazione = creazione_transazione(budget,inizio,fine,mypid,vettore_pid_utenti,parametri_utente,id_semaforo_morte_utente);
                    nuovo_messaggio = crea_messaggio(nuova_transazione);
                    successo = invio_transazione_da_utente_a_nodo(nuovo_messaggio,SO_NODES_NUM,vettore_pid_nodi);
                    if(successo == 0){
                        v = v + nuovo_messaggio.transazione.quantita;
                    }
                    retry = 0;
                }
                else{
                    /*printf("Sono %d e provo a chiedere soldi a qualcuno\n",mypid);*/
                    richiesta_denaro_segnale(mypid,vettore_pid_utenti,id_semaforo_morte_utente,SO_USERS_NUM);
                    retry++;
                }
                if(retry >= SO_RETRY){
                    /*printf("Sono %d e ho raggiunto SO_RETRY , morirò\n",mypid);*/
                    morte_processo(mypid,vettore_pid_utenti,SO_USERS_NUM,id_semaforo_morte_utente);
                }
            }
            
            
            else if(flag == 1){
                        /*TRANSAZIONE IN RISPOSTA AL SEGNALE*/
                /*printf("Mi è arrivato un segnale e proverò ad inviare una transazione\n");*/
                num_transazioni_rifiutate = 0;
                bzero(&nuova_transazione,sizeof(transazione));
                bzero(&nuovo_messaggio,sizeof(mymsg));
                bzero(&messaggio_rifiutato,sizeof(mymsg));

                msgctl(id_coda_di_messaggi_transazioni_rifiutate,IPC_STAT,&msgds);
                num_transazioni_rifiutate = msgds.msg_qnum;
                clock_gettime(CLOCK_REALTIME,&fine);
                /*CONTROLLO SE CI SONO TRANSAZIONI RIFIUTATE*/
                quantita_rifiutata = 0;
                if(num_transazioni_rifiutate > 0){
                    while(errno != ENOMSG){
                        bzero(&messaggio_rifiutato,sizeof(mymsg));
                        msgrcv(id_coda_di_messaggi_transazioni_rifiutate,&messaggio_rifiutato,sizeof(transazione),mypid,IPC_NOWAIT);
                        quantita_rifiutata = quantita_rifiutata + messaggio_rifiutato.transazione.quantita;
                        
                    }
                    /*printf("Sono %d e il totale rifiutato è : %d\n",mypid,quantita_rifiutata);*/
                }
                errno = 0;
                
                so.sem_num = 0;
                so.sem_op = -1;
                semop(id_semaforo_libro_mastro,&so,1);

                budget = calcolo_budget(libro_mastro,indice_libro_mastro,mypid,quantita_rifiutata,&t,&v,parametri_utente);

                so.sem_num = 0;
                so.sem_op = 1;
                semop(id_semaforo_libro_mastro,&so,1);
                
                if(budget >= 2){
                    nuova_transazione = creazione_transazione(budget,inizio,fine,mypid,vettore_pid_utenti,parametri_utente,id_semaforo_morte_utente);
                    nuovo_messaggio = crea_messaggio(nuova_transazione);
                    successo = invio_transazione_da_utente_a_nodo(nuovo_messaggio,SO_NODES_NUM,vettore_pid_nodi);
                    if(successo == 0){
                        v = v + nuovo_messaggio.transazione.quantita;
                    }
                    retry = 0;
                }
                else{
                    /*printf("Ho finito i soldi e non posso mandare la transazione segnalata\n");*/
                }
                flag = 0;
            }
        }
        TEST_ERROR;
        nanosleep(&req,NULL);
        sigprocmask(SIG_SETMASK,&maschera_origin,NULL);
        TEST_ERROR;
    }


}


void handler_segnale(int segnale){

    if(segnale == SIGUSR1){
        flag = 1;
    }
}