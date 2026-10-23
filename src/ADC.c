//****************************************************************************
/*    ADC

Acquisisce eventi dall'ADC (slot ADC_SLOT in config.h): per ogni evento
attende il LAM, legge i canali ADC_CHANNEL_MIN..ADC_CHANNEL_MAX e azzera
il modulo.

Uso:
  ./bin/ADC                     -> DATA_DIR/ADC.txt, EVENTS eventi
  ./bin/ADC output.txt          -> DATA_DIR/output.txt
  ./bin/ADC output.txt 5000     -> 5000 eventi

Formato del file: una riga per evento
  ADC[ADC_CHANNEL_MIN] ... ADC[ADC_CHANNEL_MAX]
(-1 se la lettura di un canale non è valida)

Ctrl-C interrompe l'acquisizione: gli eventi già acquisiti restano nel file.
*/
//****************************************************************************

#define OUTPUT_FILE "ADC.txt"
#define EVENTS 10000

#define ADC_CHANNEL_MIN 0
#define ADC_CHANNEL_MAX 11
#define ADC_NCH (ADC_CHANNEL_MAX - ADC_CHANNEL_MIN + 1)

#include <stdio.h>

#include "config.h"
#include "daq.h"
#include "camac_module.h"
#include "xxusb_helper.h"

static const camac_module modules[] = {
    { "ADC", ADC_SLOT, ADC_CHANNEL_MIN, ADC_CHANNEL_MAX },
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

    long data[ADC_NCH];
    long count = 0;

    while (count < n_events && !daq_stop_requested()) {
        if (camac_read_event(handle, modules, N_ELEMENTS(modules), data) == 0) {
            continue;
        }

        daq_write_line(out, NULL, data, ADC_NCH);
        ++count;
    }

    camac_clear_modules(handle, modules, N_ELEMENTS(modules));
    crate_shutdown(handle);
    fclose(out);

    daq_print_summary(count, "Eventi salvati");

    return 0;
}
