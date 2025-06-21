# pinCfgC

The goal of this library is to provide a CSV-based configuration that specifies how IO pins on the MySensors platform will behave.

## Table of Contents
1. [Format Overview](#format-overview)
2. [Comments](#comments)
3. [Special Characters](#special-characters)
4. [Global Configuration Items](#global-configuration-items)
5. [Switches](#switches)
   1. [Basic Switch](#basic-switch)
   2. [Impulse Output Switch](#impulse-output-switch)
   3. [Timed Output Switch](#timed-output-switch)
   4. [Switch with Feedback](#switch-with-feedback)
   5. [Impulse Output Switch with Feedback](#impulse-output-switch-with-feedback)
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
CA,1966080/
```
Lines starting with **'C'** are used for defining global parameters:
* **CD**: Debounce interval in milliseconds. Used by inputs to define the period during which input changes are ignored. Default: 100 ms or `PINCFG_DEBOUNCE_MS_D`.
* **CM**: Multiclick maximum delay in milliseconds. Maximum time to recognize a multiclick as separate from single clicks. Used by inputs. Default: 500 ms or `PINCFG_MULTICLICK_MAX_DELAY_MS_D`.
* **CR**: Impulse duration in milliseconds. Global constant used by switches to define how long a switch stays ON in impulse mode. Default: 300 ms or `PINCFG_SWITCH_IMPULSE_DURATIN_MS_D`.
* **CN**: Feedback pin ON delay in milliseconds. Global constant used by switches to specify the maximum waiting period for feedback to match the switch state when turned ON. If feedback remains OFF after this period, the switch is set back to OFF and a status message is sent. Default: 1000 ms or `PINCFG_SWITCH_FB_ON_DELAY_MS_D`.
* **CF**: Feedback pin OFF delay in milliseconds. Global constant used by switches to specify the maximum waiting period for feedback to match the switch state when turned OFF. If feedback remains ON after this period, the switch is set back to ON and a status message is sent. Default: 30000 ms or `PINCFG_SWITCH_FB_OFF_DELAY_MS_D`.
* **CA**: Timed action addition time in milliseconds. Global constant used by switches to set or prolong the ON state impulse when a trigger with a timed action is used. Default: 1966080 ms or `PINCFG_TIMED_ACTION_ADDITION_MS_D`.

## Switches
Switches are abstract configuration units that specify how output pins will behave.
Lines starting with **'S'** are parsed as switch definitions.
Each line adds to the existing configuration, and it is possible to specify multiple switches of the same variant in one line.

### Basic Switch
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

### Switch with Feedback
A [Basic Switch](#basic-switch) with state reporting logic driven by a feedback pin.
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
It has one mandatory parameter, **Name** (`char[]`), and two optional parameters:  
**Report interval** (`uint32_t`), default 300000 ms (5 min) or `PINCFG_CPUTEMP_REPORTING_INTV_MS_D`, and **Offset** (`float`), default 0.0 or `PINCFG_CPUTEMP_OFFSET_D`.
#### Line Format
```
1. TI,      (temperature sensor type definition)
2. CPUTemp, (name)
3. 300000,  (optional reporting interval)
4. 0.0/     (optional offset)
```
#### Examples
```TI,CPUTemp/```  
```TI,CPUTemp,300000/```  
```TI,CPUTemp,300000,-2.1/```
