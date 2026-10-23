//****************************************************************************
/*    TDC

Acquisisce eventi dal TDC (LRS 2228, slot TDC_SLOT in config.h): per ogni
evento attende il LAM, legge i canali TDC_CHANNEL_MIN..TDC_CHANNEL_MAX e
azzera il modulo.

I valori sono scritti grezzi (conteggi del TDC, nessun taglio):
calibrazione, tagli su zero/overflow e istogrammi si fanno in analisi.

Uso:
  ./bin/TDC                     -> DATA_DIR/TDC.txt, EVENTS eventi
  ./bin/TDC output.txt          -> DATA_DIR/output.txt
  ./bin/TDC output.txt 5000     -> 5000 eventi

Formato del file: una riga per evento
  TDC[TDC_CHANNEL_MIN] ... TDC[TDC_CHANNEL_MAX]
(-1 se la lettura di un canale non è valida)

Ctrl-C interrompe l'acquisizione: gli eventi già acquisiti restano nel file.
*/
//****************************************************************************

#define OUTPUT_FILE "TDC.txt"
#define EVENTS 10000

/* il canale 3 corrisponde all'ingresso CH4 sul pannello frontale */
#define TDC_CHANNEL_MIN 3
#define TDC_CHANNEL_MAX 3
#define TDC_NCH (TDC_CHANNEL_MAX - TDC_CHANNEL_MIN + 1)

#include <stdio.h>

#include "config.h"
#include "daq.h"
#include "camac_module.h"
#include "xxusb_helper.h"

static const camac_module modules[] = {
    { "TDC", TDC_SLOT, TDC_CHANNEL_MIN, TDC_CHANNEL_MAX },
};

int main(int argc, char **argv)
{
    const char *filename;
    long n_events;

    if (daq_parse_args(argc, argv, OUTPUT_FILE, "n_eventi", EVENTS, &filename, &n_events) != 0) {
        return 1;
    }

    daq_install_signal_handlers();

    FILE *out = daq_open_output(filename);
    if (!out) {
        return 1;
    }

    usb_dev_handle *handle = xxusb_open_first_device();
    if (!handle) {
        fclose(out);
        return 1;
    }

    crate_init(handle);
    crate_scan_slots(handle);
    camac_setup_modules(handle, modules, N_ELEMENTS(modules));

    daq_print_start(n_events, "eventi");

    long data[TDC_NCH];
    long count = 0;

    while (count < n_events && !daq_stop_requested()) {
        if (camac_read_event(handle, modules, N_ELEMENTS(modules), data) == 0) {
            continue;
        }

        daq_write_line(out, NULL, data, TDC_NCH);
        ++count;
    }

    camac_clear_modules(handle, modules, N_ELEMENTS(modules));
    crate_shutdown(handle);
    fclose(out);

    daq_print_summary(count, "Eventi salvati");

    return 0;
}
