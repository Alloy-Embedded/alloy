# SAME70 Clock Test Example

Este exemplo permite testar diferentes configurações de clock no SAME70.

## Como Compilar

```bash
make same70-clock-build
```

## Como Gravar na Placa

```bash
make same70-clock-flash
```

Ou fazer tudo de uma vez:

```bash
make same70-clock-flash
```

(O target `same70-clock-flash` já compila antes de gravar automaticamente)

## Como Testar Diferentes Clocks

Edite o arquivo `main.cpp` e descomente a configuração que quer testar:

### Opção 1: RC 12MHz (Padrão - já está ativa)
```cpp
ClockConfig config = {
    .main_source = MainClockSource::InternalRC_12MHz,
    .mck_source = MasterClockSource::MainClock,
    .mck_prescaler = MasterClockPrescaler::DIV_1  // 12MHz MCK
};
```

### Opção 2: RC 4MHz (Baixo consumo)
Comente a Opção 1 e descomente:
```cpp
ClockConfig config = {
    .main_source = MainClockSource::InternalRC_4MHz,
    .mck_source = MasterClockSource::MainClock,
    .mck_prescaler = MasterClockPrescaler::DIV_1  // 4MHz MCK
};
```

### Opção 3: Crystal 12MHz (Externo, mais estável)
```cpp
ClockConfig config = {
    .main_source = MainClockSource::ExternalCrystal,
    .crystal_freq_hz = 12000000,
    .mck_source = MasterClockSource::MainClock,
    .mck_prescaler = MasterClockPrescaler::DIV_1  // 12MHz MCK
};
```

### Opção 4: Crystal + PLL @ 150MHz (Performance máximo)
```cpp
ClockConfig config = {
    .main_source = MainClockSource::ExternalCrystal,
    .crystal_freq_hz = 12000000,
    .plla = {24, 1},  // 12MHz * 25 / 1 = 300MHz
    .mck_source = MasterClockSource::PLLAClock,
    .mck_prescaler = MasterClockPrescaler::DIV_2  // 150MHz MCK
};
```

## Como Interpretar o LED

Após gravar, observe o LED na placa:

1. **LED piscando muito rápido** = Clock init falhou (erro de hardware)
2. **5 piscadas rápidas** = Clock init com sucesso
3. **Piscada normal** = Rodando na velocidade configurada

## Workflow Completo

```bash
# 1. Editar main.cpp e escolher configuração
code examples/same70_clock_test/main.cpp

# 2. Compilar e gravar
make same70-clock-flash

# 3. Observar LED na placa
```

## Debug com VS Code

Para debugar com F5:

1. Abra o workspace no VS Code
2. Edite `.vscode/launch.json` e mude o `program` para:
   ```json
   "program": "${workspaceFolder}/build-same70/examples/same70_clock_test/same70_clock_test"
   ```
3. Aperte F5 para debugar

## Troubleshooting

**Erro: ARM toolchain not found**
```bash
# Instale o toolchain ARM
./scripts/install-xpack-toolchain.sh
```

**Erro: OpenOCD not found**
```bash
brew install openocd
```

**LED não pisca / pisca muito rápido**
- Verifique se o crystal de 12MHz está presente (para configs com ExternalCrystal)
- Verifique se a configuração PLL é válida (PLLA entre 80-300 MHz)
- Use debug com F5 para ver onde está travando
