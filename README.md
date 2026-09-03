# PETSim-Racing
Projeto de volante direct drive e pedais de hall effect e célula de carta, como projeto interno do PET EEL (UFSC)

## Status

Em desenvolvimento inicial (bring-up de hardware).

## Hardware

- ODrive ODESC v4.2 (controle de torque em malha fechada do motor)
- Motor de hoverboard (atuador direct-drive do volante)
- Encoder incremental (posição do volante)
- Fonte chaveada 24V
- STM32F411 "Blackpill" (cérebro: FFB, HID, pedais hall effect, célula de carga)

## Documentação

- [Arquitetura](docs/ARCHITECTURE.md)
- [Passo 1 — Bring-up do ODrive](docs/01_odrive_bringup.md)

## Estrutura

- `firmware/wheel_controller/` — projeto STM32CubeIDE (STM32F411)
- `docs/` — documentação técnica
- `hardware/` — esquemas, pinouts, BOM
