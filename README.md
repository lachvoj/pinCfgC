# pinCfgC

The goal of this library is to provide a CSV-based configuration that specifies how IO pins on the MySensors platform will behave.

## Table of Contents
1. [Format Overview](#format-overview)
2. [Memory Management](#memory-management)
3. [Comments](#comments)
4. [Special Characters](#special-characters)
5. [Global Configuration Items](#global-configuration-items)
6. [CLI](#cli)
   1. [Configuration Upload](#configuration-upload)
   2. [Supported Commands](#supported-commands)
   3. [Quick Start Example](#quick-start-example)
   4. [State Reporting](#state-reporting)
   5. [Security](#security)
   6. [Usage Notes](#usage-notes)
   7. [Compile-Time Configuration](#compile-time-configuration)
   8. [Troubleshooting](#troubleshooting)
   9. [Configuration Generator](#configuration-generator)
7. [Switches](#switches)
   1. [Basic switch](#basic-switch)
   2. [Basic switch with Feedback](#basic-switch-with-feedback)
   3. [Impulse Output Switch](#impulse-output-switch)
   4. [Impulse Output Switch with Feedback](#impulse-output-switch-with-feedback)
   5. [Timed Output Switch](#timed-output-switch)
8. [Inputs](#inputs)
9. [Triggers](#triggers)
10. [Measurement Sources](#measurement-sources)
    1. [CPU Temperature Measurement](#cpu-temperature-measurement)
    2. [I2C Measurement](#i2c-measurement-compile-time-optional)
    3. [SPI Measurement](#spi-measurement-compile-time-optional)
    4. [Loop Time Measurement](#loop-time-measurement-debug)
    5. [Analog Measurement](#analog-measurement-compile-time-optional)
11. [Sensor Reporters](#sensor-reporters)

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

## Memory Management

The pinCfgC library requires memory to store its internal data structures. You can choose between two memory allocation strategies:

### Pre-Allocated Memory (Default)

By default, the library uses a **pre-allocated static buffer** that you provide during initialization. This approach is recommended for embedded systems as it:
- Provides predictable memory usage
- Avoids heap fragmentation
- Eliminates runtime allocation failures
- Is suitable for safety-critical applications

#### Usage Example
```cpp
// Define memory buffer size
#ifndef PINCFG_MEMORY_SZ
#define PINCFG_MEMORY_SZ 5000
#endif

// Allocate static buffer
uint8_t au8PincfgMemory[PINCFG_MEMORY_SZ];

// Initialize library with buffer
void before(void) {
    PinCfgCsv_eInit(au8PincfgMemory, PINCFG_MEMORY_SZ, sDefaultConfig);
}
```

**Memory Size Guidelines:**
- Basic configuration (few switches/inputs): ~1000-2000 bytes
- Medium configuration (10-20 switches): ~3000-5000 bytes
- Complex configuration (sensors, I2C/SPI): ~5000-8000 bytes

### Heap Allocation (Optional)

For platforms with sufficient RAM and dynamic memory management, you can enable heap allocation using the `USE_MALLOC` compile-time flag. This approach:
- Allocates memory dynamically using `malloc()`
- May be more flexible for development/testing
- Requires careful monitoring on resource-constrained devices

#### Usage Example
```cpp
// Enable heap allocation
#define USE_MALLOC

// Pointer to dynamically allocated memory
uint8_t *au8PincfgMemory = NULL;

// Initialize library (memory will be allocated internally)
void before(void) {
    PinCfgCsv_eInit(au8PincfgMemory, 0, sDefaultConfig);
}
```

#### Compile-Time Configuration

**In your code:**
```cpp
#define USE_MALLOC
#include <PinCfg.h>
```

**Or in `platformio.ini`:**
```ini
[env:myboard]
build_flags = 
    -DUSE_MALLOC
```

**Note:** When `USE_MALLOC` is defined, the memory size parameter is ignored and memory is allocated dynamically based on configuration requirements.

### Choosing the Right Approach

| Criteria | Pre-Allocated | Heap Allocation |
|----------|---------------|-----------------|
| **Memory Safety** | ✅ Predictable | ⚠️ May fragment |
| **Embedded Systems** | ✅ Recommended | ⚠️ Use with caution |
| **RAM Usage** | Fixed at compile-time | Dynamic at runtime |
| **Setup Complexity** | Simple | Requires malloc support |
| **Best For** | Production, STM32, AVR | Development, ESP32, Linux |

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

**All CLI operations require authentication using a SHA-256 password hash.** The password is provided as a 64-character hex string representation of the SHA-256 hash. The default password hash corresponds to `admin123` and should be changed after initial deployment.

**Generating Password Hash:**
```bash
# Using the provided script:
python gen_password_hash.py "yourpassword"

# Or using command line:
echo -n "yourpassword" | sha256sum | cut -d' ' -f1
```

**Default Password Hash** (for `admin123`):
```
240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9
```

### Configuration Upload

To upload a new configuration, send a message in the format: `#[<64-char-hex-hash>/CFG:configuration]#`

The configuration content should be in the same CSV format as described in this document.

#### Format

```
#[<sha256_hex_hash>/CFG:configuration_data]#
```

#### Example

```
#[240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9/CFG:CD,330/S,o01,13/]#
```

**Note**: The password hash (64 hex characters) must be followed by `/CFG:` before the configuration data.

### Supported Commands

Commands use the format: `#[<sha256_hex_hash>/CMD:COMMAND]#`

**Note**: Commands use the same unified format as configuration with `CMD:` prefix instead of `CFG:`.

#### Available Commands

* **GET_CFG**: Returns the current configuration as a CSV string (without the password).
  ```
  #[240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9/CMD:GET_CFG]#
  ```

* **RESET**: Resets the device immediately.
  ```
  #[240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9/CMD:RESET]#
  ```

* **CHANGE_PWD**: Changes the stored password hash. Format: `#[<current_hash>/CMD:CHANGE_PWD:<new_hash>]#`
  ```
  #[240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9/CMD:CHANGE_PWD:5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8]#
  ```
  (This changes from `admin123` to `password`)

**Password Hash Requirements**:
- Exactly 64 hexadecimal characters (SHA-256 hash)
- Stored as 32-byte binary in EEPROM with CRC protection
- Use `gen_password_hash.py` to generate hash from plaintext

### Quick Start Example

Here's a typical workflow for setting up a device:

**First, generate your password hash:**
```bash
python gen_password_hash.py "mySecurePassword123"
# Output: 2b8a8f1f51f5b34b85b3f2fd5ff5b4f1... (64 hex characters)
```

1. **Initial Setup** - Change the default password (using hex hashes):
   ```
   #[240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9/CMD:CHANGE_PWD:<your_new_hash>]#
   ```
   Response: `Pwd changed OK.`

2. **Upload Configuration** - Send your configuration with the new password hash:
   ```
   #[<your_new_hash>/CFG:CD,330/CM,620/S,o01,13,o02,12/I,i01,2/T,t01,i01,1,1,o01,0/]#
   ```
   The device will:
   - Respond with `RECEIVING_DATA` status
   - Echo back received data
   - Validate the configuration
   - Report `VALIDATION_OK`
   - Save and automatically reset

3. **Retrieve Configuration** - Get the current configuration:
   ```
   #[<your_hash>/CMD:GET_CFG]#
   ```
   Response: The configuration CSV (without password)

4. **Reset Device** - Manually reset if needed:
   ```
   #[<your_hash>/CMD:RESET]#
   ```

### State Reporting

The CLI reports its state using the following status messages:

**Standard States:**
- **READY**: Ready to receive configuration or commands.
- **OUT_OF_MEMORY_ERROR**: Not enough memory to process the request.
- **RECEIVING_AUTH**: Receiving fragmented password.
- **RECEIVING_DATA**: Receiving configuration or command data (echoes back received messages).
- **RECEIVED**: Configuration data received.
- **VALIDATING**: Validating configuration.
- **VALIDATION_OK**: Configuration is valid and saved.
- **VALIDATION_ERROR**: Configuration is invalid.

**Authentication-Related Messages:**
- **Authentication required.**: Sent when attempting operations without password.
- **Authentication failed.**: Sent when incorrect password is provided.
- **Locked out. Try again later.**: Sent when too many failed attempts (rate limiting).
- **Invalid pwd length.**: Sent when password hash length is incorrect.
- **Invalid message type.**: Sent when message doesn't start with CFG: or CMD: after password.
- **Pwd changed OK.**: Sent after successful password change.
- **Pwd change failed.**: Sent if password change operation fails.

**Command-Related Messages:**
- **Unknown command.**: Sent when command is not recognized.
- **Data too large.**: Sent when configuration or command exceeds buffer size.
- **No config data.**: Sent when GET_CFG is called but no configuration exists.
- **Unable to read cfg size.**: Sent when configuration size cannot be determined.
- **Unable to load cfg.**: Sent when configuration cannot be loaded from EEPROM.
- **Save of cfg unsucessfull.**: Sent when configuration save operation fails.

Custom error messages may also be sent if an operation fails. Validation errors include detailed information about what went wrong.

### Security

**Default Password Hash**: The system ships with the default password hash for `admin123`:
```
240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9
```
**Change this immediately after deployment** using the `CHANGE_PWD` command.

**Password Hashing**: 
- Passwords are transmitted as 64-character hex strings (SHA-256 hash)
- This prevents plaintext passwords from appearing in logs or network captures
- Use `gen_password_hash.py` to convert plaintext to hash

**Password Storage**: Password hashes are stored in EEPROM as 32-byte binary (converted from hex). They benefit from:
- CRC16 integrity checking
- Triple metadata voting for fault tolerance
- Block checksums
- Dynamic backup (coverage scales with config size - smaller configs get 100% backup)
- Fault-tolerant recovery mechanisms

**Authentication**: Every configuration upload and command requires authentication. The provided hex password hash is converted to binary and compared against the stored hash using constant-time comparison to prevent timing attacks. Authentication state is not cached and is reset after each operation.

### Usage Notes

- **Authentication Required**: All configuration uploads and commands require password authentication.
- **Configuration Upload**: Configuration data is processed only after successful authentication. The device echoes back received messages during the RECEIVING state.
- **Large Messages**: Configuration or response messages are automatically split into chunks (default chunk size defined by `PINCFG_TXTSTATE_MAX_SZ_D`) with delays between transmissions (`PINCFG_LONG_MESSAGE_DELAY_MS_D`).
- **Automatic Reset**: After a successful configuration upload and validation, the device automatically resets to apply the new configuration.
- **Password Persistence**: When uploading a new configuration, the existing password is preserved. Only the configuration data is updated unless you explicitly use `CHANGE_PWD`.
- **Memory Management**: The CLI uses temporary memory allocation (default `PINCFG_CONFIG_MAX_SZ_D` = 480 bytes) for configuration processing. Ensure sufficient free memory is available.
- **Home Assistant Integration**: For best compatibility with Home Assistant, keep CLI commands to **18 characters or less** per line. This ensures reliable message handling in Home Assistant's MySensors integration.

### Compile-Time Configuration

The following constants can be defined at compile-time to customize CLI behavior:

| Constant | Default | Description |
|----------|---------|-------------|
| `PINCFG_AUTH_PASSWORD_LEN_D` | 32 | Binary password hash storage size in EEPROM (bytes). SHA-256 = 32 bytes. |
| `PINCFG_AUTH_HEX_PASSWORD_LEN_D` | 64 | Hex string length for password hash (2 × binary size). |
| `PINCFG_CONFIG_MAX_SZ_D` | 480 | Maximum configuration size (bytes). Total size for temporary configuration buffer. |
| `CLI_AUTH_DEFAULT_PASSWORD` | (SHA-256 of "admin123") | Default password hash for new devices. **Change after deployment!** |
| `PINCFG_TXTSTATE_MAX_SZ_D` | (varies) | Maximum size of text message chunks for large message transmission. |
| `PINCFG_LONG_MESSAGE_DELAY_MS_D` | (varies) | Delay (milliseconds) between message chunks when sending large responses. |

**Example** (in `platformio.ini` or compiler flags):
```ini
build_flags = 
    -DPINCFG_CONFIG_MAX_SZ_D=1024
```

**Note**: Password hash storage size is fixed at 32 bytes for SHA-256. Do not modify `PINCFG_AUTH_PASSWORD_LEN_D`.

### Troubleshooting

**Problem: "Authentication required." or "Authentication failed."**
- Verify you're using the 64-character hex hash (not plaintext): `#[<64-hex>/CFG:config]#` or `#[<64-hex>/CMD:command]#`
- Use `gen_password_hash.py "yourpassword"` to generate the correct hash
- Check if you changed the password from the default hash
- Ensure the format is `#[password/CFG:...]#` or `#[password/CMD:...]#`
- Both config and commands use `]#` terminator at the end
- If locked out, wait for lockout period or reflash the device firmware to reset to default password hash

**Problem: "OUT_OF_MEMORY_ERROR"**
- Your configuration is too large for available memory
- Reduce the configuration size or increase `PINCFG_CONFIG_MAX_SZ_D` at compile-time
- Ensure other parts of your program aren't consuming too much memory

**Problem: "VALIDATION_ERROR" with no details**
- Enable validation output by ensuring sufficient free memory
- Check CSV syntax (separators, line endings, missing values)
- Verify pin numbers are valid for your hardware
- Ensure all referenced IDs exist (e.g., triggers referencing inputs/outputs)

**Problem: "Invalid message type." or "Invalid pwd length."**
- Password must be exactly 64 hex characters (SHA-256 hash)
- Both configuration and commands use unified format: `#[<64-hex>/TYPE:data]#`
- Configuration must use `CFG:` prefix: `#[<hash>/CFG:config]#`
- Commands must use `CMD:` prefix: `#[<hash>/CMD:command]#`
- Check for typos in format strings or incomplete hash

**Problem: Messages getting truncated or lost**
- Large messages are automatically split into chunks
- If using Home Assistant, keep commands under 18 characters per line
- Increase `PINCFG_LONG_MESSAGE_DELAY_MS_D` if messages arrive too quickly

**Problem: Password change doesn't persist after reset**
- Verify EEPROM is working correctly on your hardware
- Check that `PersistentCfg` is properly initialized
- Ensure write operations complete before power loss

### Configuration Generator

An **HTML-based configuration generator** is available to simplify the creation of pinCfgC configurations. This tool provides a graphical interface for building configuration strings without manually writing CSV format.

**Location**: [`config/config-tool.html`](config/config-tool.html)

#### Why Build is Required

The configuration tool is distributed as a **single-file HTML application** for portability and ease of use. However, the source code is maintained in separate files (`config/src/`) for better maintainability:
- Separate HTML template, CSS styles, and JavaScript modules
- Automatic extraction of MySensors V_TYPES and S_TYPES from library headers
- Easier version control and collaborative development

The build process combines these separate files into one standalone HTML file that can be opened directly in any web browser without requiring a web server.

#### How to Build

To build the configuration tool, run:

```bash
cd config/src
python3 build-config-tool.py
```

This will generate the `config-tool.html` file in the `config/` directory. The built file is a self-contained HTML application that can be opened directly in any modern web browser.

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

**Requires:** `PINCFG_FEATURE_I2C_MEASUREMENT` defined at compile time.

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
#define PINCFG_FEATURE_I2C_MEASUREMENT
```

**Or in `platformio.ini`:**
```ini
[env:myboard]
build_flags = 
    -DPINCFG_FEATURE_I2C_MEASUREMENT
```

**Binary Size:** +800-1000 bytes when enabled, +0 bytes when disabled.

**Compatibility:** Both 5-parameter (simple) and 6-7 parameter (command) formats are supported.

### SPI Measurement (Compile-Time Optional)

Measures sensor data via SPI interface. Supports simple read and command modes with optional conversion delays. Common use cases: thermocouples (MAX31855), accelerometers (ADXL345), IMUs (MPU9250).

**Requires:** `PINCFG_FEATURE_SPI_MEASUREMENT` defined at compile time.

#### Non-Blocking Operation
SPI measurements use a state machine to avoid blocking the main `loop()`:
- **Simple mode:** Assert CS → Read data → Deassert CS → Return value
- **Command mode (no delay):** Assert CS → Send command → Read data → Deassert CS → Return value
- **Command mode (with delay):** Assert CS → Send command → Deassert CS → Wait → Assert CS → Read data → Deassert CS → Return value

The sensor automatically retries on PENDING, so multiple `loop()` iterations may be needed per reading.

**Timeout:** 100ms default (configurable)

#### Supported Sensors

**Simple Mode (no command):**
- MAX31855 (Thermocouple, 4 bytes)
- MAX6675 (Thermocouple, 2 bytes)

**Command Mode:**
- ADXL345 (Accelerometer, register reads)
- MPU9250 (IMU, multi-register reads)
- BME680 (Environmental sensor)
- LSM6DS3 (6-axis IMU)
- BMI160 (6-axis IMU)

#### Compile-Time Configuration
Enable SPI support by defining:

**In `Globals.h`:**
```c
#define PINCFG_FEATURE_SPI_MEASUREMENT
```

**Or in `platformio.ini`:**
```ini
[env:myboard]
build_flags = 
    -DPINCFG_FEATURE_SPI_MEASUREMENT
```

**Binary Size:** +900-1100 bytes when enabled, +0 bytes when disabled.

**SPI Settings:** MODE0 (CPOL=0, CPHA=0), 1 MHz, MSB first (configurable via `SPI_vSetConfig()`)

📖 **Detailed Documentation:** See `SPI_MEASUREMENT.md` for implementation guide and `SPI_QUICK_START.md` for quick reference.

### Loop Time Measurement (Debug)

Measures main loop execution time for debugging and performance monitoring. Unlike other measurements, loop time is measured **every loop iteration** regardless of sampling interval.

#### Format
```
MS,5,<name>/
```

#### Parameters
- **Type:** `5` = Loop time measurement
- **Name:** Identifier for this measurement (e.g., "looptime")

#### Example Configuration
```
MS,5,looptime/
SR,LoopTime,looptime,99,99,1,1,PINCFG_SENSOR_SAMPLING_INTV_MIN_MS_D,5/
```

Reports average loop execution time every 5 seconds.

**Note:** Replace `PINCFG_SENSOR_SAMPLING_INTV_MIN_MS_D` with actual value (default: 100ms).

#### Key Characteristics
- **Always Measured:** Called every loop iteration (sampling interval parameter is ignored)
- **Requires Cumulative Mode:** Use `cumulative=1` for meaningful statistics
- **Sampling Interval:** Must specify valid value (`PINCFG_SENSOR_SAMPLING_INTV_MIN_MS_D` to `PINCFG_SENSOR_SAMPLING_INTV_MAX_MS_D`, default range: 100-5000 ms) even though it's not used
- **Low Overhead:** ~1 µs per loop iteration
- **Time Units:** Milliseconds
- **First Sample Skipped:** No delta available on first measurement

#### Typical Values
- **Arduino Uno/Nano:** 0.5-2 ms (simple configuration)
- **ESP8266:** 0.2-1 ms (WiFi can cause spikes)
- **ESP32:** 0.1-0.5 ms (dual-core benefits)

⚠️ **Note:** High loop times (>50ms) may indicate blocking operations (delays, synchronous I2C, etc.)

📖 **Detailed Documentation:** See `LOOP_TIME_MEASUREMENT.md` for implementation details and troubleshooting.

### Analog Measurement (Compile-Time Optional)

Measures analog input voltages from ADC pins. Returns raw ADC values - voltage conversion and scaling handled by Sensor layer using offset parameter.

#### Format
```
MS,1,<name>,<pin>/
```

#### Parameters
- **Type:** `1` = Analog measurement
- **Name:** Identifier for this measurement (e.g., "battery", "temp_sensor")
- **Pin:** Analog pin number
  - **Arduino:** 0-7 for A0-A7 (or use actual pin numbers)
  - **STM32:** Configured pin number (via AnalogWrapper)

#### Architecture
- **Measurement Layer:** Returns raw ADC values (0-1023, 0-4095, etc.) via single read
- **Sensor Layer:** Handles timing, averaging (via sampling interval), and voltage conversion (via offset)
- **No Internal Averaging:** Measurement does one read per call - Sensor handles averaging
- **Platform Abstraction:** Uses AnalogWrapper for Arduino/STM32/Mock compatibility

#### Voltage Conversion via Offset

The offset parameter in SR line acts as a **scaling factor** for voltage conversion:

**Formula:** `result = raw_adc_value * offset`

**Common ADC Configurations:**

| Platform | Bits | Max Value | Vref | Offset (V) |
|----------|------|-----------|------|------------|
| Arduino Uno/Nano | 10 | 1023 | 5.0V | 0.00489 |
| Arduino (3.3V) | 10 | 1023 | 3.3V | 0.00323 |
| STM32 / Due | 12 | 4095 | 3.3V | 0.000806 |
| External 16-bit ADC | 16 | 65535 | 3.3V | 0.0000504 |

**Calculation:** `offset = V_reference / ADC_max_value`

#### Example Configurations

**Battery Voltage Monitor:**
```csv
# Measure battery voltage on pin A0 (10-bit, 3.3V reference)
MS,1,battery,0/
SR,BatteryVolt,battery,38,37,0,0,1000,60,0.00323/
# Reports voltage in volts every 60 seconds
# Raw 512 → 512 * 0.00323 = 1.65V
```

**Temperature Sensor (LM35 - 10mV/°C):**
```csv
# LM35 on pin A1 (10-bit, 5V reference)
MS,1,lm35,1/
SR,RoomTemp,lm35,0,6,0,0,2000,120,0.489/
# Scaling: (5.0/1023) / 0.01V/°C = 0.489°C per ADC step
# Raw 205 → 205 * 0.489 = 100.2°C
```

**Soil Moisture Sensor (0-100%):**
```csv
# Moisture sensor on pin A2 (10-bit ADC)
MS,1,moisture,2/
SR,SoilMoisture,moisture,2,35,0,0,5000,300,0.09775/
# Scaling: 100.0 / 1023 = 0.09775% per ADC step
# Raw 512 → 512 * 0.09775 = 50.05%
```

**Light Sensor (Photoresistor):**
```csv
# Light sensor on pin A3 (scale to 0-100 arbitrary units)
MS,1,light,3/
SR,LightLevel,light,3,16,0,0,1000,60,0.09775/
# Same scaling as percentage
```

**Current Sensor (ACS712-5A):**
```csv
# ACS712 on pin A4 (185mV/A, centered at 2.5V, 5V ref)
MS,1,acs712,4/
SR,Current,acs712,39,37,0,0,500,30,0.0264/
# Scaling: (5.0/1023) / 0.185 = 0.0264A per step
# Note: Subtract 512 in application for center offset (2.5V = 0A)
```

#### Sensor-Specific Scaling

**LM35 Temperature (10mV/°C):**
- Voltage per ADC step: `V_ref / ADC_max`
- Temperature per ADC step: `voltage_per_step / 0.01V/°C`
- Example (5V, 10-bit): `(5.0/1023) / 0.01 = 0.489`

**TMP36 Temperature (10mV/°C, 500mV offset):**
- Same scaling as LM35: `0.489`
- Requires subtracting 50°C in application (500mV offset)

**ACS712 Current Sensor (185mV/A):**
- Voltage per ADC step: `5.0 / 1023 = 0.00489V`
- Current per ADC step: `0.00489 / 0.185 = 0.0264A`
- Centered at 2.5V (ADC 512) = 0A

#### Multiple Analog Sensors

```csv
# Define three analog measurement sources
MS,1,analog0,0/
MS,1,analog1,1/
MS,1,analog2,2/

# Voltage sensor (3.3V ref, 12-bit ADC: 0-4095)
SR,Voltage,analog0,38,37,0,0,1000,60,0.000806/

# Current sensor (ACS712, 5V ref, 10-bit)
SR,Current,analog1,39,37,0,0,500,30,0.0264/

# Temperature (LM35, 5V ref, 10-bit)
SR,Temperature,analog2,0,6,0,0,2000,120,0.489/
```

#### MySensors Types for Analog

Common `vType` (variable type) and `sType` (sensor type) combinations:

| Measurement | vType | sType | Description |
|------------|-------|-------|-------------|
| Voltage | 38 (V_VOLTAGE) | 37 (S_MULTIMETER) | Voltage in volts |
| Current | 39 (V_CURRENT) | 37 (S_MULTIMETER) | Current in amps |
| Temperature | 0 (V_TEMP) | 6 (S_TEMP) | Temperature in °C |
| Humidity | 1 (V_HUM) | 7 (S_HUM) | Humidity in % |
| Percentage | 2 (V_STATUS) | 35 (S_MOISTURE) | Generic 0-100% |
| Light Level | 3 (V_LIGHT_LEVEL) | 16 (S_LIGHT_LEVEL) | Light in % |

#### Compile-Time Configuration

Enable analog measurement support:

**In `Globals.h`:**
```c
#define PINCFG_FEATURE_ANALOG_MEASUREMENT
```

**Or in `platformio.ini`:**
```ini
[env:myboard]
build_flags = 
    -DPINCFG_FEATURE_ANALOG_MEASUREMENT
```

**Binary Size:** +200-300 bytes when enabled, +0 bytes when disabled.

📖 **Detailed Documentation:** See `ANALOG_MEASUREMENT_USAGE.md` for more examples and platform-specific notes.

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
10. 1.0,     (optional scale factor - multiplicative, default 1.0)
11. 0.0,     (optional offset - additive adjustment, default 0.0)
12. 0,       (optional precision - decimal places 0-6, default 0)
13. 0,       (optional byte offset for multi-value sensors)
14. 0/       (optional byte count for multi-value sensors)
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
7. **Sampling Interval** (`uint16_t`) - In **milliseconds**, default 1000 ms or `PINCFG_SENSOR_SAMPLING_INTV_MS_D`
    * Valid range: 100-5000 ms (100 ms - 5 seconds)
8. **Report Interval** (`uint16_t`) - In **SECONDS**, default 300 s (5 min) or `PINCFG_SENSOR_REPORTING_INTV_SEC_D`
    * Valid range: 1-3600 seconds (1 second - 1 hour)
9. **Scale** (optional, fixed-point) - Multiplicative scale factor, default 1.0 or `PINCFG_SENSOR_SCALE_D`
    * Applied first in calibration: `result = (raw × scale) + offset`
    * Stored internally as fixed-point integer with 6 decimal places via `PINCFG_FIXED_POINT_SCALE`
    * For I2C sensors: conversion factor (e.g., `0.0625` for TMP102 becomes 62500 internally)
    * For unit conversion: use appropriate multiplier (e.g., `0.01` for percent to decimal)
    * Range: ±1000.0 (configurable via `PINCFG_SENSOR_SCALE_MIN_D`/`MAX_D`)
10. **Offset** (optional, fixed-point) - Additive offset adjustment, default 0.0 or `PINCFG_SENSOR_OFFSET_D`
    * Applied second in calibration: `result = (raw × scale) + offset`
    * Stored internally as fixed-point integer with 6 decimal places
    * For sensor calibration: correction value (e.g., `-2.1` for -2.1°C becomes -2100000 internally)
    * For zero-point adjustment: shift baseline up or down
    * Range: ±1000.0 (configurable via `PINCFG_SENSOR_OFFSET_MIN_D`/`MAX_D`)
11. **Precision** (optional, `uint8_t`) - Decimal places for display/payload sizing, default 0 or `PINCFG_SENSOR_PRECISION_D`
    * Determines MySensors payload type to accommodate value range with decimals
    * **0 decimals**: Uses `P_BYTE` (0-255) or `P_INT16` (-32768 to 32767) for signed values
    * **1-2 decimals**: Uses `P_INT16` (range -32768 to 32767, e.g., ±327.68 with 2 decimals)
    * **3-6 decimals**: Uses `P_LONG32` (range ±2147483647, e.g., ±2147.483 with 3 decimals or ±2.147483 with 6 decimals)
    * Range: 0-6, values >6 are clamped with warning message
12. **Byte Offset** (optional, `uint8_t`) - Starting byte index for multi-value sensors (0-5), default 0
13. **Byte Count** (optional, `uint8_t`) - Number of bytes to extract (1-6, 0=all), default 0

#### Examples
```
# Basic temperature sensor reporting every 5 minutes (default scale=1.0, offset=0, precision=0)
SR,CPUTemp,cpu,6,6,0,0,1000,300/

# Temperature sensor with calibration offset, 1 decimal place
SR,CPUTemp_calibrated,cpu,6,6,0,0,1000,300,1.0,-2.1,1/

# I2C TMP102 sensor with scale factor (0.0625°C per LSB), 2 decimal places
MS,3,tmp102_raw,0x48,0x00,2/                         # I2C measurement
SR,RoomTemp,tmp102_raw,6,6,0,0,5000,300,0.0625,0,2/  # Scale by 0.0625, no offset, 2 decimals

# Humidity sensor with both scale and offset, 1 decimal place
SR,Humidity,humid_raw,1,7,0,0,1000,60,0.01,5.0,1/    # Scale 0.01, add 5.0% offset, 1 decimal

# Enableable sensor with averaging, no decimals (default)
SR,Humidity_avg,humid_sensor,1,7,1,1,500,60/         # Uses P_BYTE or P_INT16

# Multiple sensors sharing one measurement source with different calibrations
MS,0,shared/                                          # type 0 = CPU temp
SR,fast_reporter,shared,6,6,0,0,500,10,1.0,0,0/      # Default, P_BYTE/INT16, every 10 sec
SR,slow_reporter,shared,6,6,0,0,1000,3600,1.0,2.5,1/ # +2.5°C offset, 1 decimal, hourly
SR,calibrated,shared,6,6,0,0,1000,300,1.0,-1.0,2/    # -1.0°C offset, 2 decimals, 5 min
SR,scaled,shared,6,6,0,0,1000,300,1.8,32.0,1/        # Convert to Fahrenheit: F = C×1.8+32

# High precision sensor (6 decimals, uses P_LONG32)
SR,PrecisionTemp,cpu,6,6,0,0,1000,300,1.0,0,6/       # 6 decimal places
```

### Measurement Reusability
Multiple sensor reporters can use the same measurement source, allowing different reporting schedules and calibrations without duplicate hardware readings:

```
MS,0,cpu_temp/                                         # One raw measurement (type 0 = CPU temp)
SR,sensor_fast,cpu_temp,6,6,0,0,500,60/               # V_TEMP, S_TEMP, defaults, report every minute
SR,sensor_slow,cpu_temp,6,6,0,0,1000,600,1.0,2.5,1/   # V_TEMP, S_TEMP, +2.5°C offset, 1 decimal, 10 min
SR,sensor_avg,cpu_temp,6,6,0,1,2000,300,1.0,-1.0,0/   # V_TEMP, S_TEMP, -1.0°C, no decimals, 5-min average
```

Each sensor can apply its own scale, offset, and precision to the raw measurement, enabling flexible use cases like:
- Raw reading for debugging (scale=1.0, offset=0, precision=0)
- Calibrated reading for home automation (scale=1.0, offset=-2.1, precision=1)
- Unit conversion (scale=1.8, offset=32 for Celsius to Fahrenheit)
- Different calibrations for different zones or accuracy requirements

**Important:** MS lines must appear **before** SR lines that reference them.
