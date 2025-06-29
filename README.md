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
8. [Temperature Sensors](#temperature-sensors)
   1. [CPU Temperature Sensor](#cpu-temperature-sensor)

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
TI,CPUTemp,300000,-2.1/
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

## Temperature Sensors

### CPU Temperature Sensor
This configuration item specifies if the internal CPU temperature sensor value should be reported.
#### Line Format
```
1. TI,      (temperature sensor type definition)
2. CPUTemp, (name)
3. 0,       (optional enableable 0/1)
3. 0,       (optional mode 0 - standard, 1 - cumulative)
4. 100,     (optional sampling interval in ms used only in cumulative mode)
3. 300000,  (optional reporting interval in ms)
4. 0.0/     (optional offset)
```
#### Parameters
1. **Name** (`char[]`) mandatory name parameter
2. **Enableable (optional) default 0**
    * **0** CPU temperature will be allways reported.
    * **1** It add toggle possibility for reporting CPU temperature (this increases runtime memory consumption).
3. **Mode (optional) default 0**
    * **0** (standard) temperature is measured once per report interval and send
    * **1** (cumulative) temperature is measured in sampling intervals and after report interval average value is send
4. **Sampling interval (optional)** (`uint32_t`) default 1000 ms (1s) or `PINCFG_CPUTEMP_SAMPLING_INTV_MS_D`. Valid values 100-5000 (100ms - 1s)
5. **Report interval (optional)** (`uint32_t`), default 300000 ms (5 min) or `PINCFG_CPUTEMP_REPORTING_INTV_MS_D` Valid values 1000-3600000 (1s-1h)
6. **Offset (optional)** (`float`), default 0.0 or `PINCFG_CPUTEMP_OFFSET_D`.

#### Examples
```
TI,CPUTemp,/
TI,CPUTemp,0/
TI,CPUTemp,1,0/
TI,CPUTemp,1,1/
TI,CPUTemp,1,1,1000/
TI,CPUTemp,1,1,1000,300000/
TI,CPUTemp,1,1,1000,300000,-2.1/
```
