#include "header.h"
#define SENDER_REWARD -1
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}




transazione * transaction_pool;
transazione * copia_tp;
size_t num_elem_transaction_pool = 0;


void handler_segnale(int segnale);

int main(int argc,char * argv[]){
    blocco * libro_mastro;
    int * indice_libro_mastro;

    int SO_TP_SIZE;
    int random_index;
    id_t id_coda_transazioni_in;
    id_t id_semaforo_attesa_code;
    id_t id_coda_di_messaggi_transazioni_rifiutate;
    id_t id_semaforo_libro_mastro;
    id_t id_semaforo_indice_libro_mastro;
    id_t id_libro_mastro;
    id_t id_indice_libro_mastro;
    size_t num_messaggi_coda = 0;
    size_t i;
    int mypid;
    int quantita_reward = 0;
    struct sembuf so;
    struct sigaction sa;
    struct timespec inizio,fine;
    struct msqid_ds msgds;
    struct timespec req;
    mymsg messaggio_transazione_in,messaggio_transazione_rifiutata;
    transazione transazione_reward;
    blocco nuovo_blocco;
    sigset_t maschera;
    parametri parametri_nodo;
    long SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC;
    long attesa_proc_blocco = 0;


    mypid = getpid();
    srand(mypid);

    /*AGGIUNGO SIGINT AI SEGNALI DA GESTIRE*/
    sigemptyset(&maschera);
    sa.sa_mask = maschera;
    sa.sa_handler = handler_segnale;
    sigaction(SIGINT,&sa,NULL);

    /*OTTENGO I PARAMETRI */
    parametri_nodo = parametri_init();
    SO_TP_SIZE = parametri_nodo.SO_TP_SIZE;
    SO_MIN_TRANS_PROC_NSEC = parametri_nodo.SO_MIN_TRANS_PROC_NSEC;
    SO_MAX_TRANS_PROC_NSEC = parametri_nodo.SO_MAX_TRANS_PROC_NSEC;

    attesa_proc_blocco = estrazione_long_casuale(SO_MIN_TRANS_PROC_NSEC,SO_MAX_TRANS_PROC_NSEC);
    
    req.tv_sec = attesa_proc_blocco / 1000000000;
    req.tv_nsec = attesa_proc_blocco - (req.tv_sec * 1000000000);


    /*OTTENGO L'ID DELLA CODA DI MESSAGGI RIFIUTATI*/
    id_coda_di_messaggi_transazioni_rifiutate = msgget(CHIAVE_CODA_DI_MESSAGGI_TRANSAZIONI_RIFIUTATE,0600);

    /*OTTENGO L'ID DEL SEMAFORO ATTESA CODE*/
    id_semaforo_attesa_code = semget(CHIAVE_SEM_ATTESA_CODE,0,0600);

    /*ALLOCO LO SPAZIO PER LA TRANSACTION POOL E LA SUA COPIA*/
    transaction_pool = calloc(SO_TP_SIZE,sizeof(transazione));
    copia_tp = calloc(SO_TP_SIZE,sizeof(transazione));

    /*OTTENGO L'ID DEL LIBRO MASTRO E DEL SUO INDICE*/
    id_libro_mastro = shmget(CHIAVE_LIBRO_MASTRO,0,0600);
    id_indice_libro_mastro = shmget(CHIAVE_INDICE_LIBRO_MASTRO,0,0600);
    /*FACCIO L'ATTACH*/
    libro_mastro = (blocco *)shmat(id_libro_mastro,NULL,0);
    indice_libro_mastro = (int *)shmat(id_indice_libro_mastro,NULL,0);
    /*OTTENGO L'ID DEI SEMAFORI LIBRO MASTRO E INDICE LIBRO MASTRO*/
    id_semaforo_libro_mastro = semget(CHIAVE_SEM_LIBRO_MASTRO,0,0600);
    id_semaforo_indice_libro_mastro = semget(CHIAVE_SEM_INDICE_LIBRO_MASTRO,0,0600);

    /*ASPETTO CHE LE CODE SIANO STATE CREATE*/
    so.sem_num = 0;
    so.sem_op = 0;
    semop(id_semaforo_attesa_code,&so,1);
    /*POSSO PARTIRE*/

    /*OTTENGO L'ID DELLA MIA CODA DI MESSAGGI IN ENTRATA*/
    id_coda_transazioni_in = msgget(mypid,0600);
    
    


    clock_gettime(CLOCK_REALTIME,&inizio);
    while(TRUE){
        bzero(&messaggio_transazione_in,sizeof(mymsg));
        msgctl(id_coda_transazioni_in,IPC_STAT,&msgds);
        num_messaggi_coda = msgds.msg_qnum;
        i = 0;
        while(num_messaggi_coda > 0 && num_elem_transaction_pool < SO_TP_SIZE){
                msgrcv(id_coda_transazioni_in,&messaggio_transazione_in,sizeof(transazione),0,0);
                
                num_messaggi_coda--;
                while(transaction_pool[i].quantita != 0){
                    i++;
                }
                transaction_pool[i] = messaggio_transazione_in.transazione;
                num_elem_transaction_pool++;       
        }
        while(num_messaggi_coda > 0){
            msgrcv(id_coda_transazioni_in,&messaggio_transazione_rifiutata,sizeof(transazione),0,0);
            num_messaggi_coda--;
            messaggio_transazione_rifiutata.mtype = messaggio_transazione_rifiutata.transazione.sender;
            msgsnd(id_coda_di_messaggi_transazioni_rifiutate,&messaggio_transazione_rifiutata,sizeof(transazione),0);
        }
        /*printf("Ci sono %ld transazioni nella tp\n",num_elem_transaction_pool);*/
        

        memset(nuovo_blocco,0,sizeof(nuovo_blocco));
        quantita_reward = 0;
        if(num_elem_transaction_pool >= (SO_BLOCK_SIZE-1)){
                bzero(&transazione_reward,sizeof(transazione));
                clock_gettime(CLOCK_REALTIME,&fine);
                memcpy(copia_tp,transaction_pool,SO_TP_SIZE * sizeof(transazione));
                for(i = 0 ; i < (SO_BLOCK_SIZE-1); i++){
                    do{
                        random_index = estrazione_int_casuale(0,(SO_TP_SIZE-1));
                    }while(copia_tp[random_index].quantita == 0);
                    nuovo_blocco[i] = copia_tp[random_index];
                    bzero(&transaction_pool[random_index],sizeof(transazione));
                    quantita_reward = quantita_reward + nuovo_blocco[i].reward;
                    copia_tp[random_index].quantita = 0;
                }
                transazione_reward.timestamp = calcolo_breakpoint(inizio,fine);
                transazione_reward.sender = SENDER_REWARD;
                transazione_reward.receiver = mypid;
                transazione_reward.quantita = quantita_reward;
                transazione_reward.reward = 0;
                nuovo_blocco[(SO_BLOCK_SIZE-1)] = transazione_reward;
                num_elem_transaction_pool = num_elem_transaction_pool - (SO_BLOCK_SIZE-1);   

                if(*indice_libro_mastro < SO_REGISTRY_SIZE){
                    so.sem_num  = 0;
                    so.sem_op = -1;

                    semop(id_semaforo_libro_mastro,&so,1);
                    
                    memcpy(libro_mastro[*indice_libro_mastro],nuovo_blocco,sizeof(blocco));
                    
                    *indice_libro_mastro = *indice_libro_mastro + 1;

                    so.sem_num  = 0;
                    so.sem_op = 1;
                    semop(id_semaforo_libro_mastro,&so,1);
                }
                else{
                     /*printf("Libro mastro pieno\n");*/
                }             
        }
        TEST_ERROR;
        nanosleep(&req,NULL);
    }
}


void handler_segnale(int segnale){


    
    if(segnale == SIGINT){
        
        printf("Nella transaction pool di %d sono presenti : %ld transazioni\n",getpid(),num_elem_transaction_pool);
        free(transaction_pool);
        free(copia_tp);
        TEST_ERROR;
        kill(getpid(),SIGTERM);
    }




}