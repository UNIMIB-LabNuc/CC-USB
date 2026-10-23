#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "daq.h"
#include "camac_module.h"

int camac_module_nch(const camac_module *module)
{
    return module->ch_max - module->ch_min + 1;
}

/* F(26): abilita il LAM. Ritorna 1 se il modulo ha risposto (X=1). */
int camac_enable_lam(usb_dev_handle *handle, int slot)
{
    int q = 0, x = 0;

    CAMAC_write(handle, slot, 0, 26, 0, &q, &x);
    return x == 1;
}

/* F(9): azzera il modulo e il suo LAM. */
void camac_clear(usb_dev_handle *handle, int slot)
{
    long dummy = 0;
    int q = 0, x = 0;

    CAMAC_read(handle, slot, 0, 9, &dummy, &q, &x);
}

/* F(8): ritorna 1 se il modulo ha il LAM attivo (Q=1). */
int camac_test_lam(usb_dev_handle *handle, int slot)
{
    long dummy = 0;
    int q = 0, x = 0;

    if (CAMAC_read(handle, slot, 0, 8, &dummy, &q, &x) < 0) {
        return 0;
    }
    return q == 1 && x == 1;
}

/* F(0): legge i canali ch_min..ch_max in data[0..].
   Un canale con lettura non valida (errore USB, Q o X diversi da 1) vale -1. */
void camac_read_channels(usb_dev_handle *handle, int slot, int ch_min, int ch_max, long *data)
{
    for (int ch = ch_min; ch <= ch_max; ch++) {
        long value = 0;
        int q = 0, x = 0;

        if (CAMAC_read(handle, slot, ch, 0, &value, &q, &x) < 0 || q != 1 || x != 1) {
            value = -1;
        }
        data[ch - ch_min] = value;
    }
}

void camac_setup_modules(usb_dev_handle *handle, const camac_module *modules, int n)
{
    for (int i = 0; i < n; i++) {
        int ok = camac_enable_lam(handle, modules[i].slot);
        printf("Enable LAM %s (slot %d): %s\n",
               modules[i].name, modules[i].slot, ok ? "ok" : "NESSUNA RISPOSTA (X=0)");
    }
    camac_clear_modules(handle, modules, n);
}

void camac_clear_modules(usb_dev_handle *handle, const camac_module *modules, int n)
{
    for (int i = 0; i < n; i++) {
        camac_clear(handle, modules[i].slot);
    }
}

/* Attende che tutti i moduli abbiano il LAM attivo.
   Ritorna 1 se l'evento è completo, 0 se scade il timeout o arriva Ctrl-C.
   Se solo alcuni moduli hanno il LAM allo scadere del timeout, lo segnala. */
int camac_wait_lam(usb_dev_handle *handle, const camac_module *modules, int n)
{
    const unsigned int all = (1u << n) - 1;
    const double timeout = LAM_TIMEOUT_US * 1e-6;
    const double start = daq_now();
    unsigned int ready = 0;

    while (!daq_stop_requested()) {
        for (int i = 0; i < n; i++) {
            if (!(ready & (1u << i)) && camac_test_lam(handle, modules[i].slot)) {
                ready |= 1u << i;
            }
        }

        if (ready == all) {
            return 1;
        }

        if (daq_now() - start >= timeout) {
            break;
        }

        usleep(LAM_POLL_US);
    }

    if (ready != 0 && !daq_stop_requested()) {
        fprintf(stderr, "LAM timeout, evento incompleto:");
        for (int i = 0; i < n; i++) {
            fprintf(stderr, " %s=%d", modules[i].name, (ready >> i) & 1u);
        }
        fprintf(stderr, "\n");
    }

    return 0;
}

/* Un evento: attende il LAM di tutti i moduli, legge i loro canali uno di
   seguito all'altro in data[] e azzera i moduli.
   Ritorna il numero di valori letti, 0 se non c'è stato un evento completo
   (in quel caso i moduli vengono comunque azzerati). */
int camac_read_event(usb_dev_handle *handle, const camac_module *modules, int n, long *data)
{
    if (!camac_wait_lam(handle, modules, n)) {
        camac_clear_modules(handle, modules, n);
        return 0;
    }

    int n_values = 0;
    for (int i = 0; i < n; i++) {
        camac_read_channels(handle, modules[i].slot, modules[i].ch_min, modules[i].ch_max,
                            &data[n_values]);
        n_values += camac_module_nch(&modules[i]);
    }

    camac_clear_modules(handle, modules, n);

    return n_values;
}

/* F(2) su ogni canale: lettura e azzeramento. */
void scaler_clear(usb_dev_handle *handle, int slot, int n_channels)
{
    for (int ch = 0; ch < n_channels; ch++) {
        long dummy = 0;
        int q = 0, x = 0;

        CAMAC_read(handle, slot, ch, 2, &dummy, &q, &x);
    }
}

/* F(0) su ogni canale: lettura senza azzeramento.
   Un canale vale -1 solo se la transazione USB fallisce (Q non è controllato). */
void scaler_read(usb_dev_handle *handle, int slot, int n_channels, long *data)
{
    for (int ch = 0; ch < n_channels; ch++) {
        long value = 0;
        int q = 0, x = 0;

        if (CAMAC_read(handle, slot, ch, 0, &value, &q, &x) < 0) {
            value = -1;
        }
        data[ch] = value;
    }
}
