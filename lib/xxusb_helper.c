#include <stdio.h>

#include "xxusb_helper.h"

usb_dev_handle *xxusb_open_first_device(void)
{
    xxusb_device_type devices[128];

    int count = xxusb_devices_find(devices);
    printf("xxusb_open_first_device: found %d (crate) devices.\n", count);

    if (count <= 0) {
        fprintf(stderr, "xxusb_open_first_device: couldn't get list of XXUSB (crate) devices.\n");
        return NULL;
    }

    for (int i = 0; i < count; ++i) {
        usb_dev_handle *handle = xxusb_device_open(devices[i].usbdev);
        if (handle) {
            return handle;
        }
    }

    fprintf(stderr, "xxusb_open_first_device: found device(s), but could not open any of them.\n");
    return NULL;
}

void xxusb_close_device(usb_dev_handle *handle)
{
    if (handle) {
        xxusb_device_close(handle);
        printf("xxusb_close_device: done.\n");
    }
}

void crate_init(usb_dev_handle *handle)
{
    CAMAC_Z(handle);          /* initialize */
    CAMAC_C(handle);          /* clear */
    CAMAC_I(handle, 0);       /* inhibit OFF: acquisizione abilitata */
}

void crate_shutdown(usb_dev_handle *handle)
{
    printf("\nCleanup CAMAC...\n");

    CAMAC_I(handle, 1);       /* inhibit ON: blocca acquisizione */
    CAMAC_C(handle);
    CAMAC_Z(handle);

    xxusb_close_device(handle);
}

void crate_scan_slots(usb_dev_handle *handle)
{
    printf("\n=== Slot scan with F(0)·A(0) ===\n");

    for (int slot = 1; slot <= CRATE_N_SLOTS; slot++) {
        long data = 0;
        int q = 0, x = 0;

        CAMAC_read(handle, slot, 0, 0, &data, &q, &x);
        if (x == 1) {
            printf("Modulo in slot %2d -> data=%ld Q=%d X=%d\n", slot, data, q, x);
        }
    }
    printf("\n");
}

unsigned long xxusb_read_lam_register(usb_dev_handle *handle, int *q, int *x)
{
    long data = 0;
    int qloc = 0, xloc = 0;

    if (CAMAC_read(handle, 25, 10, 0, &data, &qloc, &xloc) < 0) {
        qloc = 0;
        xloc = 0;
        data = 0;
    }

    if (q) *q = qloc;
    if (x) *x = xloc;

    return ((unsigned long)data) & 0xFFFFFFUL;
}

void xxusb_print_active_lams(usb_dev_handle *handle)
{
    int q = 0, x = 0;
    unsigned long lam = xxusb_read_lam_register(handle, &q, &x);

    if (x != 1) {
        printf("LAM read failed (Q=%d X=%d)\n", q, x);
        return;
    }

    printf("LAM register: 0x%06lx | Active slots: ", lam);

    int found = 0;
    for (int slot = 1; slot <= CRATE_N_SLOTS; slot++) {
        if (lam & (1UL << (slot - 1))) {
            printf("%d ", slot);
            found = 1;
        }
    }

    if (!found) {
        printf("none");
    }

    printf("\n");
}
