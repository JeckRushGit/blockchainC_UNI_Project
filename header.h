#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define NS_AL_SECONDO 1000000000
#define LUNGHEZZA_STRINGA_INIT 50
#define NUM_STAMPA 20

#define SO_BLOCK_SIZE 10
#define SO_REGISTRY_SIZE 10000

#define CHIAVE_VETTORE_PID_NODI 55
#define CHIAVE_VETTORE_PID_UTENTI 56
#define CHIAVE_LIBRO_MASTRO 57
#define CHIAVE_INDICE_LIBRO_MASTRO 58

#define CHIAVE_SEM_ATTESA_CREAZIONE_UTENTI 1
#define CHIAVE_SEM_MORTE_UTENTI 2
#define CHIAVE_SEM_LIBRO_MASTRO 3
#define CHIAVE_SEM_INDICE_LIBRO_MASTRO 4
#define CHIAVE_SEM_ATTESA_CODE 5


#define CHIAVE_CODA_DI_MESSAGGI_TRANSAZIONI_RIFIUTATE 1

#define TRUE 1
#define FALSE 0


typedef struct {
    int SO_USERS_NUM;
    int SO_NODES_NUM;
    int SO_BUDGET_INIT;
    int SO_REWARD;
    long SO_MIN_TRANS_GEN_NSEC;
    long SO_MAX_TRANS_GEN_NSEC;
    int SO_RETRY;
    int SO_TP_SIZE;
    long SO_MIN_TRANS_PROC_NSEC;
    long SO_MAX_TRANS_PROC_NSEC;
    int SO_SIM_SEC;
    
}parametri;

typedef struct {
    struct timespec timestamp;
    int sender;
    int receiver;
    int quantita;
    int reward;
}transazione;

typedef struct{
    long mtype;
    transazione transazione;
}mymsg;

typedef transazione blocco[SO_BLOCK_SIZE];

/*RITORNA UNA STRUCT DI TIPO PARAMETRI CHE CONTIENE TUTTI I PARAMETRI DI CONFIGURAZIONE LETTI A RUNTIME DAL FILE "file_parametri.txt" */
parametri parametri_init();

/*RIMUOVE GLI SPAZI VUOTI NELLA STRINGA str*/
void rimuovi_spazi_vuoti(char *str);

/*CREA ED INIZIALIZZA UN SEMAFORO CON UN SOLO SEMAFORO NELL'ARRAY E LO INIZIALIZZA CON IL VALORE IN valore_iniziale*/
id_t creazione_semaforo_singolo(int chiave,int valore_iniziale);

/*RITORNA UN INTERO ESTRATTO CASUALMENTE TRA min e max*/
int estrazione_int_casuale(int min,int max);

/*RITORNA UN LONG ESTRATTO CASUALMENTE TRA min e max*/
long estrazione_long_casuale(long min,long max);

/*ESTRAE CASUALMENTE E RITORNA UN INTERO TRA 2 E bilancio*/
int estrazione_casuale_dal_bilancio(int bilancio);

/*RITORNA UNA QUANTITA INTERA PARI AD UNA PERCENTUALE SO_REWARD CON MINIMO DI 1*/
int calcolo_reward(int bilancio_estratto,int SO_REWARD);

/*ESTRAE E RITORNA UN PID CASUALE DAL VETTORE PID NODI*/
pid_t estrazione_nodo(pid_t * vettore_pid_nodi,int SO_NODES_NUM);

/*RITORNA UNA STRUTTURA DI TIPO timespec CALCOLATA A PARTIRE DALL'INIZIO DELLA SIMULAZIONE (inizio) E L'ISTANTE IN CUI VIENE ESEGUITA UNA TRANSAZIONE(fine)*/
struct timespec calcolo_breakpoint(struct timespec inizio,struct timespec fine);

/*ESTRAE E RITORNA UN PID CASUALE DAL VETTORE PID UTENTI CHE DEVE ESSERE ACCEDUTO IN MANIERA MUTUALMENTE ESCLUSIVA
ATTRAVERSO IL SEMAFORO id_semaforo_morte_utente*/
pid_t estrazione_utente(pid_t * vettore_pid_utenti,int SO_USERS_NUM,id_t id_semaforo_morte_utente);

/*RITORNA UNA STRUCT DI TIPO TRANSAZIONE A PARTIRE DAI VALORI PASSATI COME ARGOMENTO*/
transazione creazione_transazione(int bilancio,struct timespec inizio,struct timespec fine,pid_t mypid,pid_t * vettore_pid_utenti,parametri parametri_utente,id_t id_semaforo_morte_utente);

/*CREA UN MESSAGGIO DA INVIARE CON LA nuova_transazione DI TIPO 0*/
mymsg crea_messaggio(transazione nuova_transazione);

/*INVIA IL MESSAGGIO AD UN NODO ESTRATTO A CASO DA 0 A SO_NODES_NUM-1 DA vettore_pid_nodi*/
int invio_transazione_da_utente_a_nodo(mymsg nuovo_messaggio,int SO_NODES_NUM,int * vettore_pid_nodi);

/*CALCOLA IL BUDGET CON L'AUSILIO DI DUE VARIABILI QUALI v E t CHE TERRA' CONTO DELLE TRANSAZIONI IN USCITA*/
int calcolo_budget(blocco * libro_mastro,int * indice_libro_mastro,pid_t mypid,int soldi_rifiutati,int * t,int * v,parametri parametri_utente);

/*ESTRAE UN PID UTENTE DA vettore_pid_utenti A CUI INVIARE IL SEGNALE*/
void richiesta_denaro_segnale(int mypid,int * vettore_pid_utenti,id_t id_semaforo_morte_utente,int SO_USERS_NUM);

/*ELIMINA SE STESSO DA vettore_pid_utenti METTENDO 0 NELL'ENTRY*/
void morte_processo(int mypid,int * vettore_pid_utenti,int SO_USERS_NUM,id_t id_semaforo_morte_utente);

/*STAMPA IL BUDGET DEI PROCESSI NODO E UTENTI TRAMITE LA LETTURA DAL LIBRO MASTRO*/
void stampa_budget_processi(int * vettore_pid_nodi,int * copia_vettore_pid_utenti,blocco * libro_mastro,int * indice_libro_mastro,int SO_USERS_NUM,int SO_BUDGET_INIT,int SO_NODES_NUM);