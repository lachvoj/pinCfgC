# pinCfgC

Library goal is to have CSV based configuration which specifies how IO pins on MySensors platform will behave.

## Table of Contents
1. [Format overview](#format-overview)
2. [Comments](#comments)
3. [Special characters](#special-characters)
4. [Global configuration items](#global-configuration-items)
5. [Switches](#switches)
    1. [Basic switch](#basic-switch)
    2. [Impulse output switch](#impulse-output-switch)
    3. [Switch with feedback](#switch-with-feedback)
    4. [Impulse output switch with feedback](#impulse-output-switch-with-feedback)
6. [Inputs](#inputs)
7. [Triggers](#triggers)
8. [Temperature sensors](#temperature-sensors)
    1. [CPU temperature sensor](#cpu-temperature-sensor)

## Format overview:
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
Lines starting with **#** will be ignored by parsing function and could be used as comments

## Special characters
* **','** is by default used as value separator. It could be changed by compilation by defining: PINCFG_VALUE_SEPARATOR_D
* **'/'** is by default used as line break. It could be changed by compilation by defining: PINCFG_LINE_SEPARATOR_D

## Global configuration items
```
CD,330/
CM,620/
CR,150/
CN,1000/
CF,30000/
CA,1966080/
```
Lines starting with **'C'** are used for defining global parameters
* **CD** Debounce interval in milliseconds. Is used by Input to obtain time period when input changes are ignored. Default 100(ms) or PINCFG_DEBOUNCE_MS_D.
* **CM** Multiclick maximum delay in milliseconds. Is maximum time to recognize multiclick from separate clicks. It is used by input. Default: 500(ms) or PINCFG_MULTICLICK_MAX_DELAY_MS_D.
* **CR** Impulse duration in milliseconds. Is global constant used by switch to define how long will be switch in ON state, when switch is used in impulse mode. Default: 300(ms) or PINCFG_SWITCH_IMPULSE_DURATIN_MS_D.
* **CN** Feedback pin on delay in milliseconds. Is global constant used by switch to specify maximum waiting period of feedback to achieve same value as switch when switch is turned ON. If feedback has OFF value after this period, switch value is set back to OFF and message with status is send. Default: 1000(ms) or PINCFG_SWITCH_FB_ON_DELAY_MS_D,
* **CF** Feedback pin off delay in milliseconds. Is global constant used by switch to specify maximum waiting period of feedback to achieve same value as switch when switch is turned OFF. If feedback has ON value after this period, switch value is set back to ON and message with status is send. Default: 30000(ms) or PINCFG_SWITCH_FB_OFF_DELAY_MS_D.
* **CA** Timed action addition time in milliseconds. Is global constant used by switch to set/prolong ON state impulse when trigger with timed action is used. Default: 1966080(ms) or PINCFG_TIMED_ACTION_ADDITION_MS_D.

## Switches
Are the abstract configuration unit that specifies how output pin will behave.
Lines starting with **'S'** will be parsed as switch(es) definition.
Every line acts as addition to existing ones, but is possible to specify multiple switches of same variant in one line. 

### Basic switch
Switches to ON or OFF according to state.
#### Line format
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

### Impulse output switch
Switches from OFF to ON and back to OFF for period of time defined by **CR** global constant described in: [Global configuration items](#global-configuration-items).
#### Line format
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

### Switch with feedback
It is [Basic switch](#basic-switch) with state reporting logic driven by feedback pin state.
Feedback pin state behavior is adjusted by **CN/CF** [Global configuration items](#global-configuration-items).
#### Line format
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

### Impulse output switch with feedback
It is [Impulse output switch](#impulse-output-switch) with state reporting logic driven by feedback pin as [Switch with feedback](#switch-with-feedback).
#### Line format
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
Are the configuration item that specifies how input pin will behave.
Are event producer used by **triggers**.
Lines starting with **'I'** will be parsed as input(s) definition.
Every line acts as addition to existing ones, but is possible to specify multiple inputs of in one line. 

 ### Line format
 ```
 1. I,   (starts with I for inputs)
 2. i01, (name of input  1)
 3. 16,  (pin of input  1)
    ...,
 x. ix,  (name of input x)
 y. xx/  (pin of input x)
 ```

### Input evet types
 * **0 - Down** - change from ON to OFF and while OFF state has to last longer than debounce interval specified by **CD** line in [Global configuration items](#global-configuration-items). 
 * **1 - Up** - change from OFF to ON and while ON state has to last longer than debounce interval specified by **CD** line in [Global configuration items](#global-configuration-items).
 * **2 - Longpress** - change from OFF to ON and stay in ON for time period longer than multiclick global constant specified by **CM** line in [Global configuration items](#global-configuration-items).
 * **3 - Multiclick** - series of changes OFF-ON-OFF-ON-OFF while two OFF-ON changes can't last longer than multiclick global constant specified by **CM** line in [Global configuration items](#global-configuration-items).

#### Example
```I,i01,193,i02,192,i03,19,i04,18,i05,194,i06,195,i07,196,i08,197,i09,200,i10,201,i11,30,i12,31/```

## Triggers
Configuration item which can specify how input events are transformed to output actions.
Lines starting with only **'T'** will be parsed as trigger definition.

### Line format
```
1. T,   (starts with one character T for triggers)
2. t01, (name of trigger 1)
3. i01, (name of input to listen to)
4. 1,   (input evet type to listen to 0-down, 1-up, 2-longpress, 3-multi)
5. 1,   (event count - count of actions to trigger switch/switches),
6. o01, (name of driven switch 1)
7. 0,   (switch action 1 0-toggle, 1-turn on, 2-turn off, 3-timed)
   ...,
x. oXX, (name of driven switch x)
y. 0/   (switch action x)
```
### Input event listening
Line element 3 is **name** of **input** of which events trigger will listen to.<br>
Line element 4 is input event [type](#input-evet-types) to listen to.<br>
Line element 5 is number of occurrences of listening event. If event is UP or Multiclick otherwise use 0 (DOWN or longpress).

### Switch action(s) definition
Trigger line elements from 6 are in pairs 1. switch name and 2. switch action
Switch action could be:
 * **0 - toggle** simply toggle state of switch
 * **1 - on** set switch state to ON
 * **2 - off** set switch state to OFF
 * **3 - timed** call switch timed action which means set switch to impulse variant (SI/SIF if not) and if switch is OFF then turn it ON for time period defined by CA line. If switch is already ON impulse period will be prolonged by additional timed action addition time.


## Temperature sensors

### CPU temperature sensor
Are the configuration item that specifies if internal CPU temperature sensor value should be reported.
Has one mandatory paramter **Name** (char[]), two optional parameters 
**Report iterval** (uint32_t) with default 300000(ms)(5min) or PINCFG_CPUTEMP_REPORTING_INTV_MS_D and **Offset** (float) 
default 0.0 or PINCFG_CPUTEMP_OFFSET_D.
#### Line format
```
1. TI,      (temperature sensor type definition)
2. CPUTemp, (name)
3. 300000,  (optional reporting interval)
4. 0.0/     (optional offset)
```
#### Examples
```TI,CPUTemp/```<br>
```TI,CPUTemp,300000/```<br>
```TI,CPUTemp,300000,-2.1/```