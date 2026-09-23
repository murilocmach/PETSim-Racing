# Passo 2 — Ligação física STM32 ↔ ODrive

Pré-requisito: [Passo 1 — bring-up do ODrive](01_odrive_bringup.md) já validado (motor +
encoder funcionando em malha fechada via `odrivetool`).

## Por que UART e não CAN, por enquanto

A ODESC v4.2 já traz uma **UART_A habilitada por padrão de fábrica**, nos pinos `GPIO1`
(TX) e `GPIO2` (RX) do conector AUX, 115200 baud — zero configuração extra de
hardware. CAN dá mais robustez/velocidade mas exige transceiver CAN e mais setup dos
dois lados. Fica como upgrade de fase 2 (ver [ARCHITECTURE.md](ARCHITECTURE.md)).

## Ligação de fios

| ODESC v4.2 (conector AUX) | STM32F411 Blackpill | Função |
|---|---|---|
| `GPIO1` (UART_A.TX) | `PA10` (USART1_RX) | ODrive fala, STM32 escuta |
| `GPIO2` (UART_A.RX) | `PA9` (USART1_TX) | STM32 fala, ODrive escuta |
| `GND` (qualquer pino GND do AUX) | `GND` | referência comum — **obrigatório** |

Regras importantes:

- **TX de um lado sempre no RX do outro** (cruzado), nunca TX-TX.
- **Não ligar as alimentações das duas placas entre si.** A ODESC é alimentada pela
  fonte 24V; o STM32F411 é alimentado pelo próprio cabo USB no PC. Só os 3 sinais
  acima (TX, RX, GND) conectam as duas placas.
- Nível lógico: ambos os lados trabalham em **3.3V**, e os GPIOs do ODrive ainda são
  tolerantes a 5V — não precisa de level shifter.
- Como sempre: **confira o nome serigrafado no conector da sua ODESC**, não a posição
  ou cor do fio — placas clone podem variar levemente o layout físico do AUX, mas os
  nomes `GPIO1`/`GPIO2`/`GND` devem estar identificados na serigrafia.

`PA9`/`PA10` foram escolhidos por ser a `USART1` do STM32F411, livre por padrão no
Blackpill (não conflita com USB, que usa `PA11`/`PA12`).

## Configuração no lado do ODrive

A UART_A já vem habilitada, mas o protocolo precisa estar em modo ASCII (não o
protocolo nativo/Fibre) pra ser fácil de falar a partir do firmware do STM32:

```python
odrv0.config.enable_uart_a = True
odrv0.config.uart_a_baudrate = 115200
odrv0.config.uart0_protocol = STREAM_PROTOCOL_TYPE_ASCII_AND_STDOUT
odrv0.save_configuration()
odrv0.reboot()
```

## Teste manual antes de escrever firmware

Antes de programar o STM32, vale confirmar a ligação com um adaptador USB-serial
(FTDI/CP2102) direto nos mesmos pinos GPIO1/GPIO2/GND da ODESC, abrindo um terminal
serial (115200 8N1) e digitando comandos do protocolo ASCII do ODrive, ex.:

```
r axis0.encoder.pos_estimate
w axis0.controller.input_torque 0.1
```

Se responder com a posição/aceitar o comando, a UART_A está saudável e o próximo passo
é só o STM32 falar o mesmo protocolo pela `USART1`.

## Próximo passo

Firmware mínimo no STM32 (`firmware/wheel_controller`): configurar `USART1` a
115200 8N1, mandar `r axis0.encoder.pos_estimate` periodicamente e um
`w axis0.controller.input_torque <valor>` fixo baixo, validando o elo de comunicação
antes de entrar com pedais, célula de carga e cálculo de FFB.
