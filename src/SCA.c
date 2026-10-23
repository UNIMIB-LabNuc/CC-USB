//****************************************************************************
/*    SCALER

Effettua n_misure misure di TIME secondi. In ogni misura lo scaler
(slot SCA_SLOT in config.h) viene azzerato all'inizio e letto ogni
TIME_STAMP secondi: i conteggi sono cumulativi dall'inizio della misura.

Uso:
  ./bin/SCA                     -> DATA_DIR/SCA.txt, N_MEAS misure
  ./bin/SCA output.txt          -> DATA_DIR/output.txt
  ./bin/SCA output.txt 5        -> 5 misure, tutte nello stesso file

Formato del file, per ogni misura:
  SCA[0] ... SCA[N_CHANNELS-1]     (una riga ogni TIME_STAMP secondi)
  ...
  <riga vuota>
  Tempo di acquisizione: T secondi

Ctrl-C interrompe l'acquisizione: la misura in corso viene chiusa con
il tempo effettivamente acquisito (multiplo di TIME_STAMP).
*/
//****************************************************************************

#define OUTPUT_FILE "SCA.txt"
#define N_MEAS 1          /* numero di misure */
#define TIME 180          /* durata di ogni misura, secondi */
#define TIME_STAMP 60     /* intervallo tra due letture, secondi */
#define N_CHANNELS 7

#include <stdio.h>

#include "config.h"
#include "daq.h"
#include "camac_module.h"
#include "xxusb_helper.h"

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

    daq_print_start(n_meas, "misure");

    long data[N_CHANNELS];
    long meas = 0;

    while (meas < n_meas && !daq_stop_requested()) {
        printf("=== Misura %ld/%ld (%d s) ===\n", meas + 1, n_meas, TIME);

        scaler_clear(handle, SCA_SLOT, N_CHANNELS);

        int elapsed = 0;
        while (elapsed < TIME && daq_sleep(TIME_STAMP)) {
            elapsed += TIME_STAMP;

            scaler_read(handle, SCA_SLOT, N_CHANNELS, data);

            printf("remaining time: %06d   counts: ", TIME - elapsed);
            daq_write_line(out, NULL, data, N_CHANNELS);
        }

        /* interrotta prima della prima lettura: niente da salvare */
        if (elapsed == 0) {
            break;
        }

        fprintf(out, "\nTempo di acquisizione: %d secondi\n", elapsed);
        fflush(out);
        printf("Tempo di acquisizione: %d secondi\n\n", elapsed);

        ++meas;
    }

    crate_shutdown(handle);
    fclose(out);

    daq_print_summary(meas, "Misure salvate");

    return 0;
}
