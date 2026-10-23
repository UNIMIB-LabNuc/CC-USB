//****************************************************************************
/*    Configurazione comune a tutti i programmi

Questo è l'unico posto dove si definiscono:
  - la cartella dove vengono scritti i file di output (DATA_DIR)
  - la posizione (stazione N) dei moduli nel crate
  - i parametri di attesa del LAM

Dopo una modifica basta rilanciare "make": ricompila tutto ciò che serve.
DATA_DIR si può anche cambiare senza toccare questo file:
  make DATA_DIR=/altra/cartella
*/
//****************************************************************************

#ifndef CONFIG_H
#define CONFIG_H

/* cartella dei file di output */
#ifndef DATA_DIR
#define DATA_DIR "/home/labstudent/Desktop/Muoni_25-26/data"
#endif

/* stazioni (slot) dei moduli nel crate */
#define ADC_SLOT 6
#define TDC_SLOT 16
#define SCA_SLOT 2

/* attesa del LAM: timeout e intervallo di polling (microsecondi) */
#define LAM_TIMEOUT_US 1000000
#define LAM_POLL_US 10

#endif
