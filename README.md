# pinCfgC

The goal of this library is to provide a CSV-based configuration that specifies how IO pins on the MySensors platform will behave.

## Table of Contents
1. [Format Overview](#format-overview)
2. [Comments](#comments)
3. [Special Characters](#special-characters)
4. [Global Configuration Items](#global-configuration-items)
5. [Switches](#switches)
   1. [Basic switch](#basic-switch)
   2. [Basic switch with Feedback](#basic-switch-with-feedback)
   3. [Impulse Output Switch](#impulse-output-switch)
   4. [Impulse Output Switch with Feedback](#impulse-output-switch-with-feedback)
   5. [Timed Output Switch](#timed-output-switch)
6. [Inputs](#inputs)
7. [Triggers](#triggers)
8. [Measurement Sources](#measurement-sources)
   1. [CPU Temperature Measurement](#cpu-temperature-measurement)
   2. [I2C Measurement](#i2c-measurement-compile-time-optional)
9. [Sensor Reporters](#sensor-reporters)

## Format Overview
```
#comment
CD,330/
CM,620/
CR,150/
CN,1000/
CF,30000/
CA,1966080/
S,o01,13,o02,12,o03,11,o04,10,o05,3,o06,2,o07,7/
SI,o10,4/
SF,o09,5,193/
SIF,o08,6,192/
ST,o09,5,5000/
STF,o10,4,6000,200/
I,i01,193,i02,192,i03,19,i04,18,i05,194,i06,195,i07,196,i08,197,i09,200,i10,201,i11,30,i12,31/
T,t01,i01,1,1,o01,0/
T,t02,i02,1,1,o02,0/
T,t03,i03,1,1,o03,0/
T,t04,i04,1,1,o04,0/
T,t05,i05,1,1,o05,0/
T,t06,i06,1,1,o06,0/
T,t07,i07,1,1,o07,0/
T,t08,i08,1,1,o08,0/
T,t09,i09,1,1,o09,0/
T,t10,i10,1,1,o10,0/
MS,0,cpu_temp/
SR,CPUTemp,cpu_temp,6,6,0,0,1000,300,-2.1/
```
## Comments
Lines starting with **#** will be ignored by the parsing function and can be used as comments.

## Special Characters
* **','** is used as the default value separator. It can be changed at compile time by defining: `PINCFG_VALUE_SEPARATOR_D`.
* **'/'** is used as the default line break. It can be changed at compile time by defining: `PINCFG_LINE_SEPARATOR_D`.

## Global Configuration Items
```
CD,330/
CM,620/
CR,150/
CN,1000/
CF,30000/
```
Lines starting with **'C'** are used for defining global parameters:
* **CD**: Debounce interval in milliseconds. Used by inputs to define the period during which input changes are ignored. Default: 100 ms or `PINCFG_DEBOUNCE_MS_D`.
* **CM**: Multiclick maximum delay in milliseconds. Maximum time to recognize a multiclick as separate from single clicks. Used by inputs. Default: 500 ms or `PINCFG_MULTICLICK_MAX_DELAY_MS_D`.
* **CR**: Impulse duration in milliseconds. Global constant used by switches to define how long a switch stays ON in impulse mode. Default: 300 ms or `PINCFG_SWITCH_IMPULSE_DURATIN_MS_D`.
* **CN**: Feedback pin ON delay in milliseconds. Global constant used by switches to specify the maximum waiting period for feedback to match the switch state when turned ON. If feedback remains OFF after this period, the switch is set back to OFF and a status message is sent. Default: 1000 ms or `PINCFG_SWITCH_FB_ON_DELAY_MS_D`.
* **CF**: Feedback pin OFF delay in milliseconds. Global constant used by switches to specify the maximum waiting period for feedback to match the switch state when turned OFF. If feedback remains ON after this period, the switch is set back to ON and a status message is sent. Default: 30000 ms or `PINCFG_SWITCH_FB_OFF_DELAY_MS_D`.

## CLI

The CLI (Command Line Interface) allows configuration and control of the device via special messages. It supports receiving configuration data, executing commands, and reporting status.

### Supported Commands

* **GET_CFG**: Returns the current configuration as a CSV string.
* **RESET**: Resets the device.

### Configuration Upload

To upload a new configuration, send a message starting with `#[` and ending with `]#`. The configuration content should be in the same CSV format as described in this document.

#### Example

```
#[CD,330/CM,620/CR,150/CN,1000/CF,30000/S,o01,13,o02,12/]#
```

### State Reporting

The CLI reports its state using the following status messages:
- **LISTENING**: Ready to receive configuration or commands.
- **OUT_OF_MEMORY_ERROR**: Not enough memory to process the request.
- **RECEIVING**: Receiving configuration data.
- **RECEIVED**: Configuration data received.
- **VALIDATING**: Validating configuration.
- **VALIDATION_OK**: Configuration is valid and saved.
- **VALIDATION_ERROR**: Configuration is invalid.

Custom error messages may also be sent if an operation fails.

### Usage Notes

- Configuration upload and commands are handled via the same interface.
- Large configuration or response messages may be split into multiple parts.
- After a successful configuration upload and validation, the device will automatically reset to apply the new configuration.

## Switches
Switches are abstract configuration units that specify how output pins will behave.
Lines starting with **'S'** are parsed as switch definitions.
Each line adds to the existing configuration, and it is possible to specify multiple switches of the same variant in one line.

### Basic switch
Switches ON or OFF according to state.
#### Line Format
```
1. S,    (Switch type definition)
2. o01,  (name of switch 1)
3. 13,   (pin of switch 1)
   ...,
x. ox,   (name of switch x)
y. xx/   (pin of switch x)
```
#### Example
```S,o01,13,o02,12,o03,11,o04,10,o05,3,o06,2,o07,7/```

### Basic switch with Feedback
A [Basic switch](#basic-switch) with state reporting logic driven by a feedback pin.
Feedback pin behavior is adjusted by **CN/CF** in [Global Configuration Items](#global-configuration-items).
#### Line Format
```
1. SF,   (Switch type definition)
2. o01,  (name of switch 1)
3. 13,   (pin of switch 1)
4. 193,  (feedback pin of switch 1)
   ...,
x. ox,   (name of switch x)
y. xx,   (pin of switch x)
z. xxf/  (feedback pin of switch x)
```
#### Example
```SF,o09,5,193/```

### Impulse Output Switch
Switches from OFF to ON and back to OFF for a period defined by the **CR** global constant described in [Global Configuration Items](#global-configuration-items).
#### Line Format
```
1. SI,   (Switch type definition)
2. o01,  (name of switch 1)
3. 13,   (pin of switch 1)
   ...,
x. ox,   (name of switch x)
y. xx/   (pin of switch x)
```
#### Example
```SI,o10,4/```

### Impulse Output Switch with Feedback
An [Impulse Output Switch](#impulse-output-switch) with state reporting logic driven by a feedback pin, as in [Switch with Feedback](#switch-with-feedback).
#### Line Format
```
1. SIF,  (Switch type definition)
2. o01,  (name of switch 1)
3. 13,   (pin of switch 1)
4. 193,  (feedback pin of switch 1)
   ...,
x. ox,   (name of switch x)
y. xx,   (pin of switch x)
z. xxf/  (feedback pin of switch x)
```
#### Example
```SIF,o08,6,192/```

### Timed Output Switch
Similar to [Impulse Output Switch](#impulse-output-switch), but the line also specifies the impulse duration (50–600000 ms).
#### Line Format
```
1. ST,   (Switch type definition)
2. o01,  (name of switch 1)
3. 13,   (pin of switch 1)
4. 300000 (ON period duration of switch 1)
   ...,
x. ox,   (name of switch x)
y. xx,   (pin of switch x)
z. xxx   (ON period duration of switch x)
```
#### Trigger Behavior
When **OFF** and a trigger event fires, the switch turns **ON** for the period specified in [Line Format](#line-format-2).<br>
If, during the **ON** period, an input event **Up** ([Input Event Types](#input-event-types)) is forwarded using **forward** ([Switch Action](#switch-actions-definition)), the **ON** period is prolonged by the defined duration.<br>
If, during the **ON** period, an input event **Longpress** is forwarded using **forward**, the switch is turned **OFF** immediately.<br>
If no additional event occurs during the **ON** period, the switch will be turned **OFF** after the cumulative period passes.
#### Example
```
ST,o01,13,30000/
T,t01,i01,4,1,o01,3/
```

## Inputs
Inputs are configuration items that specify how input pins will behave.
They are event producers used by **triggers**.
Lines starting with **'I'** are parsed as input definitions.
Each line adds to the existing configuration, and it is possible to specify multiple inputs in one line.

### Line Format
```
1. I,   (starts with I for inputs)
2. i01, (name of input  1)
3. 16,  (pin of input  1)
   ...,
x. ix,  (name of input x)
y. xx/  (pin of input x)
```

### Input Event Types
* **0 - Down**: Change from ON to OFF, and the OFF state must last longer than the debounce interval specified by **CD** in [Global Configuration Items](#global-configuration-items).
* **1 - Up**: Change from OFF to ON, and the ON state must last longer than the debounce interval specified by **CD**.
* **2 - Longpress**: Change from OFF to ON and remain ON for a period longer than the multiclick constant specified by **CM**.
* **3 - Multiclick**: Series of OFF-ON-OFF-ON-OFF changes, where two OFF-ON changes cannot last longer than the multiclick constant specified by **CM**.

#### Example
```I,i01,193,i02,192,i03,19,i04,18,i05,194,i06,195,i07,196,i08,197,i09,200,i10,201,i11,30,i12,31/```

## Triggers
Triggers specify how input events are transformed into output actions.
Lines starting with **'T'** are parsed as trigger definitions.

### Line Format
```
1. T,   (starts with one character T for triggers)
2. t01, (name of trigger 1)
3. i01, (name of input to listen to)
4. 1,   (input event type to listen to: 0-down, 1-up, 2-longpress, 3-multi, 4-all)
5. 1,   (event count - number of actions to trigger switch/switches)
6. o01, (name of driven switch 1)
7. 0,   (switch action 1: 0-toggle, 1-turn on, 2-turn off, 3-forward)
   ...,
x. oXX, (name of driven switch x)
y. 0/   (switch action x)
```
### Input Event Listening
Line element 3 is the **name** of the **input** whose events the trigger will listen to.  
Line element 4 is the input event [type](#input-event-types) to listen for, or **4 - All** to listen to all events produced by the input.  
Line element 5 is the number of occurrences of the event to listen for. Use 0 for DOWN or longpress events; otherwise, specify the count for UP or Multiclick.

### Switch Action(s) Definition
Trigger line elements from 6 onward are pairs: 1. switch name and 2. switch action.
Switch actions can be:
* **0 - toggle**: Toggle the state of the switch.
* **1 - on**: Set the switch state to ON.
* **2 - off**: Set the switch state to OFF.
* **3 - forward**: Forward the input event to the switch.

## Measurement Sources
Measurement sources define hardware reading logic without timing or reporting behavior. This allows multiple sensors to share the same measurement source.

Lines starting with **'MS'** are parsed as measurement source definitions.

### CPU Temperature Measurement
Defines a CPU temperature measurement source that can be used by one or more sensor reporters.

#### Line Format
```
1. MS,  (measurement source type definition)
2. 0,   (measurement type enum: 0 = CPU temperature)
3. cpu/ (unique name for this measurement)
```

#### Parameters
1. **Type** (`uint8_t`) - Measurement type enum value
   * **0** = `MEASUREMENT_TYPE_CPUTEMP_E` - CPU temperature sensor
   * **1** = `MEASUREMENT_TYPE_ANALOG_E` - Analog input (Phase 3)
   * **2** = `MEASUREMENT_TYPE_DIGITAL_E` - Digital input (Phase 3)
   * **3** = `MEASUREMENT_TYPE_I2C_E` - I2C sensor (Phase 3)
   * **4** = `MEASUREMENT_TYPE_CALCULATED_E` - Calculated (Phase 3)
2. **Name** (`char[]`) - Unique name for this measurement source (used by sensor reporters)

#### Examples
```
MS,0,cpu/                 # CPU temp measurement (type 0)
MS,0,basement_temp/       # Named measurement for basement
MS,0,external_sensor/     # Another measurement source
```

**Note:** Using numeric type enum instead of string for compact CSV format.

### I2C Measurement (Compile-Time Optional)
Defines an I2C sensor measurement source with non-blocking read support.

**Requires:** `FEATURE_I2C_MEASUREMENT` defined at compile time.

#### Line Formats

**Simple Mode (5 parameters)** - For register-based sensors:
```
MS,3,<name>,<i2c_addr>,<register>,<data_size>/
```

**Command Mode (6-7 parameters)** - For triggered sensors:
```
MS,3,<name>,<i2c_addr>,<cmd1>,<data_size>,<cmd2>[,<cmd3>]/
```

#### Parameters
1. **Type** (`uint8_t`) - Must be **3** (`MEASUREMENT_TYPE_I2C_E`)
2. **Name** (`char[]`) - Unique name for this measurement source
3. **I2C Address** (`uint8_t`) - 7-bit I2C device address (hex: 0x48 or decimal: 72)
4. **Register/Cmd1** (`uint8_t`) - Register address (simple) or first command byte (command mode)
5. **Data Size** (`uint8_t`) - Number of bytes to read (1-6)
6. **Cmd2** (optional, `uint8_t`) - Second command byte (triggers command mode)
7. **Cmd3** (optional, `uint8_t`) - Third command byte

#### Mode Detection
- **5 parameters** → Simple mode: Write register → Read immediately
- **6-7 parameters** → Command mode: Write command → Wait → Read

**Auto Conversion Delays:**
- `0xAC` (AHT10 trigger): 80ms
- `0xF4` (BME280 forced): 10ms
- Others: 0ms (immediate)

#### Data Format
- **Byte Order:** Big-endian (MSB first) - standard for most I2C sensors
- **Sign Extension:** Automatic for signed values (based on MSB)
- **Scaling:** Apply sensor-specific scaling via offset parameter in SR line

#### Examples

**Simple Mode Sensors:**
```
# TMP102 temperature sensor (12-bit, 0.0625°C per LSB)
MS,3,tmp102_raw,0x48,0x00,2/                        # 5 params = simple mode
SR,RoomTemp,tmp102_raw,6,6,0,0,5000,300,0.0625/    # V_TEMP, S_TEMP, scale by 0.0625

# BH1750 light sensor (16-bit, 1 lux per LSB)
MS,3,light_raw,0x23,0x10,2/                         # Simple mode
SR,Brightness,light_raw,37,16,0,0,1000,60,1.0/     # V_LIGHT_LEVEL, S_LIGHT_LEVEL

# Multiple I2C sensors at different addresses
MS,3,tmp102_1,0x48,0x00,2/                          # First TMP102
MS,3,tmp102_2,0x49,0x00,2/                          # Second TMP102 (different address)
SR,IndoorTemp,tmp102_1,6,6,0,0,5000,300,0.0625/
SR,OutdoorTemp,tmp102_2,6,6,0,0,5000,300,0.0625/
```

**Command Mode Sensors:**
```
# AHT10 temperature/humidity sensor
MS,3,aht10_raw,0x38,0xAC,6,0x33,0x00/              # 7 params = command mode
                                                    # Writes: 0xAC, 0x33, 0x00
                                                    # Waits: 80ms (auto)
                                                    # Reads: 6 bytes
SR,Temperature,aht10_raw,6,6,0,0,10000,300,0.0/    # V_TEMP, S_TEMP, read every 10s

# BME280 pressure sensor  
MS,3,bme280_raw,0x76,0xF4,8,0x25/                  # 6 params = command mode
                                                    # Writes: 0xF4, 0x25 (forced mode)
                                                    # Waits: 10ms (auto)
SR,Pressure,bme280_raw,17,4,0,0,5000,300,1.0/       # V_PRESSURE, S_BARO, read every 5s
```

#### Multi-Value Sensors

Many I2C sensors return multiple measurements in one read (e.g., AHT10 returns both temperature and humidity in 6 bytes). To extract specific values, use byte offset and count parameters in the SR line.

**Extended SR Format:**
```
SR,<name>,<meas>,<vType>,<sType>,<en>,<cum>,<samp>,<rep>,<offset>,<byte_offset>,<byte_count>/
```

**Byte Extraction Parameters:**
- **byte_offset** (param 10, optional): Starting byte index (0-5), default 0
- **byte_count** (param 11, optional): Number of bytes to extract (1-6, 0=all), default 0

**Default behavior (0, 0):** Extract all bytes from measurement
**Custom extraction:** Specify byte range for multi-value sensors

**Example: AHT10 Temperature + Humidity**

AHT10 returns 6 bytes containing both values:
- Temperature: bytes 3-5 (20-bit value)
- Humidity: bytes 1-3 (20-bit value)

```csv
# Single I2C read for both measurements
MS,3,aht10,0x38,0xAC,6,0x33,0x00/

# Extract temperature (bytes 3-5) and scale (V_TEMP=6, S_TEMP=6)
SR,Temperature,aht10,6,6,0,0,10000,300,0.000191,3,3/

# Extract humidity (bytes 1-3) and scale (V_HUM=1, S_HUM=7)
SR,Humidity,aht10,1,7,0,0,10000,300,0.000095,1,3/
```

**Benefits:**
- Single I2C read for both values (efficient)
- Clear separation of temperature and humidity
- No duplicate I2C transactions
- Each value has independent scaling factor

**Other Multi-Value Examples:**

```csv
# BME280: Temperature + Pressure + Humidity in 8 bytes
MS,3,bme280,0x76,0xF4,8,0x25/
SR,Temp,bme280,6,6,0,0,10000,300,0.01,0,3/       # V_TEMP, S_TEMP, bytes 0-2
SR,Press,bme280,17,4,0,0,10000,300,0.01,3,3/     # V_PRESSURE, S_BARO, bytes 3-5
SR,Hum,bme280,1,7,0,0,10000,300,0.001,6,2/       # V_HUM, S_HUM, bytes 6-7

# SHT31: Temperature + Humidity in 6 bytes
MS,3,sht31,0x44,0x2C,6,0x06/
SR,Temp,sht31,6,6,0,0,10000,300,0.00268,0,2/     # V_TEMP, S_TEMP, bytes 0-1
SR,Hum,sht31,1,7,0,0,10000,300,0.00152,3,2/      # V_HUM, S_HUM, bytes 3-4
```

#### Non-Blocking Operation
I2C measurements use a state machine to avoid blocking the main `loop()`:
- **Simple mode:** Write register → Check data → Return value
- **Command mode:** Write command → Wait conversion → Check data → Return value

The sensor automatically retries on PENDING, so multiple `loop()` iterations may be needed per reading.

**Timeout:** 100ms default (compile-time configurable)

#### Supported Sensors

**Simple Mode:** TMP102, LM75, LM75A, BH1750  
**Command Mode:** AHT10, BME280, SHT31

See `PHASE3_IMPLEMENTATION.md` for complete sensor compatibility list.

#### Compile-Time Configuration
Enable I2C support by defining:

**In `Globals.h`:**
```c
#define FEATURE_I2C_MEASUREMENT
```

**Or in `platformio.ini`:**
```ini
[env:myboard]
build_flags = 
    -DFEATURE_I2C_MEASUREMENT
```

**Binary Size:** +800-1000 bytes when enabled, +0 bytes when disabled.

**Compatibility:** Both 5-parameter (simple) and 6-7 parameter (command) formats are supported.

## Sensor Reporters
Sensor reporters define timing, averaging, and MySensors reporting behavior. They reference measurement sources by name.

Lines starting with **'SR'** are parsed as sensor reporter definitions.

### Line Format
```
1. SR,       (sensor reporter type definition)
2. CPUTemp,  (unique sensor name)
3. cpu,      (measurement source name - must match an MS definition)
4. 6,        (V_TYPE: MySensors variable type - e.g., 6 = V_TEMP)
5. 6,        (S_TYPE: MySensors sensor type - e.g., 6 = S_TEMP)
6. 0,        (enableable: 0 or 1)
7. 0,        (cumulative mode: 0 or 1)
8. 1000,     (sampling interval in milliseconds)
9. 300,      (reporting interval in SECONDS)
10. 0.0,     (optional temperature offset/scale factor)
11. 0,       (optional byte offset for multi-value sensors)
12. 0/       (optional byte count for multi-value sensors)
```

#### Parameters
1. **Name** (`char[]`) - Unique name for this sensor
2. **Measurement Source** (`char[]`) - Name of measurement source (from MS line)
3. **V_TYPE** (`uint8_t`) - MySensors variable type (see MySensors documentation)
    * **0** = V_TEMP (temperature)
    * **1** = V_HUM (humidity)
    * **2** = V_STATUS (binary status)
    * **6** = V_TEMP (legacy, same as 0)
    * See MySensors `mysensors_data_t` for complete list
4. **S_TYPE** (`uint8_t`) - MySensors sensor type (see MySensors documentation)
    * **0** = S_DOOR (door/window sensor)
    * **1** = S_MOTION (motion sensor)
    * **6** = S_TEMP (temperature sensor)
    * **7** = S_HUM (humidity sensor)
    * See MySensors `mysensors_sensor_t` for complete list
5. **Enableable** (`uint8_t`) - 0 or 1
    * **0** - Sensor always reports
    * **1** - Sensor can be enabled/disabled at runtime (increases memory)
6. **Cumulative Mode** (`uint8_t`) - 0 or 1
    * **0** (standard) - Temperature measured once per report interval
    * **1** (cumulative) - Temperature sampled at sampling interval, average reported
7. **Sampling Interval** (`uint16_t`) - In **milliseconds**, default 1000 ms or `PINCFG_CPUTEMP_SAMPLING_INTV_MS_D`
    * Valid range: 100-5000 ms (100 ms - 5 seconds)
8. **Report Interval** (`uint16_t`) - In **SECONDS**, default 300 s (5 min) or `PINCFG_CPUTEMP_REPORTING_INTV_SEC_D`
    * Valid range: 1-3600 seconds (1 second - 1 hour)
9. **Offset** (optional, `float`) - Scaling factor or calibration offset, default 0.0 or `PINCFG_CPUTEMP_OFFSET_D`
    * For temperature: calibration offset in °C
    * For I2C: scaling multiplier (e.g., 0.0625 for TMP102)
10. **Byte Offset** (optional, `uint8_t`) - Starting byte index for multi-value sensors (0-5), default 0
11. **Byte Count** (optional, `uint8_t`) - Number of bytes to extract (1-6, 0=all), default 0

#### Examples
```
# Basic temperature sensor reporting every 5 minutes
SR,CPUTemp,cpu,6,6,0,0,1000,300/

# Temperature sensor with calibration offset
SR,CPUTemp_calibrated,cpu,6,6,0,0,1000,300,-2.1/

# Enableable humidity sensor with averaging, reporting every minute
SR,Humidity_avg,humid_sensor,1,7,1,1,500,60,0.0/

# Multiple sensors sharing one measurement source with different types
MS,0,shared/                                           # type 0 = CPU temp
SR,fast_reporter,shared,6,6,0,0,500,10,0.0/          # V_TEMP, S_TEMP, report every 10 sec
SR,slow_reporter,shared,6,6,0,0,1000,3600,2.5/       # V_TEMP, S_TEMP, +2.5°C offset, hourly
SR,calibrated,shared,6,6,0,0,1000,300,-1.0/          # V_TEMP, S_TEMP, -1.0°C offset, 5 min
```

### Measurement Reusability
Multiple sensor reporters can use the same measurement source, allowing different reporting schedules and calibrations without duplicate hardware readings:

```
MS,0,cpu_temp/                                    # One raw measurement (type 0 = CPU temp)
SR,sensor_fast,cpu_temp,6,6,0,0,500,60,0.0/      # V_TEMP, S_TEMP, report every minute
SR,sensor_slow,cpu_temp,6,6,0,0,1000,600,2.5/    # V_TEMP, S_TEMP, +2.5°C offset, 10 min
SR,sensor_avg,cpu_temp,6,6,0,1,2000,300,-1.0/    # V_TEMP, S_TEMP, -1.0°C, 5-min average
```

Each sensor can apply its own calibration offset to the raw measurement, enabling flexible use cases like:
- Raw reading for debugging
- Calibrated reading for home automation
- Different offsets for different zones

**Important:** MS lines must appear **before** SR lines that reference them.
