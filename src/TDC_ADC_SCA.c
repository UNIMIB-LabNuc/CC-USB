//****************************************************************************
/*    TDC_ADC_SCA

Effettua n_misure blocchi di MEAS_TIME secondi. In ogni blocco:
  - lo scaler viene azzerato all'inizio;
  - gli eventi ADC+TDC sono acquisiti come in TDC_ADC (LAM di entrambi);
  - alla fine si legge lo scaler e si scrive una riga che inizia con EOM
    ("end of measure") seguita dai conteggi dei canali dello scaler.

Uso:
  ./bin/TDC_ADC_SCA                   -> DATA_DIR/TDC_ADC_SCA.txt, N_MEAS misure
  ./bin/TDC_ADC_SCA output.txt        -> DATA_DIR/output.txt
  ./bin/TDC_ADC_SCA output.txt 5      -> 5 misure

Formato del file:
  ADC[ADC_CHANNEL_MIN] ... ADC[ADC_CHANNEL_MAX] TDC[TDC_CHANNEL_MIN] ... TDC[TDC_CHANNEL_MAX]
  ...                                   (una riga per evento, -1 se non valido)
  EOM SCA[0] ... SCA[SCA_N_CHANNELS-1]  (fine della misura 1)
  ...                                   (eventi della misura 2)
  EOM SCA[0] ... SCA[SCA_N_CHANNELS-1]  (fine della misura 2)

Ctrl-C interrompe l'acquisizione: la misura in corso viene chiusa con la
sua riga EOM (sarà più corta di MEAS_TIME) e i dati restano nel file.
*/
//****************************************************************************

#define OUTPUT_FILE "TDC_ADC_SCA.txt"
#define N_MEAS 3          /* numero di misure */
#define MEAS_TIME 300     /* durata di ogni misura, secondi */

#define ADC_CHANNEL_MIN 3
#define ADC_CHANNEL_MAX 4
#define ADC_NCH (ADC_CHANNEL_MAX - ADC_CHANNEL_MIN + 1)

#define TDC_CHANNEL_MIN 0
#define TDC_CHANNEL_MAX 0
#define TDC_NCH (TDC_CHANNEL_MAX - TDC_CHANNEL_MIN + 1)

#define SCA_N_CHANNELS 7

#include <stdio.h>

#include "config.h"
#include "daq.h"
#include "camac_module.h"
#include "xxusb_helper.h"

/* l'ordine dei moduli è l'ordine delle colonne nel file */
static const camac_module modules[] = {
    { "ADC", ADC_SLOT, ADC_CHANNEL_MIN, ADC_CHANNEL_MAX },
    { "TDC", TDC_SLOT, TDC_CHANNEL_MIN, TDC_CHANNEL_MAX },
};

int main(int argc, char **argv)
{
    const char *filename;
    long n_meas;

    if (daq_parse_args(argc, argv, OUTPUT_FILE, "n_misure", N_MEAS, &filename, &n_meas) != 0) {
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

    daq_print_start(n_meas, "misure");

    long data[ADC_NCH + TDC_NCH];
    long sca[SCA_N_CHANNELS];
    long meas = 0;
    long total_events = 0;

    while (meas < n_meas && !daq_stop_requested()) {
        printf("=== Misura %ld/%ld (%d s) ===\n", meas + 1, n_meas, MEAS_TIME);

        scaler_clear(handle, SCA_SLOT, SCA_N_CHANNELS);
        camac_clear_modules(handle, modules, N_ELEMENTS(modules));

        const double start = daq_now();
        long count = 0;

        while (daq_now() - start < MEAS_TIME && !daq_stop_requested()) {
            if (camac_read_event(handle, modules, N_ELEMENTS(modules), data) == 0) {
                continue;
            }

            daq_write_line(out, NULL, data, ADC_NCH + TDC_NCH);
            ++count;
        }

        scaler_read(handle, SCA_SLOT, SCA_N_CHANNELS, sca);
        daq_write_line(out, "EOM", sca, SCA_N_CHANNELS);

        printf("Misura %ld: %ld eventi in %.1f s\n\n", meas + 1, count, daq_now() - start);

        total_events += count;
        ++meas;
    }

    camac_clear_modules(handle, modules, N_ELEMENTS(modules));
    crate_shutdown(handle);
    fclose(out);

    printf("Eventi totali: %ld\n", total_events);
    daq_print_summary(meas, "Misure salvate");

    return 0;
}
