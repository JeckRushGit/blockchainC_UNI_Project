#include "header.h"
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}


parametri parametri_init(){
    int i,j;
    parametri parametri;
    FILE * file_parametri;
    char * stringa,*stringatmp1,*stringatmp2;
    char * p,*p1;
    char c;
    size_t lunghezza_riga = 0;
    size_t lunghezza_stringa = LUNGHEZZA_STRINGA_INIT;

    stringa = calloc(lunghezza_stringa,sizeof(char));

    file_parametri = fopen("file_parametri.txt","r");

    while((c = fgetc(file_parametri)) != EOF){
        if(c != '\n'){
            if(lunghezza_riga >= lunghezza_stringa){
                lunghezza_stringa = lunghezza_stringa * 2;
                stringa = realloc(stringa,lunghezza_stringa);
            }
            stringa[lunghezza_riga] = c;
            lunghezza_riga++;
        }
        else{
        

        stringatmp1 = calloc(sizeof(stringa),sizeof(char));
        stringatmp2 = calloc(sizeof(stringa),sizeof(char));
    
    
        rimuovi_spazi_vuoti(stringa);
        i = 0;
        c = stringa[i];
        while(c != '='){
            stringatmp1[i] = c;
            i++;
            c = stringa[i];
        }
        i++;
        j = 0;
        c = stringa[i];
        while(c != ';'){
            stringatmp2[j] = c;
            i++;
            j++;
            c = stringa[i];
        }
       
        if(!strcmp(stringatmp1,"SO_USERS_NUM") ){
            parametri.SO_USERS_NUM = atoi(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_NODES_NUM") ){
            parametri.SO_NODES_NUM = atoi(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_BUDGET_INIT") ){
            parametri.SO_BUDGET_INIT = atoi(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_REWARD") ){
            parametri.SO_REWARD = atoi(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_MIN_TRANS_GEN_NSEC") ){
            parametri.SO_MIN_TRANS_GEN_NSEC = atol(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_MAX_TRANS_GEN_NSEC") ){
            parametri.SO_MAX_TRANS_GEN_NSEC = atol(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_RETRY") ){
            parametri.SO_RETRY = atoi(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_TP_SIZE") ){
            parametri.SO_TP_SIZE = atoi(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_MIN_TRANS_PROC_NSEC") ){
            parametri.SO_MIN_TRANS_PROC_NSEC = atol(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_MAX_TRANS_PROC_NSEC") ){
            parametri.SO_MAX_TRANS_PROC_NSEC = atol(stringatmp2);
        }
        if(!strcmp(stringatmp1,"SO_SIM_SEC") ){
            parametri.SO_SIM_SEC = atoi(stringatmp2);
        }
        lunghezza_stringa = LUNGHEZZA_STRINGA_INIT;
        stringa = calloc(lunghezza_stringa,sizeof(char));
        lunghezza_riga = 0;
        }
    }

    fclose(file_parametri);
    free(stringa);
    free(stringatmp1);
    free(stringatmp2);

    return parametri;
}

void rimuovi_spazi_vuoti(char *str){
	int i = 0, j = 0;
	while (str[i])
	{
		if (str[i] != ' ')
          str[j++] = str[i];
		i++;
	}
	str[j] = '\0';

}

id_t creazione_semaforo_singolo(int chiave,int valore_iniziale){

    id_t id_semaforo;
    id_semaforo = semget(chiave,1,IPC_CREAT | 0600);
    semctl(id_semaforo,0,SETVAL,valore_iniziale);
    return id_semaforo;

}

int estrazione_int_casuale(int min,int max){
    return(rand() % (max+1 - min) + min);
}

long estrazione_long_casuale(long min,long max){
    return(rand() % (max+1 - min) + min);

}

int estrazione_casuale_dal_bilancio(int bilancio){
    return estrazione_int_casuale(2,bilancio);
}

int calcolo_reward(int bilancio_estratto,int SO_REWARD){
    int tmp;
    tmp = ((bilancio_estratto * SO_REWARD)/100);
    if(tmp < 1){
        return 1;
    }
    return tmp;
}

pid_t estrazione_nodo(pid_t * vettore_pid_nodi,int SO_NODES_NUM){
    int random_index = estrazione_int_casuale(0,(SO_NODES_NUM-1));
    return vettore_pid_nodi[random_index];
}

struct timespec calcolo_breakpoint(struct timespec inizio,struct timespec fine){
    struct timespec delta;
    
    delta.tv_sec = fine.tv_sec - inizio.tv_sec;
    delta.tv_nsec = fine.tv_nsec - inizio.tv_nsec;
    if(delta.tv_sec > 0 && delta.tv_nsec < 0){
        delta.tv_nsec += NS_AL_SECONDO;
        delta.tv_sec--;
    }
    else if (delta.tv_sec < 0 && delta.tv_nsec > 0)
    {
        delta.tv_nsec -= NS_AL_SECONDO;
        delta.tv_sec++;
    }
    
    return delta;
}

pid_t estrazione_utente(pid_t * vettore_pid_utenti,int SO_USERS_NUM,id_t id_semaforo_morte_utente){

    struct sembuf so;
    int random_index,i;
    pid_t mypid = getpid();
    size_t num_utenti_vivi = 0;

    so.sem_num = 0;
    so.sem_op = -1;
    semop(id_semaforo_morte_utente,&so,1);
   
    
    for(i = 0; i < SO_USERS_NUM ; i++){
        if(vettore_pid_utenti[i] != 0){
            num_utenti_vivi++;
        }
    }
    
    if(num_utenti_vivi < 2){
        /*printf("Non ci sono abbastanza utenti vivi\n");*/
        so.sem_num = 0;
        so.sem_op = 1;
        semop(id_semaforo_morte_utente,&so,1);
       
    }
    else{
        do{
            random_index = estrazione_int_casuale(0,(SO_USERS_NUM-1));
        }while(vettore_pid_utenti[random_index] == 0 || vettore_pid_utenti[random_index] == mypid);

        so.sem_num = 0;
        so.sem_op = 1;
        semop(id_semaforo_morte_utente,&so,1);
        

        return vettore_pid_utenti[random_index];
    }
}



transazione creazione_transazione(int bilancio,struct timespec inizio,struct timespec fine,pid_t mypid,pid_t * vettore_pid_utenti,parametri parametri_utente,id_t id_semaforo_morte_utente){

    transazione nuova_transazione;

    nuova_transazione.timestamp = calcolo_breakpoint(inizio,fine);
    nuova_transazione.sender = mypid;
    nuova_transazione.receiver = estrazione_utente(vettore_pid_utenti,parametri_utente.SO_USERS_NUM,id_semaforo_morte_utente);
    nuova_transazione.quantita = estrazione_casuale_dal_bilancio(bilancio);
    nuova_transazione.reward = calcolo_reward(nuova_transazione.quantita,parametri_utente.SO_REWARD);

    return nuova_transazione;
}



mymsg crea_messaggio(transazione nuova_transazione){

    mymsg nuovo_messaggio;
    nuovo_messaggio.mtype = 1;
    nuovo_messaggio.transazione = nuova_transazione;
    return nuovo_messaggio;
    
}
int invio_transazione_da_utente_a_nodo(mymsg nuovo_messaggio,int SO_NODES_NUM,int * vettore_pid_nodi){
    int random_index,pid_nodo;
    id_t id_coda_casuale;
    struct msqid_ds msgds;
    int num_messaggi_coda;
    random_index = estrazione_int_casuale(0,(SO_NODES_NUM-1));
    pid_nodo = vettore_pid_nodi[random_index];
    id_coda_casuale = msgget(pid_nodo,0600);
    msgctl(id_coda_casuale,IPC_STAT,&msgds);
    num_messaggi_coda = msgds.msg_qnum;
    /*printf("Sono %d == %d e invierò a %d :     %d\n",nuovo_messaggio.transazione.sender,getpid(),nuovo_messaggio.transazione.receiver,nuovo_messaggio.transazione.quantita);*/
    /*printf("Ci sono %d messaggi nella coda\n",num_messaggi_coda);*/
    if(num_messaggi_coda < 512){
        msgsnd(id_coda_casuale,&nuovo_messaggio,sizeof(transazione),0);
        return 0;
    }
    else return -1;
    

}


int calcolo_budget(blocco * libro_mastro,int * indice_libro_mastro,pid_t mypid,int soldi_rifiutati,int * t,int * v,parametri parametri_utente){

    int i,j;
    int soldi_ottenuti = 0,soldi_inviati = 0,soldi_persi_tmp = 0;
    int budget = 0;

    /*printf("Il libro mastro si trova ad indice : %d\n",*indice_libro_mastro);*/

    for(i = 0 ; i <= *indice_libro_mastro; i++){
        for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
            if(libro_mastro[i][j].receiver == mypid){
                soldi_ottenuti = soldi_ottenuti + ((libro_mastro[i][j].quantita) - (libro_mastro[i][j].reward));
            }
        }
    }
    
    for(i = 0 ; i <= *indice_libro_mastro; i++){
        for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
            if(libro_mastro[i][j].sender == mypid){
                soldi_inviati += (libro_mastro[i][j].quantita);
            }
        }
    }
    
    for(i = *t; i <= *indice_libro_mastro ; i++){
        for(j = 0; j < (SO_BLOCK_SIZE-1) ; j++){
            if(libro_mastro[i][j].sender == mypid){
                    soldi_persi_tmp = soldi_persi_tmp + (libro_mastro[i][j].quantita);
            }
        }
    }
    

    *t = *indice_libro_mastro;
    *v = *v - soldi_persi_tmp - soldi_rifiutati;
    soldi_persi_tmp = 0;
    soldi_rifiutati = 0;

    budget = parametri_utente.SO_BUDGET_INIT + soldi_ottenuti - soldi_inviati - *v;
    soldi_ottenuti = 0;
    soldi_inviati = 0;

    return budget;
}

void richiesta_denaro_segnale(int mypid,int * vettore_pid_utenti,id_t id_semaforo_morte_utente,int SO_USERS_NUM){
    size_t i;
    struct sembuf so;
    int num_proc_vivi = 0;
    int random_index = 0;
    
    
    so.sem_num = 0;
    so.sem_op = -1;
    semop(id_semaforo_morte_utente,&so,1);
   

    for(i = 0; i < SO_USERS_NUM ; i++){
        if(vettore_pid_utenti[i] != 0){
            num_proc_vivi++;
        }
        if(num_proc_vivi >= 2){
            break;
        }
    }
    if(num_proc_vivi >= 2){
        do{
            random_index = estrazione_int_casuale(0,(SO_USERS_NUM-1));
        }while(vettore_pid_utenti[random_index] == 0 || vettore_pid_utenti[random_index] == mypid);    
        kill(vettore_pid_utenti[random_index],SIGUSR1);
    }
    else{
        /*printf("Non ci sono abbastanza utenti in vita\n");*/
    }
    so.sem_num = 0;
    so.sem_op = 1;
    semop(id_semaforo_morte_utente,&so,1);
    
}

void morte_processo(int mypid,int * vettore_pid_utenti,int SO_USERS_NUM,id_t id_semaforo_morte_utente){
    size_t i;
    struct sembuf so;

    so.sem_num = 0;
    so.sem_op = -1;
    semop(id_semaforo_morte_utente,&so,1);
      
    

    for(i = 0; i < SO_USERS_NUM ; i++){
        if(vettore_pid_utenti[i] == mypid){
            vettore_pid_utenti[i] = 0;
            break;
        }
    }

    so.sem_num = 0;
    so.sem_op = 1;
    semop(id_semaforo_morte_utente,&so,1);
    
    kill(mypid,SIGTERM);
}

void stampa_budget_processi(int * vettore_pid_nodi,int * copia_vettore_pid_utenti,blocco * libro_mastro,int * indice_libro_mastro,int SO_USERS_NUM,int SO_BUDGET_INIT,int SO_NODES_NUM){
    int pid_utente_maggiore = 0,pid_tmp = 0;
    int pid_utente_minore = 0;
    int quantita_pid_utente_minore = SO_BUDGET_INIT;
    int pid_utente = 0; int quantita_pid_utente = SO_BUDGET_INIT;
    int quantita_pid_utente_maggiore = SO_BUDGET_INIT,quantita_pid_utente_tmp = SO_BUDGET_INIT;
    int pid_nodo = 0;
    int pid_nodo_maggiore = 0,pid_nodo_minore = 0;
    int quantita_pid_nodo_maggiore = 0, quantita_pid_nodo_minore = 0;
    int quantita_pid_nodo_tmp = 0;

    size_t i,j,n;
    
    if(SO_USERS_NUM <= NUM_STAMPA){
        for(n = 0 ; n < SO_USERS_NUM ; n++){
            pid_utente = copia_vettore_pid_utenti[n];
            quantita_pid_utente = SO_BUDGET_INIT;
            for(i = 0 ; i < *indice_libro_mastro ; i++){
                for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
                    if(libro_mastro[i][j].sender == pid_utente){
                        quantita_pid_utente = quantita_pid_utente - libro_mastro[i][j].quantita;
                    }
                    else if(libro_mastro[i][j].receiver == pid_utente){
                        quantita_pid_utente = quantita_pid_utente + (libro_mastro[i][j].quantita - libro_mastro[i][j].reward);
                    }
                }
            }
            printf("L'utente %d ha budget : %d\n",pid_utente,quantita_pid_utente);
        }
        for(n = 0 ; n < SO_NODES_NUM ; n++){
            pid_nodo = vettore_pid_nodi[n];
            quantita_pid_nodo_tmp = 0;
            for(i = 0 ; i < *indice_libro_mastro ; i++){
                if(libro_mastro[i][(SO_BLOCK_SIZE-1)].receiver == pid_nodo){
                    quantita_pid_nodo_tmp = quantita_pid_nodo_tmp + libro_mastro[i][(SO_BLOCK_SIZE-1)].quantita;
                }
            }
            printf("Il nodo  %d ha budget : %d\n",pid_nodo,quantita_pid_nodo_tmp);
        }



    }
    else{
        pid_utente_maggiore = copia_vettore_pid_utenti[0];
        for(i = 0 ; i < *indice_libro_mastro ; i++){
            for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
                if(libro_mastro[i][j].sender == pid_utente_maggiore){
                    quantita_pid_utente_maggiore = quantita_pid_utente_maggiore - libro_mastro[i][j].quantita;
                }
                else if(libro_mastro[i][j].receiver == pid_utente_maggiore){
                    quantita_pid_utente_maggiore = quantita_pid_utente_maggiore + (libro_mastro[i][j].quantita - libro_mastro[i][j].reward);
                }
            }
        }
        for(n = 1 ; n < SO_USERS_NUM ; n++){
            pid_tmp = copia_vettore_pid_utenti[n];
            quantita_pid_utente_tmp = SO_BUDGET_INIT;
            for(i = 0 ; i < *indice_libro_mastro ; i++){
                for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
                    if(libro_mastro[i][j].sender == pid_tmp){
                        quantita_pid_utente_tmp = quantita_pid_utente_tmp - libro_mastro[i][j].quantita;
                    }
                    else if(libro_mastro[i][j].receiver == pid_tmp){
                        quantita_pid_utente_tmp = quantita_pid_utente_tmp + (libro_mastro[i][j].quantita - libro_mastro[i][j].reward);
                    }
                }
            }
            if(quantita_pid_utente_maggiore < quantita_pid_utente_tmp){
                pid_utente_maggiore = pid_tmp;
                quantita_pid_utente_maggiore = quantita_pid_utente_tmp;
            }
        }
        printf("L'utente con più budget è  : %d = %d\n",pid_utente_maggiore,quantita_pid_utente_maggiore);

        pid_utente_minore= copia_vettore_pid_utenti[0];
        for(i = 0 ; i < *indice_libro_mastro ; i++){
            for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
                if(libro_mastro[i][j].sender == pid_utente_minore){
                    quantita_pid_utente_minore = quantita_pid_utente_minore - libro_mastro[i][j].quantita;
                }
                else if(libro_mastro[i][j].receiver == pid_utente_minore){
                    quantita_pid_utente_minore = quantita_pid_utente_minore + (libro_mastro[i][j].quantita - libro_mastro[i][j].reward);
                }
            }
        }
        for(n = 1 ; n < SO_USERS_NUM ; n++){
            pid_tmp = copia_vettore_pid_utenti[n];
            quantita_pid_utente_tmp = SO_BUDGET_INIT;
            for(i = 0 ; i < *indice_libro_mastro ; i++){
                for(j = 0 ; j < (SO_BLOCK_SIZE-1) ; j++){
                    if(libro_mastro[i][j].sender == pid_tmp){
                        quantita_pid_utente_tmp = quantita_pid_utente_tmp - libro_mastro[i][j].quantita;
                    }
                    else if(libro_mastro[i][j].receiver == pid_tmp){
                        quantita_pid_utente_tmp = quantita_pid_utente_tmp + (libro_mastro[i][j].quantita - libro_mastro[i][j].reward);
                    }
                }
            }
            if(quantita_pid_utente_minore > quantita_pid_utente_tmp){
                pid_utente_minore = pid_tmp;
                quantita_pid_utente_minore = quantita_pid_utente_tmp;
            }
        }
        printf("L'utente con meno budget è : %d = %d\n",pid_utente_minore,quantita_pid_utente_minore);

        pid_nodo_maggiore = vettore_pid_nodi[0];
        for(i = 0; i < *indice_libro_mastro; i++){
            if(libro_mastro[i][SO_BLOCK_SIZE-1].receiver == pid_nodo_maggiore){
                quantita_pid_nodo_maggiore = quantita_pid_nodo_maggiore + libro_mastro[i][SO_BLOCK_SIZE-1].quantita;
            }
        }
        for(n = 1; n < SO_NODES_NUM ; n++){
            pid_tmp = vettore_pid_nodi[n];
            quantita_pid_nodo_tmp = 0;
            for(i = 0; i < *indice_libro_mastro; i++){
                if(libro_mastro[i][SO_BLOCK_SIZE-1].receiver == pid_tmp){
                    quantita_pid_nodo_tmp = quantita_pid_nodo_tmp + libro_mastro[i][SO_BLOCK_SIZE-1].quantita;
                }
            }
        }
        if(quantita_pid_nodo_maggiore < quantita_pid_nodo_tmp){
            pid_nodo_maggiore = pid_tmp;
            quantita_pid_nodo_maggiore = quantita_pid_nodo_tmp;
        }
        printf("Il nodo con più budget è   : %d = %d\n",pid_nodo_maggiore,quantita_pid_nodo_maggiore);

        pid_nodo_minore = vettore_pid_nodi[0];
        for(i = 0; i < *indice_libro_mastro; i++){
            if(libro_mastro[i][SO_BLOCK_SIZE-1].receiver == pid_nodo_minore){
                quantita_pid_nodo_minore = quantita_pid_nodo_minore + libro_mastro[i][SO_BLOCK_SIZE-1].quantita;
            }
        }
        for(n = 1; n < SO_NODES_NUM ; n++){
            pid_tmp = vettore_pid_nodi[n];
            quantita_pid_nodo_tmp = 0;
            for(i = 0; i < *indice_libro_mastro; i++){
                if(libro_mastro[i][SO_BLOCK_SIZE-1].receiver == pid_tmp){
                    quantita_pid_nodo_tmp = quantita_pid_nodo_tmp + libro_mastro[i][SO_BLOCK_SIZE-1].quantita;
                }
            }
        }
        if(quantita_pid_nodo_minore > quantita_pid_nodo_tmp){
            pid_nodo_minore = pid_tmp;
            quantita_pid_nodo_minore = quantita_pid_nodo_tmp;
        }
        printf("Il nodo con meno budget è  : %d = %d\n",pid_nodo_minore,quantita_pid_nodo_minore);
    }
}