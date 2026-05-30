# TC Meat Thermometer

This project uses an ESP32 dual-slope ADC state machine to measure the selected sensor path and convert the result into an input voltage and temperature reading.

## Core Dual-Slope Flow

The converter runs through these phases:

1. `RESET`
   - The integrator reset switch is closed with `S3`.
   - `S0` is switched at the very beginning of `RESET`.
   - The firmware alternates the sensor selection so that 10 conversions use TC, then 1 conversion uses LM35.

2. `WAIT_RESET`
   - The reset switch stays closed for `TRESET_US`.
   - This gives the auto-zero/reset path time to settle.

3. `WAIT_DELAY`
   - The reset switch is released.
   - The firmware waits `TDELAY_US` before starting integration.

4. `INTEGRATE`
   - `S1` and `S2` are driven into the integrate configuration.
   - The hardware timer is zeroed at the start of integration.
   - A one-shot timer alarm is armed for `TREF_US`.

5. `DEINTEGRATE`
   - When the timer alarm fires, the MUX switches to the de-integration path.
   - The comparator ISR timestamps the exact trip time using `timerRead(adcTimer)`.
   - The measured time is used to compute the final input voltage.

6. `DONE`
   - The result is handed off to the main task.
   - Core 1 prints the final voltage while Core 0 continues the state machine pacing.

## Timing Model

- Timer resolution: `1 us`
- `TRESET_US`: reset settling time before the conversion starts
- `TDELAY_US`: short pause after reset release
- `TREF_US`: fixed integration window
- `MAX_TIMEOUT_US`: watchdog-style fallback timeout during de-integration

The timer is free-running and the conversion window is started by writing the timer to zero at the beginning of integration.

## Sensor Selection

The firmware cycles the input select pin `S0` like this:

- 10 cycles on TC
- 1 cycle on LM35
- repeat

This lets the downstream code distinguish ambient-temperature reads from hot-junction reads using the `lastWasLM35` flag.

## Output Path

The real-time task copies shared conversion values under a critical section and then the main loop performs the blocking serial output.

The printed output is currently kept minimal and reports the final `Vinput`.

## Important Implementation Notes

- No blocking `Serial.print()` calls remain inside the dual-slope state machine critical section.
- ISR-critical work is limited to timestamp capture and state transitions.
- Shared state is protected with task-safe critical sections in task context and ISR-safe critical sections in interrupt context.
- `Vin` stores the final calculated input voltage so it can be consumed externally.

## Files of Interest

- `src/DualSlope.h` - dual-slope state machine, timer handling, sensor cycling, and conversion math
- `src/main.cpp` - task scheduling, pacing, and serial output
- `src/HAL.cpp` - hardware initialization helpers
- `src/MAX31855.h` - MAX31855 setup and reads

## Build

```bash
platformio run
```

## Upload

```bash
platformio run --target upload
```
