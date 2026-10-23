//****************************************************************************
/*    Funzioni per il controller CC-USB e per il crate

  xxusb_open_first_device  apre il primo CC-USB trovato sul bus USB
  xxusb_close_device       chiude la connessione
  crate_init               Z, C, I(false): crate inizializzato e acquisizione abilitata
  crate_shutdown           I(true), C, Z e chiusura della connessione
  crate_scan_slots         stampa gli slot che rispondono a F(0)·A(0)
  xxusb_read_lam_register  legge il registro LAM del CC-USB (N=25 A=10 F=0)
  xxusb_print_active_lams  stampa gli slot con LAM attivo
*/
//****************************************************************************

#ifndef XXUSB_HELPER_H
#define XXUSB_HELPER_H

#include <libxxusb.h>

/* numero di stazioni del crate controllate dal registro LAM */
#define CRATE_N_SLOTS 24

usb_dev_handle *xxusb_open_first_device(void);
void xxusb_close_device(usb_dev_handle *handle);

void crate_init(usb_dev_handle *handle);
void crate_shutdown(usb_dev_handle *handle);
void crate_scan_slots(usb_dev_handle *handle);

unsigned long xxusb_read_lam_register(usb_dev_handle *handle, int *q, int *x);
void xxusb_print_active_lams(usb_dev_handle *handle);

#endif
