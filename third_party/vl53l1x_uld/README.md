# ST VL53L1X Ultra Lite Driver 3.5.4

[STSW-IMG009](https://www.st.com/en/embedded-software/stsw-img009.html), revision 0000.
Core API source from the [RIOT-OS package mirror](https://github.com/gschorcht/riot_st_vl53l1x_uld_api/tree/76303d0588d346583c3479b5a22d33aadc8ee25d),
pinned to commit `76303d0588d346583c3479b5a22d33aadc8ee25d`.
The mirror's platform code and optional calibration API are not included.

Retain `LICENSE.txt` and `SLA0103.txt` (ST Clear BSD) when distributing the component or firmware.

## Integration notes

- Both unbounded ready loops in `VL53L1X_api.c` were replaced with
  `VL53L1_WaitForDataReady`: bounded to 500 ms and returns on errors.
- The project platform adapter under `src/devices/distance_sensor/` supplies
  big-endian transfers, initialized read buffers, operation deadlines, and latched
  errors to preserve failures discarded by some ST functions.
- Use the C++ `Vl53l1x` wrapper. Platform addresses are **7-bit**, fixed at **0x29**;
  only `I2cBus` shifts them for HAL. Do not use the vendor's `SetI2CAddress`,
  which expects an 8-bit address.
