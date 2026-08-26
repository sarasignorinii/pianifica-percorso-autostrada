/* Prova finale di Algoritmi e Strutture Dati
 *
 * Simulazione di un'autostrada di stazioni di servizio con veicoli a noleggio,
 * e pianificazione del percorso con il minor numero di tappe fra due stazioni.
 * Le scelte di progetto e i costi delle operazioni sono spiegati nel README;
 * qui i commenti si limitano a cio' che serve per seguire il codice.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_AUTO 512

/* 
 * Lettura e scrittura bufferizzate
 *
 * Due buffer da 64 KB e un parser di interi scritto a mano, al posto di scanf
 * e printf che sui file di prova costerebbero piu' dell'algoritmo.
 */

#define DIM_BUFFER (64 * 1024)

static char bufferIngresso[DIM_BUFFER];
static int lunghezzaIngresso = 0;
static int posizioneIngresso = 0;

static char bufferUscita[DIM_BUFFER];
static int posizioneUscita = 0;

/* Restituisce il prossimo carattere dell'ingresso, oppure -1 a fine file. */
static int leggiCarattere(void)
{
    if (posizioneIngresso == lunghezzaIngresso) {
        lunghezzaIngresso = (int) fread(bufferIngresso, 1, DIM_BUFFER, stdin);
        posizioneIngresso = 0;
        if (lunghezzaIngresso <= 0)
            return -1;
    }
    return (unsigned char) bufferIngresso[posizioneIngresso++];
}

static void svuotaUscita(void)
{
    fwrite(bufferUscita, 1, (size_t) posizioneUscita, stdout);
    posizioneUscita = 0;
}

static void scriviCarattere(char c)
{
    if (posizioneUscita == DIM_BUFFER)
        svuotaUscita();
    bufferUscita[posizioneUscita++] = c;
}

static void scriviStringa(const char *s)
{
    while (*s != '\0')
        scriviCarattere(*s++);
}

/* Scrive un intero seguito dal separatore indicato (spazio o a capo). */
static void scriviNumero(unsigned valore, char separatore)
{
    char cifre[11];
    int n = 0;

    do {
        cifre[n++] = (char) ('0' + valore % 10);
        valore /= 10;
    } while (valore > 0);

    while (n > 0)
        scriviCarattere(cifre[--n]);
    scriviCarattere(separatore);
}

/* Legge la prossima parola non vuota. Restituisce 0 a fine file. */
static int leggiParola(char *destinazione, int dimensione)
{
    int c;
    int n = 0;

    do {
        c = leggiCarattere();
    } while (c == ' ' || c == '\n' || c == '\r' || c == '\t');

    if (c < 0)
        return 0;

    while (c > ' ') {
        if (n < dimensione - 1)
            destinazione[n++] = (char) c;
        c = leggiCarattere();
    }
    destinazione[n] = '\0';
    return 1;
}

/* Legge il prossimo intero senza segno saltando i separatori. */
static unsigned leggiNumero(void)
{
    unsigned valore = 0;
    int c;

    do {
        c = leggiCarattere();
        if (c < 0)
            return 0;
    } while (c < '0' || c > '9');

    while (c >= '0' && c <= '9') {
        valore = valore * 10 + (unsigned) (c - '0');
        c = leggiCarattere();
    }
    return valore;
}

/* 
 * Le stazioni: albero AVL ordinato per distanza
 */

typedef struct Stazione {
    struct Stazione *sinistra;
    struct Stazione *destra;
    unsigned *autonomie;    /* vettore ordinato in modo decrescente */
    unsigned distanza;      /* distanza da inizio autostrada */
    int numeroAuto;
    int capacita;           /* posti allocati in autonomie */
    int altezza;
} Stazione;

static Stazione *autostrada = NULL;

static int altezzaDi(const Stazione *s)
{
    return s != NULL ? s->altezza : 0;
}

static void aggiornaAltezza(Stazione *s)
{
    int a = altezzaDi(s->sinistra);
    int b = altezzaDi(s->destra);

    s->altezza = (a > b ? a : b) + 1;
}

static Stazione *ruotaADestra(Stazione *s)
{
    Stazione *nuovaRadice = s->sinistra;

    s->sinistra = nuovaRadice->destra;
    nuovaRadice->destra = s;
    aggiornaAltezza(s);
    aggiornaAltezza(nuovaRadice);
    return nuovaRadice;
}

static Stazione *ruotaASinistra(Stazione *s)
{
    Stazione *nuovaRadice = s->destra;

    s->destra = nuovaRadice->sinistra;
    nuovaRadice->sinistra = s;
    aggiornaAltezza(s);
    aggiornaAltezza(nuovaRadice);
    return nuovaRadice;
}

/* Ripristina la proprieta' AVL nel nodo s e restituisce la nuova radice. */
static Stazione *riequilibra(Stazione *s)
{
    int differenza;

    aggiornaAltezza(s);
    differenza = altezzaDi(s->sinistra) - altezzaDi(s->destra);

    if (differenza > 1) {
        if (altezzaDi(s->sinistra->sinistra) < altezzaDi(s->sinistra->destra))
            s->sinistra = ruotaASinistra(s->sinistra);
        return ruotaADestra(s);
    }
    if (differenza < -1) {
        if (altezzaDi(s->destra->destra) < altezzaDi(s->destra->sinistra))
            s->destra = ruotaADestra(s->destra);
        return ruotaASinistra(s);
    }
    return s;
}

static Stazione *cercaStazione(Stazione *s, unsigned distanza)
{
    while (s != NULL && s->distanza != distanza)
        s = distanza < s->distanza ? s->sinistra : s->destra;
    return s;
}

/* La distanza va verificata assente dal chiamante. In *creata torna il nodo. */
static Stazione *inserisciStazione(Stazione *s, unsigned distanza,
                                   Stazione **creata)
{
    if (s == NULL) {
        s = malloc(sizeof(Stazione));
        s->sinistra = NULL;
        s->destra = NULL;
        s->autonomie = NULL;
        s->distanza = distanza;
        s->numeroAuto = 0;
        s->capacita = 0;
        s->altezza = 1;
        *creata = s;
        return s;
    }

    if (distanza < s->distanza)
        s->sinistra = inserisciStazione(s->sinistra, distanza, creata);
    else
        s->destra = inserisciStazione(s->destra, distanza, creata);

    return riequilibra(s);
}

static Stazione *demolisciStazione(Stazione *s, unsigned distanza)
{
    if (s == NULL)
        return NULL;

    if (distanza < s->distanza) {
        s->sinistra = demolisciStazione(s->sinistra, distanza);
    } else if (distanza > s->distanza) {
        s->destra = demolisciStazione(s->destra, distanza);
    } else {
        free(s->autonomie);

        if (s->sinistra == NULL || s->destra == NULL) {
            Stazione *figlio = s->sinistra != NULL ? s->sinistra : s->destra;
            free(s);
            return figlio;
        }

        /* Con due figli si copia nel nodo il successore, cioe' la stazione
           piu' vicina nel sottoalbero destro, e si elimina quest'ultimo. */
        Stazione *successore = s->destra;
        while (successore->sinistra != NULL)
            successore = successore->sinistra;

        s->distanza = successore->distanza;
        s->autonomie = successore->autonomie;
        s->numeroAuto = successore->numeroAuto;
        s->capacita = successore->capacita;
        successore->autonomie = NULL;   /* evita la doppia deallocazione */

        s->destra = demolisciStazione(s->destra, successore->distanza);
    }

    return riequilibra(s);
}

static void liberaAutostrada(Stazione *s)
{
    if (s == NULL)
        return;
    liberaAutostrada(s->sinistra);
    liberaAutostrada(s->destra);
    free(s->autonomie);
    free(s);
}

/* 
 * Il parco veicoli di una stazione
 *
 * Invariante: le autonomie sono ordinate in modo decrescente, percio' quella
 * massima e' sempre in prima posizione.
 */

static int confrontoDecrescente(const void *a, const void *b)
{
    unsigned x = *(const unsigned *) a;
    unsigned y = *(const unsigned *) b;

    if (x > y)
        return -1;
    if (x < y)
        return 1;
    return 0;
}

static unsigned autonomiaMassima(const Stazione *s)
{
    return s->numeroAuto > 0 ? s->autonomie[0] : 0;
}

static void aggiungiAuto(Stazione *s, unsigned autonomia)
{
    int i;

    if (s->numeroAuto == s->capacita) {
        s->capacita = s->capacita > 0 ? s->capacita * 2 : 4;
        s->autonomie = realloc(s->autonomie,
                               (size_t) s->capacita * sizeof(unsigned));
    }

    /* Inserimento per scorrimento: al piu' 512 elementi da spostare. */
    for (i = s->numeroAuto; i > 0 && s->autonomie[i - 1] < autonomia; i--)
        s->autonomie[i] = s->autonomie[i - 1];

    s->autonomie[i] = autonomia;
    s->numeroAuto++;
}

/* Rimuove un'auto con l'autonomia indicata. Restituisce 1 se l'ha trovata. */
static int rottamaAuto(Stazione *s, unsigned autonomia)
{
    int primo = 0;
    int ultimo = s->numeroAuto - 1;

    while (primo <= ultimo) {
        int medio = (primo + ultimo) / 2;

        if (s->autonomie[medio] == autonomia) {
            memmove(s->autonomie + medio, s->autonomie + medio + 1,
                    (size_t) (s->numeroAuto - medio - 1) * sizeof(unsigned));
            s->numeroAuto--;
            return 1;
        }
        if (s->autonomie[medio] > autonomia)
            primo = medio + 1;      /* il vettore e' decrescente */
        else
            ultimo = medio - 1;
    }
    return 0;
}

/* 
 * La sintesi dell'autostrada
 *
 * Copia contigua e ordinata dell'albero con le sole informazioni che servono a
 * pianificare. Vale finche' sintesiValida resta 1: le modifiche alle stazioni
 * lo azzerano, le modifiche ai veicoli aggiornano la singola voce.
 */

typedef struct {
    unsigned distanza;
    unsigned autonomia;     /* autonomia massima disponibile nella stazione */
} VoceSintesi;

static VoceSintesi *sintesi = NULL;
static int *fascia = NULL;      /* confini delle fasce della visita */
static int *tappa = NULL;       /* percorso ricostruito */
static int numeroVoci = 0;
static int capacitaSintesi = 0;
static int sintesiValida = 0;

static void aggiungiVoce(unsigned distanza, unsigned autonomia)
{
    if (numeroVoci == capacitaSintesi) {
        capacitaSintesi = capacitaSintesi > 0 ? capacitaSintesi * 2 : 256;
        sintesi = realloc(sintesi, (size_t) capacitaSintesi * sizeof(VoceSintesi));
        fascia = realloc(fascia, (size_t) capacitaSintesi * sizeof(int));
        tappa = realloc(tappa, (size_t) capacitaSintesi * sizeof(int));
    }

    sintesi[numeroVoci].distanza = distanza;
    sintesi[numeroVoci].autonomia = autonomia;
    numeroVoci++;
}

/* La visita simmetrica produce le voci gia' ordinate per distanza. */
static void riempiSintesi(const Stazione *s)
{
    if (s == NULL)
        return;
    riempiSintesi(s->sinistra);
    aggiungiVoce(s->distanza, autonomiaMassima(s));
    riempiSintesi(s->destra);
}

/* Posizione di una distanza nella sintesi, oppure -1 se assente. */
static int cercaVoce(unsigned distanza)
{
    int primo = 0;
    int ultimo = numeroVoci - 1;

    while (primo <= ultimo) {
        int medio = (primo + ultimo) / 2;

        if (sintesi[medio].distanza == distanza)
            return medio;
        if (sintesi[medio].distanza < distanza)
            primo = medio + 1;
        else
            ultimo = medio - 1;
    }
    return -1;
}

/* 
 * Pianificazione del percorso
 *
 * Le stazioni utilizzabili sono numerate da 0 (partenza) a n-1 (arrivo) nel
 * verso di marcia, cosi' i due sensi di percorrenza usano lo stesso codice:
 * dalla tappa p si raggiunge ogni tappa successiva che disti dalla partenza
 * non piu' di quanto permetta l'autonomia disponibile in p.
 */

typedef struct {
    int primaVoce;      /* posizione della partenza nella sintesi */
    int passo;          /* +1 verso distanze crescenti, -1 nell'altro verso */
    int numeroTappe;    /* stazioni comprese fra partenza e arrivo */
    unsigned partenza;  /* distanza della stazione di partenza */
} Viaggio;

static unsigned distanzaTappa(const Viaggio *v, int p)
{
    return sintesi[v->primaVoce + p * v->passo].distanza;
}

/* Chilometri percorsi dalla partenza fino alla tappa p: crescono con p. */
static unsigned kmPercorsi(const Viaggio *v, int p)
{
    unsigned d = distanzaTappa(v, p);

    return v->passo > 0 ? d - v->partenza : v->partenza - d;
}

/* Chilometri raggiungibili noleggiando alla tappa p il veicolo migliore. */
static unsigned kmRaggiungibili(const Viaggio *v, int p)
{
    unsigned percorsi = kmPercorsi(v, p);
    unsigned autonomia = sintesi[v->primaVoce + p * v->passo].autonomia;

    if (percorsi > UINT_MAX - autonomia)
        return UINT_MAX;            /* satura invece di andare in overflow */
    return percorsi + autonomia;
}

static void pianificaPercorso(unsigned partenza, unsigned arrivo)
{
    Viaggio v;
    int voceArrivo;
    int numeroFasce;
    int i;

    if (partenza == arrivo) {
        scriviNumero(partenza, '\n');
        return;
    }

    if (!sintesiValida) {
        numeroVoci = 0;
        riempiSintesi(autostrada);
        sintesiValida = 1;
    }

    v.primaVoce = cercaVoce(partenza);
    voceArrivo = cercaVoce(arrivo);
    if (v.primaVoce < 0 || voceArrivo < 0) {
        scriviStringa("nessun percorso\n");
        return;
    }

    v.passo = partenza < arrivo ? 1 : -1;
    v.partenza = partenza;
    v.numeroTappe = (voceArrivo - v.primaVoce) * v.passo + 1;

    /* Visita in ampiezza. Da una tappa si raggiunge sempre un intervallo
       contiguo di tappe successive, quindi quelle raggiungibili con L
       spostamenti formano un blocco e basta ricordarne l'ultimo indice:
       fascia[L] = ultima tappa raggiungibile con L spostamenti. */
    numeroFasce = 0;
    fascia[0] = 0;

    while (fascia[numeroFasce] < v.numeroTappe - 1) {
        unsigned limite = 0;
        int prima = numeroFasce > 0 ? fascia[numeroFasce - 1] + 1 : 0;
        int ultima = fascia[numeroFasce];

        for (i = prima; i <= ultima; i++) {
            unsigned raggiungibili = kmRaggiungibili(&v, i);
            if (raggiungibili > limite)
                limite = raggiungibili;
        }

        i = ultima;
        while (i + 1 < v.numeroTappe && kmPercorsi(&v, i + 1) <= limite)
            i++;

        if (i == ultima) {          /* la fascia non si estende: si e' bloccati */
            scriviStringa("nessun percorso\n");
            return;
        }
        numeroFasce++;
        fascia[numeroFasce] = i;
    }

    /* Ricostruzione a ritroso dall'arrivo. Fra i predecessori validi della
       fascia precedente si sceglie il primo se si viaggia in avanti, l'ultimo
       se si torna indietro: in entrambi i casi la stazione piu' vicina
       all'inizio dell'autostrada, come chiede la regola di spareggio. Ogni
       tappa di una fascia sta su un percorso minimo, quindi scegliere una
       tappa per volta basta. */
    tappa[numeroFasce] = v.numeroTappe - 1;

    for (i = numeroFasce; i > 0; i--) {
        int prima = i >= 2 ? fascia[i - 2] + 1 : 0;
        int ultima = fascia[i - 1];
        unsigned obiettivo = kmPercorsi(&v, tappa[i]);

        if (v.passo > 0) {
            while (kmRaggiungibili(&v, prima) < obiettivo)
                prima++;
            tappa[i - 1] = prima;
        } else {
            while (kmRaggiungibili(&v, ultima) < obiettivo)
                ultima--;
            tappa[i - 1] = ultima;
        }
    }

    for (i = 0; i <= numeroFasce; i++)
        scriviNumero(distanzaTappa(&v, tappa[i]),
                     i == numeroFasce ? '\n' : ' ');
}

/* 
 * Esecuzione dei comandi
 */

static void comandoAggiungiStazione(void)
{
    unsigned autonomie[MAX_AUTO];
    unsigned distanza = leggiNumero();
    unsigned quante = leggiNumero();
    Stazione *s;
    unsigned i;

    /* Da leggere comunque: anche se la stazione esiste vanno tolte dall'ingresso. */
    for (i = 0; i < quante; i++) {
        unsigned autonomia = leggiNumero();
        if (i < MAX_AUTO)
            autonomie[i] = autonomia;
    }
    if (quante > MAX_AUTO)
        quante = MAX_AUTO;

    if (cercaStazione(autostrada, distanza) != NULL) {
        scriviStringa("non aggiunta\n");
        return;
    }

    autostrada = inserisciStazione(autostrada, distanza, &s);
    if (quante > 0) {
        qsort(autonomie, quante, sizeof(unsigned), confrontoDecrescente);
        s->autonomie = malloc((size_t) quante * sizeof(unsigned));
        memcpy(s->autonomie, autonomie, (size_t) quante * sizeof(unsigned));
        s->numeroAuto = (int) quante;
        s->capacita = (int) quante;
    }

    sintesiValida = 0;
    scriviStringa("aggiunta\n");
}

static void comandoDemolisciStazione(void)
{
    unsigned distanza = leggiNumero();

    if (cercaStazione(autostrada, distanza) == NULL) {
        scriviStringa("non demolita\n");
        return;
    }

    autostrada = demolisciStazione(autostrada, distanza);
    sintesiValida = 0;
    scriviStringa("demolita\n");
}

static void comandoAggiungiAuto(void)
{
    unsigned distanza = leggiNumero();
    unsigned autonomia = leggiNumero();
    Stazione *s = cercaStazione(autostrada, distanza);

    if (s == NULL) {
        scriviStringa("non aggiunta\n");
        return;
    }

    aggiungiAuto(s, autonomia);
    if (sintesiValida) {
        int voce = cercaVoce(distanza);
        if (autonomia > sintesi[voce].autonomia)
            sintesi[voce].autonomia = autonomia;
    }
    scriviStringa("aggiunta\n");
}

static void comandoRottamaAuto(void)
{
    unsigned distanza = leggiNumero();
    unsigned autonomia = leggiNumero();
    Stazione *s = cercaStazione(autostrada, distanza);

    if (s == NULL || !rottamaAuto(s, autonomia)) {
        scriviStringa("non rottamata\n");
        return;
    }

    if (sintesiValida)
        sintesi[cercaVoce(distanza)].autonomia = autonomiaMassima(s);
    scriviStringa("rottamata\n");
}

static void comandoPianificaPercorso(void)
{
    unsigned partenza = leggiNumero();
    unsigned arrivo = leggiNumero();

    pianificaPercorso(partenza, arrivo);
}

int main(void)
{
    char comando[32];

    while (leggiParola(comando, sizeof(comando))) {
        if (strcmp(comando, "aggiungi-stazione") == 0)
            comandoAggiungiStazione();
        else if (strcmp(comando, "demolisci-stazione") == 0)
            comandoDemolisciStazione();
        else if (strcmp(comando, "aggiungi-auto") == 0)
            comandoAggiungiAuto();
        else if (strcmp(comando, "rottama-auto") == 0)
            comandoRottamaAuto();
        else if (strcmp(comando, "pianifica-percorso") == 0)
            comandoPianificaPercorso();
    }

    svuotaUscita();
    liberaAutostrada(autostrada);
    free(sintesi);
    free(fascia);
    free(tappa);
    return 0;
}