//****************************************************************************
/*    Crate status

Inizializza il crate, legge i registri del CC-USB e stampa:
  - i moduli presenti (slot che rispondono a F(0)·A(0))
  - la risposta al test LAM F(8) di ogni slot
  - il registro LAM del CC-USB

Uso:
  ./bin/status
*/
//****************************************************************************

#include <stdio.h>

#include "daq.h"
#include "xxusb_helper.h"

int main(void)
{
    long data = 0;
    int q = 0, x = 0;

    daq_install_signal_handlers();

    usb_dev_handle *handle = xxusb_open_first_device();
    if (!handle) {
        return 1;
    }

    crate_init(handle);

    if (!daq_sleep(1)) {
        xxusb_close_device(handle);
        return 0;
    }

    /* CC-USB internal registers */
    CAMAC_read(handle, 25, 0, 0, &data, &q, &x);
    printf("CC-USB firmware ID: data=%ld Q=%d X=%d\n", data, q, x);

    CAMAC_read(handle, 25, 1, 0, &data, &q, &x);
    printf("CC-USB global mode: data=%ld Q=%d X=%d\n", data, q, x);

    xxusb_print_active_lams(handle);

    /* module presence */
    crate_scan_slots(handle);

    /* local LAM */
    printf("=== Slot scan with F(8) test LAM ===\n");
    for (int slot = 1; slot <= CRATE_N_SLOTS; slot++) {
        long dummy = 0;

        CAMAC_read(handle, slot, 0, 8, &dummy, &q, &x);
        if (x == 1) {
            printf("slot %2d: test LAM -> Q=%d X=%d\n", slot, q, x);
        }
    }

    xxusb_close_device(handle);

    return 0;
}
