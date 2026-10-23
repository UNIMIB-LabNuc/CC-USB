# CC-USB

Programmi in C per acquisire dati da un crate CAMAC attraverso il controller
USB **Wiener CC-USB**, con ADC, TDC e scaler.

| Programma | Cosa fa | Secondo argomento |
|---|---|---|
| `status` | inizializza il crate e mostra moduli presenti e LAM | — |
| `ADC` | eventi dall'ADC (attesa LAM) | numero di eventi |
| `TDC` | eventi dal TDC (attesa LAM) | numero di eventi |
| `TDC_ADC` | eventi in coincidenza ADC + TDC | numero di eventi |
| `SCA` | conteggi dello scaler a intervalli regolari | numero di misure |
| `TDC_ADC_SCA` | eventi ADC + TDC a blocchi di tempo, con lo scaler letto a fine blocco | numero di misure |

## Struttura

```
CC-USB/
├── Makefile
├── include/
│   ├── config.h          configurazione comune: DATA_DIR, slot dei moduli, timeout LAM
│   ├── xxusb_helper.h    controller CC-USB e crate (apertura, init, shutdown, scan)
│   ├── camac_module.h    moduli CAMAC (LAM, clear, lettura canali, scaler)
│   └── daq.h             argomenti, file di output, Ctrl-C, scrittura dei dati
├── lib/                  implementazione del codice comune
├── src/                  un file per programma: src/NOME.c -> bin/NOME
├── scripts/
│   └── lancia_scan_Vbias.py   scan in tensione: imposta l'HV e lancia SCA
├── build/obj/  build/dep/     file oggetto e dipendenze (generati da make)
└── bin/                       eseguibili (generati da make)
```

## Installazione

### 1. Libreria xxusb

I programmi usano la libreria Wiener `libxxusb` (testata con la versione 3.22),
che a sua volta usa `libusb-0.1` (pacchetto `libusb-compat` o `libusb-devel`
a seconda della distribuzione). Il Makefile si aspetta:

```
/usr/include/libxxusb.h
/usr/lib/libxx_usb.so      (o un'altra cartella di sistema delle librerie)
```

Se la libreria è installata altrove si indicano le cartelle a `make`:

```
make XXUSB_INC=/percorso/include XXUSB_LIB=/percorso/lib
```

### 2. Permessi sul dispositivo USB

Senza configurazione l'accesso al CC-USB richiede i privilegi di root. Per
usarlo da utente normale si crea il file `/etc/udev/rules.d/cc_usb.rules`
con la riga:

```
SUBSYSTEM=="usb", ATTR{idVendor}=="16dc", ATTR{idProduct}=="0001", MODE="0666"
```

(16dc = Wiener, 0001 = CC-USB), poi si ricaricano le regole e si ricollega
il cavo USB:

```
sudo udevadm control --reload-rules
```

### 3. Compilazione

```
make            # compila tutti i programmi in bin/
make clean      # cancella build/ e bin/
make print      # mostra sorgenti, eseguibili e flag usati
```

Dopo ogni modifica a un file `.c` o `.h` basta rilanciare `make`: vengono
ricompilati solo i file che ne dipendono.

## Configurazione

La configurazione è su due livelli: quella **comune** a tutti i programmi in
`include/config.h`, e quella **specifica** di ogni programma nei `#define` in
cima al suo file in `src/`.

### Configurazione comune: `include/config.h`

| Parametro | Significato | Default |
|---|---|---|
| `DATA_DIR` | cartella in cui tutti i programmi scrivono i file di output | `/home/labstudent/Desktop/Muoni_25-26/data` |
| `ADC_SLOT` | stazione dell'ADC nel crate | 6 |
| `TDC_SLOT` | stazione del TDC | 16 |
| `SCA_SLOT` | stazione dello scaler | 2 |
| `LAM_TIMEOUT_US` | tempo massimo di attesa del LAM per un evento (µs) | 1000000 |
| `LAM_POLL_US` | intervallo tra due test del LAM (µs) | 10 |

Se si sposta un modulo nel crate, o si cambia la cartella dei dati, si
modifica `config.h` e si rilancia `make`: tutti i programmi usano il nuovo
valore. Per controllare quali moduli rispondono e in che slot, usare
`./bin/status`.

`DATA_DIR` si può anche cambiare al momento della compilazione, senza
modificare il file:

```
make DATA_DIR=/home/labstudent/Desktop/Muoni_26-27/data
```

La cartella deve esistere: i programmi non la creano.

### Configurazione di ogni programma

In cima a ogni file in `src/` ci sono i parametri specifici del programma:

| Programma | Parametri |
|---|---|
| `ADC` | `OUTPUT_FILE`, `EVENTS`, `ADC_CHANNEL_MIN`, `ADC_CHANNEL_MAX` |
| `TDC` | `OUTPUT_FILE`, `EVENTS`, `TDC_CHANNEL_MIN`, `TDC_CHANNEL_MAX` |
| `TDC_ADC` | `OUTPUT_FILE`, `EVENTS`, canali ADC e TDC |
| `SCA` | `OUTPUT_FILE`, `N_MEAS`, `TIME` (durata di una misura, s), `TIME_STAMP` (intervallo tra le letture, s), `N_CHANNELS` |
| `TDC_ADC_SCA` | `OUTPUT_FILE`, `N_MEAS`, `MEAS_TIME` (durata di un blocco, s), canali ADC e TDC, `SCA_N_CHANNELS` |

`OUTPUT_FILE` è il nome del file di default, `EVENTS` e `N_MEAS` sono i
valori di default del secondo argomento da riga di comando.

## Uso

Tutti i programmi di acquisizione hanno la stessa riga di comando:

```
./bin/PROGRAMMA [file_output] [quantità]
```

- `file_output`: nome del file, creato in `DATA_DIR`. Se inizia con `/` è
  usato come percorso completo. Default: `OUTPUT_FILE` del programma.
  **Un file esistente con lo stesso nome viene sovrascritto.**
- `quantità`: numero di eventi (`ADC`, `TDC`, `TDC_ADC`) o di misure
  (`SCA`, `TDC_ADC_SCA`). Default: `EVENTS` o `N_MEAS` del programma.
- `-h` mostra l'uso, con la cartella dei dati e i valori di default.

Esempi:

```
./bin/status
./bin/ADC                        # DATA_DIR/ADC.txt, EVENTS eventi
./bin/ADC run_42.txt 5000        # DATA_DIR/run_42.txt, 5000 eventi
./bin/SCA soglia15.txt 5         # 5 misure di TIME secondi nello stesso file
./bin/TDC_ADC_SCA /tmp/prova.txt 1
```

### Interruzione con Ctrl-C

Tutti i programmi si interrompono pulitamente con **Ctrl-C**:

1. il ciclo di acquisizione termina subito, anche se il programma è in attesa
   del LAM o in pausa tra due letture dello scaler;
2. i moduli vengono azzerati, il crate messo in inhibit e il CC-USB chiuso;
3. il file di output viene chiuso: i dati già acquisiti sono salvati (ogni
   riga è scritta su disco appena acquisita).

Un secondo Ctrl-C termina il programma immediatamente, senza cleanup (da
usare solo se il primo non ha effetto, ad esempio se il CC-USB non risponde).

### Formato dei file di output

Una riga per evento o lettura, valori separati da spazi. Un canale ADC o
TDC la cui lettura non è valida (Q o X diversi da 1) vale `-1`. I valori sono
sempre quelli grezzi del modulo: calibrazioni e tagli si fanno in analisi.

- **ADC, TDC**: i canali da `*_CHANNEL_MIN` a `*_CHANNEL_MAX`.
- **TDC_ADC**: prima i canali ADC, poi quelli TDC.
- **SCA**: per ogni misura, una riga di conteggi ogni `TIME_STAMP` secondi
  (cumulativi dall'inizio della misura), poi una riga vuota e
  `Tempo di acquisizione: T secondi`. Se interrotta, T è il tempo
  effettivamente acquisito.
- **TDC_ADC_SCA**: per ogni misura, le righe degli eventi (come `TDC_ADC`),
  poi `EOM` seguito dai conteggi dello scaler. Se interrotta, l'ultima misura
  è più corta di `MEAS_TIME`.

### Scan in tensione

`scripts/lancia_scan_Vbias.py` imposta l'alta tensione (alimentatore CAEN
N1470) per ogni valore in `VOLTAGES`, attende la stabilizzazione e lancia
`bin/SCA` con un file di output che contiene tensione e ora. Alla fine, o in
caso di errore, spegne l'HV.

```
./scripts/lancia_scan_Vbias.py
```

## Funzionamento

### Lettura a eventi (ADC, TDC, TDC_ADC, TDC_ADC_SCA)

1. `crate_init`: Z, C e rimozione dell'inhibit;
2. `camac_setup_modules`: abilita il LAM dei moduli (F(26)) e li azzera (F(9));
3. per ogni evento, `camac_read_event`:
   - attende il LAM di **tutti** i moduli dell'evento con F(8);
   - legge i canali con F(0);
   - azzera i moduli con F(9).

   Se dopo `LAM_TIMEOUT_US` non tutti i moduli hanno il LAM, i moduli vengono
   azzerati e l'attesa ricomincia. Se alcuni moduli avevano il LAM e altri
   no, lo segnala su stderr;
4. alla fine, `crate_shutdown`: inhibit, C, Z e chiusura del CC-USB.

### Scaler

Lo scaler è azzerato all'inizio di ogni misura leggendo ogni canale con F(2)
(lettura e azzeramento) e letto con F(0) (lettura senza azzeramento).

## Aggiungere un programma

1. Creare `src/NUOVO.c`, partendo dal programma più simile. Ad esempio, per
   una coincidenza di più moduli basta aggiungere una riga all'array
   `modules[]` di `TDC_ADC.c`.
2. Usare le funzioni di `include/*.h` invece di riscrivere apertura del
   CC-USB, attesa del LAM, gestione del file e di Ctrl-C.
3. `make`: il Makefile compila automaticamente ogni file in `src/` in
   `bin/NUOVO`.

I file in `src/` e in `lib/` devono avere nomi diversi, perché i loro oggetti
finiscono nella stessa cartella `build/obj/`.
