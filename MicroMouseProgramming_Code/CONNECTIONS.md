# STM32L476VE — MicroMouse Peripheral Connections

MCU: **STM32L476VETx** | 3.0 V supply | HSE external oscillator on PH0/PH1

---

## I2C Buses

### I2C1 — `PB8` (SCL) / `PB9` (SDA)
Fast mode, rise/fall time 100 ns

| Device | I2C Address | Notes |
|---|---|---|
| INA219 (power monitor) | `0x41` (7-bit) | A1=LOW, A0=HIGH |
| VL53L0X Back ToF | `0x72` (reassigned) | XSHUT6 on PC11 |

### I2C2 — `PB10` (SCL) / `PB11` (SDA)
Fast mode, rise/fall time 10 ns

| Device | I2C Address | Notes |
|---|---|---|
| LSM6DS3 IMU | default `0x6A` | INT on PB5 |
| SSD1306 OLED | `0x3C` | 128×64 display |
| VL53L0X Left ToF | `0x54` (reassigned) | XSHUT1 on PE8 |
| VL53L0X Front-Left ToF | `0x5A` (reassigned) | XSHUT4 on PE10 |
| VL53L0X Centre ToF | `0x60` (reassigned) | XSHUT2 on PE15 |
| VL53L0X Front-Right ToF | `0x66` (reassigned) | XSHUT5 on PE2 |
| VL53L0X Right ToF | `0x6C` (reassigned) | XSHUT3 on PE3 |

> All VL53L0X sensors boot at default address `0x52` and are reassigned sequentially during `initTOFs()` by toggling XSHUT pins one at a time.

---

## USART1 — `PB6` (TX) / `PB7` (RX)

| Parameter | Value |
|---|---|
| Baud rate | 1 843 200 |
| Mode | Asynchronous |
| DMA TX | DMA2 Channel 6 |
| DMA RX | DMA2 Channel 7 (circular) |

Used for the MicroMouse Interface (Python/Flask web app).

---

## Timers

### TIM1 — IR LED PWM (4 channels)
Prescaler: 800-1 | Period: 1000-1 → ~100 Hz at 80 MHz

| Channel | Pin | Label | Notes |
|---|---|---|---|
| CH1 | PE9 | LED_MOT_LS | Left motor-side IR LED |
| CH2 | PE11 | LED_DOWN_LS | Left downward IR LED |
| CH3 | PE13 | LED_MOT_RS | Right motor-side IR LED |
| CH4 | PE14 | LED_DOWN_RS | Right downward IR LED (polarity LOW) |

### TIM3 — Left Motor PWM
Prescaler: 8000-1 | Period: 100-1 | Auto-reload preload enabled

| Channel | Pin | Label | Direction |
|---|---|---|---|
| CH3 | PC8 | MOT_LEFT_FWD | Forward |
| CH4 | PC9 | MOT_LEFT_BWD | Backward (polarity LOW) |

### TIM4 — Right Motor PWM
Prescaler: 8000-1 | Period: 100-1 | Auto-reload preload enabled

| Channel | Pin | Label | Direction |
|---|---|---|---|
| CH1 | PD12 | MOT_RIGHT_FWD | Forward (polarity LOW) |
| CH2 | PD13 | MOT_RIGHT_BWD | Backward (polarity LOW) |

### TIM5 — General Purpose / Encoder
Auto-reload preload enabled. Prescaler: 1, Period: 1.

### TIM6 — HAL Timebase (system tick)
IRQ priority 15 (lowest). Not user-accessible.

### TIM7 — General Purpose / Scheduling
Auto-reload preload enabled. Prescaler: 1.

---

## ADC1 — 5-channel, DMA circular (DMA1 Channel 1)

Oversampling: 256× ratio, 4-bit right shift. Sampling time: 640.5 cycles per channel.

| Rank | Channel | Pin | Label | Measures |
|---|---|---|---|---|
| 1 | VBAT (CH18) | internal | — | Battery voltage (÷4 internal divider) |
| 2 | CH8 | PA3 | ADC_MOT_RS | Right motor current sense |
| 3 | CH13 | PC4 | ADC_DOWN_RS | Right downward IR sensor |
| 4 | CH14 | PC5 | ADC_DOWN_LS | Left downward IR sensor |
| 5 | CH15 | PB0 | ADC_MOT_LS | Left motor current sense |

---

## GPIO

### Outputs

| Pin | Label | Function |
|---|---|---|
| PD7 | MOTOR_EN | Motor driver enable (H-bridge) |
| PE8 | XSHUT1 | VL53L0X Left ToF shutdown |
| PE15 | XSHUT2 | VL53L0X Centre ToF shutdown |
| PE3 | XSHUT3 | VL53L0X Right ToF shutdown |
| PE10 | XSHUT4 | VL53L0X Front-Left ToF shutdown |
| PE2 | XSHUT5 | VL53L0X Front-Right ToF shutdown |
| PC11 | XSHUT6 | VL53L0X Back ToF shutdown (I2C1) |
| PB3 | CTRL_LEDS | LED control (WS2812 / NeoPixel data) |
| PC13 | LED0 | Status LED 0 |
| PC14 | LED1 | Status LED 1 |
| PC15 | LED2 | Status LED 2 |

### Inputs

| Pin | Label | Pull | Function |
|---|---|---|---|
| PE6 | SW1 | Pull-up | User button 1 |
| PB2 | SW2 | Pull-up | User button 2 |
| PB5 | IMU_INT | — | LSM6DS3 interrupt output |

---

## Debug / Programming

| Pin | Function |
|---|---|
| PA13 (SWDIO) | SWD data |
| PA14 (SWCLK) | SWD clock |

---

## Clock

| Source | Value |
|---|---|
| HSE | External oscillator on PH0/PH1 |
| System clock | 80 MHz (configured via PLL) |
| VDD | 3.0 V |

---

## Timing Budget — `while(1)` Loop Analysis

I2C timing model at **400 kHz Fast mode** (2.5 µs/bit):

| Operation | Formula | Time |
|---|---|---|
| `HAL_I2C_Mem_Read(1 byte)` | (31 + 9×1) × 2.5 µs | ~100 µs |
| `HAL_I2C_Mem_Read(2 bytes)` | (31 + 9×2) × 2.5 µs | ~123 µs |
| `HAL_I2C_Mem_Read(12 bytes)` | (31 + 9×12) × 2.5 µs | ~347 µs |
| `HAL_I2C_Mem_Write(1 byte)` | (20 + 9×1) × 2.5 µs | ~73 µs |
| `HAL_I2C_Master_Transmit(2 bytes)` | (11 + 9×2) × 2.5 µs | ~73 µs |
| `HAL_I2C_Master_Transmit(129 bytes)` | (11 + 9×129) × 2.5 µs | ~2930 µs |

> Formula: write phase = START + addr(9) + reg(9) bits; read phase = RESTART + addr(9) + n×data(9) bits + STOP.

---

### `refreshADCs()`
Only copies DMA-captured values from `ADCs[]` to named variables. No I2C.
**~1 µs** (negligible)

---

### `refreshScreen()` — SSD1306, I2C2
`SSD1306_UpdateScreen()` sends the full 128×64 framebuffer in 8 horizontal pages:

| Per page | Count | Time |
|---|---|---|
| `HAL_I2C_Master_Transmit(2 B)` — page/col commands | 3 | 3 × 73 µs = 219 µs |
| `HAL_I2C_Master_Transmit(129 B)` — 128 pixel bytes | 1 | 2930 µs |
| **Page subtotal** | | **~3149 µs** |
| **× 8 pages** | | **~25.2 ms** |

**~25.2 ms** — the dominant cost in the loop.
Returns immediately if `SSD1306.Initialized == false`.

---

### `refreshLEDs()` / `refreshSWValues()` / `refreshMotors()`
GPIO register and timer CCR writes only.
**~1–5 µs each** (negligible)

---

### `refreshTOFValues()` — VL53L0X, I2C2
Currently **3 sensors active** (front_left, centre, front_right); left, right, and back calls are commented out.

Per `getVL53L0()` call — assuming measurement already complete in continuous mode:

| Step | Call | Time |
|---|---|---|
| Poll interrupt status | `readReg(1 B)` × 1 | ~100 µs |
| Burst-read result | `readMulti(12 B)` | ~347 µs |
| Clear interrupt | `writeReg(1 B)` | ~73 µs |
| **Per sensor** | | **~520 µs** |
| **× 3 active sensors** | | **~1560 µs ≈ 1.6 ms** |

Worst case (measurement not ready, 5 ms `setTimeout`): up to **5.5 ms per sensor → ~16.5 ms** for all 3.

> **VL53L0X timing budget note:** `initVL53L0()` calls `setMeasurementTimingBudget(50*(FinalRange−PreRange)*1000UL)`.  
> With `PreRange=18` and `FinalRange=14` (both `uint8_t`), the C expression evaluates to a wrapped large value, causing `setMeasurementTimingBudget` to return `false`.  
> The sensor therefore retains the default budget loaded earlier by `initVL53L0X()`: **~33 ms per measurement (~30 Hz)**.  
> With the ~28 ms main loop, measurements will usually complete by the time they are polled.

---

### `refreshIMUValues()` — ICM42605, I2C2
7 individual `readWord()` calls (`HAL_I2C_Mem_Read`, 2 bytes each):

| Data | Reads |
|---|---|
| Accel X, Y, Z | 3 |
| Gyro X, Y, Z | 3 |
| Temperature | 1 |
| **Total** | **7 × ~123 µs = ~861 µs ≈ 0.86 ms** |

PWM timer stop/start around the reads: ~20 µs additional.
**~880 µs ≈ 0.9 ms**

---

### `refreshINA219Values()` — INA219, I2C1
4 calls to `Read16()` (`HAL_I2C_Mem_Read`, 2 bytes each, on **I2C1**):

| Read | Register |
|---|---|
| `INA219_ReadBusVoltage` | `INA219_REG_BUSVOLTAGE` |
| `INA219_ReadShuntVolage` | `INA219_REG_SHUNTVOLTAGE` |
| `INA219_ReadCurrent` | `INA219_REG_CURRENT` |
| `INA219_ReadPower` | `INA219_REG_POWER` |
| **Total** | **4 × ~123 µs = ~492 µs ≈ 0.5 ms** |

Runs on I2C1 independently of I2C2 (no bus contention with ToF/IMU/OLED).

---

### `refreshLoggedData()` — Flash logging (conditional)
Rate-gated. Returns immediately when logging is not active.
**~1 µs** when idle.

---

### `sendToSimulink()` — UART TX *(currently commented out)*
171-byte packet at 1 843 200 baud (8N1):

```
171 bytes × 10 bits = 1710 bits
1710 / 1 843 200 ≈ 0.93 ms
```

Uses `HAL_UART_Transmit` with `HAL_MAX_DELAY` — blocks until complete.
**~0.93 ms** if re-enabled.

---

### Loop Rate Summary

| Function | Bus | Normal time |
|---|---|---|
| `refreshADCs()` | DMA | ~1 µs |
| `refreshScreen()` | I2C2 | **~25.2 ms** |
| `refreshLEDs()` | GPIO | ~2 µs |
| `refreshSWValues()` | GPIO | ~1 µs |
| `refreshTOFValues()` (3 active) | I2C2 | ~1.6 ms |
| `refreshIMUValues()` | I2C2 | ~0.9 ms |
| `refreshINA219Values()` | I2C1 | ~0.5 ms |
| `refreshMotors()` | Timer | ~1 µs |
| `refreshLoggedData()` | — | ~1 µs |
| `sendToSimulink()` *(disabled)* | UART | *(+0.93 ms if enabled)* |
| **Total (screen on)** | | **~28.2 ms → ~35 Hz** |
| **Total (screen off / uninitialized)** | | **~3.0 ms → ~330 Hz** |

> The SSD1306 screen update accounts for **~89 %** of the loop time.  
> ToF, IMU, and INA219 together add **~3 ms** and share I2C2 (sequentially — no contention).  
> INA219 runs on the separate I2C1 bus and does not compete for I2C2 bandwidth.
