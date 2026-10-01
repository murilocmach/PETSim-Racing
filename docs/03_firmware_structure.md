# Passo 3 — Estrutura do firmware (`wheel_controller`)

Este documento explica como o código em `firmware/wheel_controller/` está organizado,
pra quem for mexer nele depois (incluir pedais, célula de carga, FFB, etc.) entender o
que já existe e onde encaixar coisa nova.

## Por que não tem `.ioc` (projeto não foi gerado pelo CubeMX)

Na máquina usada pra montar isso só tinha o **STM32CubeIDE** instalado (sem o CubeMX
standalone), e a geração "headless" dele não estava disponível. Então, em vez de
depender da GUI, o código HAL foi escrito à mão e as fontes do driver foram copiadas
manualmente do pacote `STM32Cube_FW_F4_V1.28.3` (só os módulos realmente usados — ver
"Drivers" abaixo). Isso significa que **o projeto não tem um arquivo `.ioc`** — se você
abrir o Device Configuration Tool do CubeIDE nele, ele vai tentar criar um do zero. Pra
alterar pinos/periféricos, o caminho mais simples por enquanto é editar
`Src/main.c` / `Src/stm32f4xx_hal_msp.c` diretamente.

## Árvore de pastas

```
firmware/wheel_controller/
├── Inc/                      headers da aplicação + config do HAL
│   ├── main.h                defines de pino (ex.: LED_Pin/LED_GPIO_Port)
│   ├── stm32f4xx_hal_conf.h  quais módulos HAL estão habilitados, clock values
│   └── stm32f4xx_it.h        protótipos dos exception/interrupt handlers
├── Src/
│   ├── main.c                ponto de entrada: clock, init de periféricos, loop principal
│   ├── stm32f4xx_it.c        exception/interrupt handlers (SysTick, faults, etc.)
│   ├── stm32f4xx_hal_msp.c   init de baixo nível por periférico (pinos, clocks de GPIO/UART)
│   ├── syscalls.c/sysmem.c   stubs de libc (newlib) — já vinham do template original
│   └── (vazio ainda)         é aqui que entram os módulos de pedais/célula de carga/FFB
├── Drivers/                  HAL + CMSIS, só os arquivos realmente usados (ver abaixo)
├── Startup/startup_stm32f411ceux.s   vetor de interrupção + reset handler (ST, original)
├── STM32F411CEUX_FLASH.ld / _RAM.ld  linker scripts (ST, original)
├── Makefile                  build via `make` (funciona em Linux/macOS; no Windows ver build.ps1)
└── build.ps1                  build via PowerShell — usar essa no Windows (ver nota abaixo)
```

## Por que existe `build.ps1` além do `Makefile`

O `make.exe` que vem empacotado no STM32CubeIDE 2.2.0 falha ao tentar rodar o
`arm-none-eabi-gcc` como subprocesso nesse ambiente Windows/Git Bash (erro genérico,
sem mensagem do compilador — sintoma de incompatibilidade entre o `make` nativo do
Windows e a emulação de processos do MSYS/Git Bash). `build.ps1` faz a mesma coisa que
o `Makefile` (compila cada `.c`/`.s` e linka), só que chamando o compilador direto via
PowerShell, sem passar por essa camada problemática. **No Windows, use `build.ps1`:**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Isso gera `build/wheel_controller.elf/.hex/.bin`. Em Linux/macOS o `Makefile` normal
(`make all`) deve funcionar sem esse problema.

## `Drivers/` — por que só uma parte do HAL está aqui

Em vez de copiar o pacote `STM32F4xx_HAL_Driver` inteiro (±127MB, cheio de módulos que
esse projeto não usa — ADC, CAN, I2C, SPI, timers, etc.), só foram trazidos os módulos
que o firmware atual realmente referencia:

| Módulo | Pra quê |
|---|---|
| `RCC` (+ `_ex`) | configurar o clock (`SystemClock_Config` em `main.c`) |
| `GPIO` | LED onboard + pinos da UART |
| `UART` | comunicação com o ODrive |
| `CORTEX` | `HAL_NVIC_*`, `SysTick` |
| `PWR` (+ `_ex`) | voltage scaling, necessário pra rodar a 96MHz |
| `FLASH` (+ `_ex`/`_ramfunc`) | latência de flash, necessário por `HAL_RCC_ClockConfig` |
| `DMA` (+ `_ex`) | não usado ainda em modo DMA, mas `stm32f4xx_hal_uart.c` referencia os tipos/funções dele |
| `EXTI` | baseline que o CubeMX sempre inclui, não usado ainda |

**Quando adicionar pedais hall effect (ADC) e célula de carga**, vai precisar trazer o
módulo `ADC` (`stm32f4xx_hal_adc.c/.h` + `_ex`) da mesma forma — copiar de
`C:\Users\milol\STM32Cube\Repository\STM32Cube_FW_F4_V1.28.3\Drivers\STM32F4xx_HAL_Driver\`
e adicionar o `.c` na lista de fontes do `Makefile`/`build.ps1`.

## O que o firmware faz hoje (smoke test)

`main()` em `Src/main.c`:

1. `HAL_Init()` + `SystemClock_Config()` — sobe o clock pra **96MHz** a partir do
   cristal HSE de 25MHz da Blackpill (`PLLQ` já deixado em 48MHz, pronto pra quando
   entrar USB OTG FS pro HID).
2. `MX_GPIO_Init()` — configura o LED onboard (`PC13`, ativo em nível baixo).
3. `MX_USART1_UART_Init()` — `USART1` a 115200 8N1 em `PA9`(TX)/`PA10`(RX), falando o
   protocolo ASCII do ODrive (ver [docs/02](02_stm32_odrive_wiring.md)).
4. Loop principal, a cada ~500ms:
   - manda `r axis0.encoder.pos_estimate\n` pro ODrive (`ODrive_SendCommand`)
   - tenta ler uma linha de resposta com timeout (`ODrive_ReadLine`)
   - pisca o LED (heartbeat visual de que o firmware não travou)

Esse loop existe só pra validar a cadeia **clock → UART → build → gravação** de ponta a
ponta antes de entrar com a lógica de verdade. Ainda **não há**: leitura de pedais,
leitura de célula de carga, cálculo de FFB, nem HID USB — tudo isso entra depois que o
elo de comunicação com o ODrive estiver confirmado com hardware real (ver
[docs/01](01_odrive_bringup.md) e [docs/02](02_stm32_odrive_wiring.md)).

## Como compilar e gravar

```powershell
cd firmware/wheel_controller
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Gravar via ST-Link (SWD), usando o `STM32_Programmer_CLI` que vem com o CubeIDE:

```powershell
& "C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.500.202603051304\tools\bin\STM32_Programmer_CLI.exe" `
  -c port=SWD -w build\wheel_controller.hex -v --start
```

## Próximos módulos (onde vão entrar)

Conforme a arquitetura ([docs/ARCHITECTURE.md](ARCHITECTURE.md)), a ideia é que cada
responsabilidade vire seu próprio par de arquivos em `Src/`/`Inc/`, chamados a partir de
`main.c`, em vez de tudo empilhado num arquivo só:

- `odrive_link.c/.h` — hoje é só `ODrive_SendCommand`/`ODrive_ReadLine` dentro de
  `main.c`; deveria virar seu próprio módulo com um parser de respostas de verdade.
- `pedals.c/.h` — leitura ADC dos pedais hall effect.
- `load_cell.c/.h` — leitura da célula de carga (ADC direto ou um chip tipo HX711).
- `ffb.c/.h` — cálculo dos efeitos de force feedback a partir da posição do volante.
- `usb_hid.c/.h` — dispositivo HID composto (volante + pedais + FFB) exposto ao PC.
