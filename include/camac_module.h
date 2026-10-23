//****************************************************************************
/*    Funzioni comuni per i moduli CAMAC (ADC, TDC, SCALER)

Un modulo letto a eventi (ADC, TDC) è descritto da un camac_module:
nome, slot e intervallo di canali da leggere. Le funzioni che prendono
un array di moduli li trattano come un unico evento: si attende il LAM
di tutti, si leggono tutti i canali, si azzerano tutti.

Funzioni CAMAC usate:
  F(0)  lettura          F(2)  lettura e azzeramento (scaler)
  F(8)  test LAM         F(9)  azzeramento modulo e LAM
  F(26) abilita LAM
*/
//****************************************************************************

#ifndef CAMAC_MODULE_H
#define CAMAC_MODULE_H

#include <libxxusb.h>

typedef struct {
    const char *name;   /* usato solo nei messaggi */
    int slot;
    int ch_min;         /* primo canale letto */
    int ch_max;         /* ultimo canale letto (incluso) */
} camac_module;

/* numero di elementi di un array statico */
#define N_ELEMENTS(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* numero di canali letti di un modulo */
int camac_module_nch(const camac_module *module);

/* singolo modulo */
int camac_enable_lam(usb_dev_handle *handle, int slot);
void camac_clear(usb_dev_handle *handle, int slot);
int camac_test_lam(usb_dev_handle *handle, int slot);
void camac_read_channels(usb_dev_handle *handle, int slot, int ch_min, int ch_max, long *data);

/* gruppo di moduli letti insieme a ogni evento */
void camac_setup_modules(usb_dev_handle *handle, const camac_module *modules, int n);
void camac_clear_modules(usb_dev_handle *handle, const camac_module *modules, int n);
int camac_wait_lam(usb_dev_handle *handle, const camac_module *modules, int n);
int camac_read_event(usb_dev_handle *handle, const camac_module *modules, int n, long *data);

/* scaler */
void scaler_clear(usb_dev_handle *handle, int slot, int n_channels);
void scaler_read(usb_dev_handle *handle, int slot, int n_channels, long *data);

#endif
