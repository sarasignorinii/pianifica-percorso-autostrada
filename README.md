# highway-route-planner

Prova finale di Algoritmi e Strutture Dati, anno accademico 2022–2023.
C standard, un unico file, nessuna libreria esterna.
Specifica completa in [`specifica.pdf`](specifica.pdf).

## Il problema

Un'autostrada è una sequenza di stazioni di servizio, ognuna identificata dalla
distanza dall'inizio e dotata di al più 512 veicoli elettrici a noleggio: da una
stazione si raggiungono tutte quelle che distano non più dell'autonomia del
veicolo noleggiato. Il programma legge da standard input una sequenza di comandi
che modificano stazioni e veicoli, e per ogni richiesta pianifica il percorso con
il minor numero di tappe. A parità di tappe vince il percorso che predilige le
stazioni più vicine all'inizio dell'autostrada.

Compila senza warning con le stesse opzioni usate dal verificatore ufficiale:

```sh
gcc -Wall -Werror -std=gnu11 -O2 -lm autostrada.c -o autostrada
./autostrada < input.txt > output.txt
```

## Comandi

| Comando | Risposta |
|---|---|
| `aggiungi-stazione <distanza> <n> <autonomia_1> ... <autonomia_n>` | `aggiunta` / `non aggiunta` |
| `demolisci-stazione <distanza>` | `demolita` / `non demolita` |
| `aggiungi-auto <distanza> <autonomia>` | `aggiunta` / `non aggiunta` |
| `rottama-auto <distanza> <autonomia>` | `rottamata` / `non rottamata` |
| `pianifica-percorso <partenza> <arrivo>` | le tappe separate da spazi, oppure `nessun percorso` |

Con quattro stazioni a distanza 20 (autonomie 5, 10, 15, 25), 30 (40), 45 (30) e
50 (20, 25), `pianifica-percorso 50 20` risponde `50 30 20`: il percorso
`50 45 20` ha lo stesso numero di tappe, ma 30 è più vicina all'inizio
dell'autostrada di 45.

## Scelte implementative

**Stazioni: albero AVL** ordinato per distanza. Ricerca, inserimento e
demolizione in O(log n), memoria proporzionale alle sole stazioni esistenti.

**Veicoli: vettore ordinato in modo decrescente**, allocato su misura invece che
con capacità fissa 512. L'autonomia massima, l'unico dato che serve a
pianificare, è sempre il primo elemento; la rottamazione usa la ricerca binaria.

**Sintesi dell'autostrada.** Pianificare richiede di scorrere le stazioni in
ordine, e sull'albero questo significa una visita con molti salti in memoria. Il
programma mantiene perciò un vettore contiguo di coppie (distanza, autonomia
massima), rigenerato solo quando l'insieme delle stazioni cambia: aggiungere o
rottamare un'auto aggiorna la singola voce.

**Percorso: visita in ampiezza a fasce.** Da una stazione si raggiunge sempre un
intervallo contiguo di stazioni successive, quindi quelle raggiungibili con L
spostamenti formano un blocco di cui basta ricordare l'ultimo indice: la visita
costa O(k) sul vettore contiguo, senza allocazioni né code esplicite. Il percorso
si ricostruisce a ritroso dall'arrivo scegliendo, fra i predecessori validi della
fascia precedente, il primo se si viaggia in avanti e l'ultimo se si torna
indietro — in entrambi i casi la stazione più vicina all'inizio dell'autostrada,
cioè la regola di spareggio. Numerando le tappe nel verso di marcia, i due sensi
di percorrenza condividono lo stesso codice.

**I/O bufferizzato.** I file di prova contengono milioni di token: leggerli con
`scanf` costerebbe più dell'algoritmo. Due buffer da 64 KB e un parser di interi
scritto a mano.

## Verifica in locale

Per confrontare l'output del programma con quello atteso di un caso di test:

```sh
./autostrada < caso.txt > risultato.txt
diff atteso.txt risultato.txt
```

`diff` non stampa nulla se i due file sono identici. Durante lo sviluppo il
programma è stato anche controllato con AddressSanitizer (`gcc -fsanitize=address`)
e con Valgrind Memcheck per escludere accessi fuori dai limiti e perdite di
memoria; i due strumenti vanno usati in compilazioni separate, perché sono
incompatibili fra loro. I dettagli d'uso sono in [`docs/note-strumenti.md`](docs/note-strumenti.md).

## Costi

n = stazioni sull'autostrada, k = stazioni fra partenza e arrivo, m ≤ 512 auto.

| Operazione | Costo |
|---|---|
| `aggiungi-stazione` | O(log n + m log m) |
| `demolisci-stazione` | O(log n) |
| `aggiungi-auto`, `rottama-auto` | O(log n + m) |
| `pianifica-percorso` | O(k), più O(n) per rigenerare la sintesi dopo una modifica alle stazioni |

La sintesi si rigenera per intero dopo ogni aggiunta o demolizione di una
stazione: renderla incrementale conviene solo se le modifiche alle stazioni si
alternano fittamente alle pianificazioni.