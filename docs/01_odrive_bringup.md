# Passo 1 — Bring-up do ODrive ODESC v4.2 (sem STM32 ainda)

Objetivo: validar que motor de hoverboard + encoder + ODESC v4.2 funcionam juntos em
malha fechada, **antes** de meter o STM32 no meio. Isso isola problemas: se algo não
funcionar depois com o STM32 na jogada, você já sabe que não é o motor/encoder/ODrive.

Tudo aqui é feito com `odrivetool` (Python) direto do PC via USB — já está instalado
(`odrive 0.5.4`).

## 1. Ligações físicas

**IMPORTANTE (aviso do próprio FFBeast, vale igual aqui): nunca confie em cor de fio.
Conecte sempre pelo nome/label serigrafado na placa.**

- **Alimentação:** fonte chaveada 24V → entrada DC da ODESC (`DC+` / `GND` — confira a
  polaridade duas vezes antes de ligar; ODrive não gosta de reversão de polaridade).
- **Motor:** as 3 fases do motor de hoverboard → conector `M0` (as 3 fases entre si
  podem trocar de ordem sem problema — só inverte o sentido de giro, ajusta depois).
- **Encoder:** → porta `ENCODER0` / `AUX` da ODESC, respeitando os pinos `A`, `B`,
  (`Z` opcional, o ODrive não usa índice por padrão), alimentação (`5V` ou `3.3V`,
  confira o que o seu encoder pede) e `GND`.
- **USB:** ODESC → PC (só serve pra configurar/depurar; **não alimenta a placa**,
  a fonte de 24V tem que estar ligada pra placa ligar).

## 2. Descobrir os parâmetros do seu hardware

Antes de configurar, anota:

- **Pole pairs do motor**: motores de hoverboard tipicamente têm **15 pares de polos**
  (30 imãs) — é o valor mais comum, mas confirme contando os imãs do seu motor
  (número de imãs ÷ 2) se quiser ter certeza.
- **CPR do encoder**: com o **MT6701 em modo ABZ (incremental)**, CPR = **4096**
  (confirmado na documentação do FFBeast). O MT6701 é um encoder magnético multi-modo
  (ABZ, PWM, SSI, I2C, UVW) — confirme no seu breakout específico que ele está
  configurado/travado em modo **ABZ**, já que é esse o único modo que o ODrive lê
  nativamente como encoder incremental.
- **Corrente máxima** que você quer permitir no primeiro teste — comece **conservador**
  (ex.: 10-15A) até confirmar que tudo está saudável, depois sobe.

## 3. Configuração inicial via odrivetool

Abra um terminal e rode:

```bash
odrivetool
```

Ele conecta automaticamente na ODESC via USB. Dentro do shell interativo do
`odrivetool`, rode (ajustando os valores marcados com `# AJUSTAR`):

```python
# zera config anterior, se a placa já tiver sido usada antes
odrv0.erase_configuration()
# a placa reseta e desconecta — rode `odrivetool` de novo pra reconectar

# --- motor ---
odrv0.axis0.motor.config.pole_pairs = 15                # AJUSTAR se confirmou outro valor
odrv0.axis0.motor.config.motor_type = MOTOR_TYPE_HIGH_CURRENT
odrv0.axis0.motor.config.calibration_current = 8         # corrente do teste de calibração, conservador
odrv0.axis0.motor.config.current_lim = 15                # AJUSTAR — limite de corrente do teste inicial
odrv0.axis0.motor.config.requested_current_range = 25

# --- encoder ---
odrv0.axis0.encoder.config.cpr = 4096                      # MT6701 em modo ABZ
odrv0.axis0.encoder.config.mode = ENCODER_MODE_INCREMENTAL

# --- limites de segurança da fonte ---
odrv0.config.dc_bus_undervoltage_trip_level = 8
odrv0.config.dc_bus_overvoltage_trip_level = 26           # folga acima dos 24V nominais da fonte

odrv0.save_configuration()
# a placa reseta de novo — reconecte com `odrivetool`
```

## 4. Calibração

Com o motor **livre pra girar** (sem o volante montado ainda, por segurança):

```python
odrv0.axis0.requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE
```

O motor vai fazer um pequeno movimento de calibração. Depois, confira se não deu erro:

```python
odrv0.axis0.error            # deve ser 0 (AXIS_ERROR_NONE)
odrv0.axis0.motor.error      # deve ser 0
odrv0.axis0.encoder.error    # deve ser 0
```

Se deu tudo certo, marca a calibração como boa e salva (assim não precisa recalibrar
toda vez que ligar):

```python
odrv0.axis0.motor.config.pre_calibrated = True
odrv0.axis0.encoder.config.pre_calibrated = True
odrv0.save_configuration()
```

## 5. Primeiro teste em malha fechada (velocidade, não torque ainda)

Testar em **velocidade** primeiro é mais seguro/previsível que torque puro pra validar
que a malha fecha certo:

```python
odrv0.axis0.controller.config.control_mode = CONTROL_MODE_VELOCITY_CONTROL
odrv0.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
odrv0.axis0.controller.input_vel = 1     # 1 volta/s — bem devagar
# ... confirma que gira suave e para:
odrv0.axis0.controller.input_vel = 0
odrv0.axis0.requested_state = AXIS_STATE_IDLE
```

Se girou suave, sem vibração/ruído estranho e sem trip de erro: motor + encoder + ODrive
estão validados.

## 6. Teste em torque (o modo que o STM32 vai usar de verdade)

```python
odrv0.axis0.controller.config.control_mode = CONTROL_MODE_TORQUE_CONTROL
odrv0.axis0.requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL
odrv0.axis0.controller.input_torque = 0.1   # Nm, bem baixo pra teste
odrv0.axis0.controller.input_torque = 0.0
odrv0.axis0.requested_state = AXIS_STATE_IDLE
```

## Próximo passo

Com isso validado, o próximo passo é o firmware do STM32F411 (`firmware/wheel_controller`):
UART pro ODrive replicando esses mesmos comandos via protocolo ASCII, começando por
"ler posição do encoder" e "mandar torque setpoint fixo" — sem pedais/FFB ainda.

> Nota: os valores exatos de comando (`w axis0...`, nomes de estado) valem pra firmware
> ODrive da árvore 0.5.x, que é o que o `odrivetool 0.5.4` instalado espera. Se a placa
> vier com firmware diferente, o `odrivetool` avisa incompatibilidade de versão ao
> conectar — nesse caso me avisa que ajustamos os comandos.
