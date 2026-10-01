# PETSim-Racing

Volante direct drive + pedais de hall effect + célula de carga, projeto interno do
**PET EEL (UFSC)**.

## Status

🚧 Em desenvolvimento — bring-up de hardware e primeiro firmware.

- [x] Arquitetura definida
- [x] ODrive ODESC v4.2 detectado e gravável via SWD *(esperando bring-up físico do motor+encoder)*
- [x] Firmware inicial no STM32F411: clock, UART, heartbeat — compilando e rodando em hardware real
- [ ] Elo de comunicação STM32 ↔ ODrive validado com o ODrive ligado de verdade
- [ ] Leitura dos pedais hall effect
- [ ] Leitura da célula de carga
- [ ] Cálculo de force feedback
- [ ] HID USB (volante + pedais + FFB como um dispositivo só)

## Arquitetura

```
              USB (HID: joystick + FFB)
                        │
                   [ PC / Jogo ]
                        │
        ┌───────────────┴────────────────┐
        │      STM32F411 "Blackpill"      │
        │  wheel_controller firmware      │
        │  • lê posição do encoder        │
        │    (via UART, consultando       │
        │    o ODrive)                    │
        │  • lê pedais hall effect (ADC)  │
        │  • lê célula de carga           │
        │  • calcula efeitos de FFB       │
        │  • expõe HID composto           │
        └───────────────┬────────────────┘
                        │ UART (fase 1) / CAN (fase 2)
                        │ comando: torque setpoint
                        │ leitura: posição do encoder
        ┌───────────────┴────────────────┐
        │       ODrive ODESC v4.2         │
        │  firmware ODrive padrão         │
        │  closed-loop torque control     │
        │  FOC do motor hoverboard        │
        └───────────────┬────────────────┘
                        │
                [ Motor hoverboard ]
                        │
                    [ Volante ]
```

O porquê dessa divisão (STM32 como "cérebro" único, ODrive só executando torque) está
detalhado em [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Hardware

| Componente | Papel |
|---|---|
| ODrive ODESC v4.2 | controle de torque em malha fechada do motor |
| Motor de hoverboard | atuador direct-drive do volante |
| Encoder MT6701 (magnético) | posição do volante — modo ABZ, 4096 CPR |
| Fonte chaveada 24V | alimentação do ODESC/motor |
| STM32F411 "Blackpill" | cérebro: FFB, HID, pedais hall effect, célula de carga |
| ST-Link V2 | gravação/depuração do STM32 via SWD |

## Documentação

| # | Documento | Conteúdo |
|---|---|---|
| — | [Arquitetura](docs/ARCHITECTURE.md) | decisão de design, divisão STM32 ↔ ODrive, protocolo |
| 1 | [Bring-up do ODrive](docs/01_odrive_bringup.md) | calibrar motor + encoder via `odrivetool`, sem STM32 |
| 2 | [Ligação física STM32 ↔ ODrive](docs/02_stm32_odrive_wiring.md) | wiring UART, pinos, config ASCII protocol |
| 3 | [Estrutura do firmware](docs/03_firmware_structure.md) | organização do código, build, drivers usados, próximos módulos |

## Estrutura do repositório

```
firmware/wheel_controller/   firmware do STM32F411 (ver docs/03)
docs/                        documentação de arquitetura, bring-up, protocolo, firmware
hardware/                    esquemas, pinouts, BOM (a preencher)
```

## Build rápido (firmware)

```powershell
cd firmware/wheel_controller
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Detalhes de build/gravação via ST-Link em [docs/03_firmware_structure.md](docs/03_firmware_structure.md#como-compilar-e-gravar).
