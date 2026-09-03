# Arquitetura — PETSim-Racing

## Visão geral

O volante direct-drive é dividido em dois "cérebros" com responsabilidades bem separadas:

```
              USB (HID: joystick + FFB)
                        |
                     [ PC / Jogo ]
                        |
        +---------------+---------------+
        |     STM32F411 "wheel_controller"|
        |  - Lê encoder do volante         |
        |    (via UART, consultando ODrive)|
        |  - Lê pedais hall effect (ADC)   |
        |  - Lê célula de carga (ADC/HX711)|
        |  - Calcula efeitos de FFB        |
        |  - Expõe HID composto (volante   |
        |    + pedais + eixo de força)     |
        +---------------+---------------+
                        | UART (fase 1) / CAN (fase 2)
                        | comando: torque setpoint
                        | leitura: posição do encoder
        +---------------+---------------+
        |         ODrive ODESC v4.2       |
        |  - Firmware ODrive padrão        |
        |  - Closed-loop torque control    |
        |  - FOC do motor hoverboard       |
        |  - Lê o encoder (conectado nele) |
        +---------------+---------------+
                        |
                [ Motor hoverboard ]
                        |
                    [ Volante ]
```

## Por que essa divisão (e não o firmware pronto do FFBeast)

O FFBeast "padrão" substitui o firmware da própria placa ODrive/ODESC (ela roda um
STM32F405) pelo firmware deles, e a placa vira o dispositivo USB HID sozinha.

Aqui a decisão foi diferente porque os pedais de hall effect e a célula de carga também
vão ficar no STM32F411 (Blackpill). Faz mais sentido esse STM32 ser o único dispositivo
USB HID (volante + pedais + força), o único lugar onde mora o algoritmo de FFB, e o
ODrive ficar restrito a fazer bem uma coisa: controle de torque em malha fechada do
motor. Vantagens:

- Um único HID composto — o jogo enxerga um controle só, sem conflito de eixos entre
  dois dispositivos.
- Sensor fusion (volante + pedais + célula de carga) centralizado num lugar só.
- Conteúdo técnico real pro projeto do PET: protocolo de comunicação, malha de
  controle, HID, em vez de só flashar firmware de terceiro.

Trade-off: mais firmware pra escrever (o ODrive já resolve o controle de motor sozinho;
aqui ainda precisamos do elo de comunicação STM32↔ODrive).

## Protocolo STM32 ↔ ODrive

- **Fase 1 (bring-up / MVP):** protocolo ASCII do ODrive sobre **UART**. Simples de
  depurar (dá pra digitar comando na mão via terminal serial), latência ok para
  primeiros testes. Comandos principais:
  - `w axis0.controller.input_torque <valor>` — manda setpoint de torque
  - `r axis0.encoder.pos_estimate` — lê posição do encoder (em voltas)
  - `r axis0.encoder.vel_estimate` — lê velocidade
- **Fase 2 (produção):** migrar pra **CAN** (ODrive CAN Simple Protocol) quando a malha
  de FFB precisar de mais taxa de atualização / menos jitter do que UART ASCII entrega
  confortavelmente. O ODESC v4.2 já expõe CAN, então a migração é só de firmware.

## Encoder — uma única fonte

O encoder fica cabeado **somente no ODrive** (porta encoder0), que já precisa dele pra
comutação (FOC) e pra malha de controle de torque/posição. O STM32 **não lê o encoder
diretamente** — ele consulta a posição via UART (`r axis0.encoder.pos_estimate`). Isso
evita ter que "espelhar" o sinal do encoder pra dois consumidores (ODrive + STM32), o
que é fonte comum de problema de integridade de sinal.

Se no futuro a taxa de atualização via UART for insuficiente pro FFB, a alternativa é
ler o encoder em paralelo também no STM32 (linhas A/B em fanout) — mas isso fica pra
depois, só se for necessário.

## Estrutura do repositório

```
firmware/wheel_controller/   projeto STM32CubeIDE (STM32F411CEUx / Blackpill)
docs/                        documentação de arquitetura, bring-up, protocolo
hardware/                    esquemas, pinouts, BOM (a preencher)
```
