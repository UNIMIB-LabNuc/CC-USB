//****************************************************************************
/*    Funzioni comuni di gestione dell'acquisizione

  - argomenti da riga di comando: [file_output] [quantità]
  - apertura del file di output in DATA_DIR
  - interruzione pulita con Ctrl-C
  - scrittura di una riga di dati su schermo e su file
*/
//****************************************************************************

#ifndef DAQ_H
#define DAQ_H

#include <stdio.h>

/* Legge gli argomenti opzionali:
     argv[1] = nome del file di output (default: default_name)
     argv[2] = quantità da acquisire, intero > 0 (default: default_count)
   count_label è il nome della quantità mostrato nell'uso (es. "n_eventi").
   Ritorna 0 se gli argomenti sono validi, -1 altrimenti (e stampa l'uso). */
int daq_parse_args(int argc, char **argv,
                   const char *default_name,
                   const char *count_label, long default_count,
                   const char **filename, long *count);

/* Apre in scrittura DATA_DIR/filename (o filename, se inizia con '/').
   Ritorna NULL in caso di errore. */
FILE *daq_open_output(const char *filename);

/* Ctrl-C (SIGINT) e SIGTERM chiedono l'interruzione dell'acquisizione:
   il programma termina il ciclo, pulisce i moduli e chiude il file.
   Un secondo Ctrl-C termina il programma immediatamente. */
void daq_install_signal_handlers(void);
int daq_stop_requested(void);

/* Attende seconds secondi. Ritorna 0 se interrotto da Ctrl-C, 1 altrimenti. */
int daq_sleep(unsigned int seconds);

/* Tempo in secondi da un'origine arbitraria (orologio monotono). */
double daq_now(void);

/* Scrive una riga "[prefix] v0 v1 ... vn-1" su schermo e su file. */
void daq_write_line(FILE *out, const char *prefix, const long *values, int n);

/* Messaggi di inizio e fine acquisizione,
   es. daq_print_start(1000, "eventi"), daq_print_summary(n, "Eventi salvati"). */
void daq_print_start(long count, const char *what);
void daq_print_summary(long count, const char *what);

#endif
