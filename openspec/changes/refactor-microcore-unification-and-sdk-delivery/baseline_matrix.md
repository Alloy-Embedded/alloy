# Baseline Matrix (2026-02-10)

This baseline captures the current `ucore` smoke matrix after catalog normalization.

## Command Used

```bash
./ucore smoke --non-interactive --fail-fast
```

## Result

- Total combinations: `17`
- Passed: `17`
- Failed: `0`

## Supported Combinations

### `blink`
- `nucleo_f401re`
- `nucleo_f722ze`
- `nucleo_g071rb`
- `nucleo_g0b1re`
- `same70_xplained`

### `rtos/simple_tasks`
- `nucleo_f401re`
- `nucleo_f722ze`

### `api_tiers/simple_gpio_blink`
- `nucleo_f401re`
- `nucleo_f722ze`
- `nucleo_g071rb`
- `nucleo_g0b1re`
- `same70_xplained`

### `api_tiers/simple_gpio_button`
- `nucleo_f401re`
- `nucleo_f722ze`
- `nucleo_g071rb`
- `nucleo_g0b1re`
- `same70_xplained`
