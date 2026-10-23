#!/usr/bin/env python3

import subprocess
import sys
import time
import datetime
from pathlib import Path

CAEN_SCRIPT = "/home/labstudent/Utils/drive_CAEN_N1470.py"
# eseguibile SCA del pacchetto (bin/SCA), ovunque sia installato
SCA_EXEC = str(Path(__file__).resolve().parent.parent / "bin" / "SCA")

# lista tensioni
VOLTAGES = [2050, 2050, 2050, 2050, 2050]

# tempo di stabilizzazione (secondi)
SETTLE_TIME = 3


def run_cmd(cmd):
    print(f"\n>>> Running: {' '.join(cmd)}\n")
    result = subprocess.run(cmd)

    if result.returncode != 0:
        print(f"Error: command failed with code {result.returncode}")
        sys.exit(1)


def main():
    try:
        for voltage in VOLTAGES:
            print("\n" + "=" * 50)
            print(f"Setting voltage to {voltage} V")
            print("=" * 50)

            # Set voltage and turn channel on
            run_cmd([
                CAEN_SCRIPT,
                "-c", str(1),
                "--vset", str(voltage),
                "--on",
                "--yes"
            ])

            run_cmd([
                CAEN_SCRIPT,
                "-c", str(2),
                "--vset", str(voltage),
                "--on",
                "--yes"
            ])

            print(f"Waiting {SETTLE_TIME} s for stabilization...")
            time.sleep(SETTLE_TIME)
            
            # Timestamped output filename
            ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            outfile = f"SCA_th15_{voltage}V_{ts}.txt"

            # Run scaler acquisition
            run_cmd([
                SCA_EXEC,
                outfile
            ])

        print("\nScan completed.")

    finally:
        print("\nTurning HV off...")
        run_cmd([
            CAEN_SCRIPT,
            "-c", str(1),
            "--off",
            "--yes"
        ])
        print("\nTurning HV off...")
        run_cmd([
            CAEN_SCRIPT,
            "-c", str(2),
            "--off",
            "--yes"
        ])        


if __name__ == "__main__":
    main()
