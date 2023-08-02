# pinCfgC

Library goal is to have CSV based configuration which specifies how IO pins on MySensors platform will behave.

## Format overview:
```csv
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
```
## Comments
Lines starting with **#** will be ignored by parsing function and could be used as comments

## Special characters
* **','** is by default used as value separator (could be changed by compilation define)
* **'/'** is by default used as line break (could be changed by compilation define)

## Global configuration items
```csv
CD,330/
CM,620/
CR,150/
CN,1000/
CF,30000/
CA,1966080/
```
Lines starting with **'C'** are used for defining global parameters
* **CD** Debounce interval in milliseconds (default: 100ms). Is used by Input to obtain time period when input changes are ignored.
* **CM** Multiclick maximum delay in milliseconds (default: 500ms). Is maximum time to recognize multiclick from separate clicks. It is used by input.
* **CR** Impulse duration in milliseconds (default: 300ms). Is global constant used by switch to define how long will be switch in ON state, when switch is used in impulse mode.
* **CN** Feedback pin on delay in milliseconds (default: 1000ms). Is global constant used by switch to specify maximum waiting period of feedback to achieve same value as switch when switch is turned ON. If feedback has OFF value after this period, switch value is set back to OFF and message with status is send.
* **CF** Feedback pin off delay in milliseconds (default: 30000ms). Is global constant used by switch to specify maximum waiting period of feedback to achieve same value as switch when switch is turned OFF. If feedback has ON value after this period, switch value is set back to ON and message with status is send.
* **CA** Timed action addition time in milliseconds (default: 5000ms). Is global constant used by switch to set/prolong ON state impulse when trigger with timed action is used.

## Switches
Are the abstract configuration unit that specifies how output pin will behave.
Lines starting with **'S'** will be parsed as switch(es) definition.
Every line acts as addition to existing ones, but is possible to specify multiple switches of same variant in one line. 

### Variants
 * **S** - basic switch no feedback pin, **classic mode**.
   * format: ```S,o01(name of switch 1),13(pin of switch 1)...,ox(switch x),xx(pin)```
 * **SI** - no feedback pin handling **impulse mode**, global constant defined by CR line is used.
   * format: ```SI,o01(name of switch 1),13(pin of switch 1)...,ox(switch x),xx(pin)```
 * **SF** - feedback pin handling, global constants defined by CN/CF lines are used, **classic mode**.
   * format: ```SF,o01(name of switch 1),13(pin of switch 1),193(feedback pin of switch 1)...,ox(switch x),xx(pin of switch x),xx(feedback pin of switch x)```
 * **SIF** - feedback pin handling, global constants defined by CN/CF lines are used, **impulse mode**, global constant defined by CR line is used.
   * format: ```SIF,o01(name of switch 1),13(pin of switch 1),193(feedback pin of switch 1)...,ox(switch x),xx(pin of switch x),xx(feedback pin of switch x)```

### Mode
* **Classic** standard mode if the switch is activated then output pin is set to state 1 until switch is active.
* **Impulse** mode means that if the switch is activated or deactivated, output pin will do impulse from 0 to 1 and back to 0 for **Impulse Duration** time period specified by CR global constant line.

### Example
```csv
S,o01,13,o02,12,o03,11,o04,10,o05,3,o06,2,o07,7/
SI,o10,4/
SF,o09,5,193/
SIF,o08,6,192/
```

## Inputs
Are the abstract configuration unit that specifies how input pin will behave.
Are event producer used by **triggers**.
Lines starting with **'I'** will be parsed as input(s) definition.
Every line acts as addition to existing ones, but is possible to specify multiple inputs of in one line. 

 ### Input line format
 ```I,i01(name of input  1),16(pin of input  1)...,ix(input  x),xx(pin)```

### Input evet types
 * **Up** - change from OFF to ON and while ON state has to last longer than debounce interval specified by CD line.
 * **Down** - change from ON to OFF and while OFF state has to last longer than debounce interval specified by CD line. 
 * **Multiclick** - series of changes OFF-ON-OFF-ON-OF while two OFF-ON changes can't be further from each-other in time than multiclick global constant specified by CM line.
 * **Longpress** - change from OFF to ON and stay in ON for time period longer than multiclick global constant specified by CM line.

#### Example
```csv
I,i01,193,i02,192,i03,19,i04,18,i05,194,i06,195,i07,196,i08,197,i09,200,i10,201,i11,30,i12,31/
```

### Triggers
Abstract configuration unit which can specify how input events are transformed to output actions.
Lines starting with **'T'** will be parsed as trigger definition.

### Trigger line format
```
T(starts with T for triggers),
t01(name of trigger 1),
i01(name of input to listen to),
1(input evet type to listen to 0-down, 1-up, 2-longpress, 3-multiclick),
1(event count - count of actions to trigger switch/switches),
o01(name of driven switch 1),0(switch action 1 0-toggle, 1-turn on, 2-turn off, 3-timed),...oxx(name of driven switch x),0(switch action x)
```

### Input event listening
Trigger line element 3 is name of input of which events trigger will listen to.
Line element 4 is input event type to listen to.
Element 5 is number of occurrences of listening event if event is UP or Multiclick otherwise use 0 (DOWN or longpress).

### Switch action(s) definition
Trigger line elements from 6 are in pairs 1. switch name and 2. switch action
Switch action could be:
 * **0-toggle** simply toggle state of switch
 * **1-on** set switch state to ON
 * **2-off** set switch state to OFF
 * **3-timed** call switch timed action which means set switch to impulse mode and if switch is OFF then turn it on for time period defined by CA line. If switch is already ON impulse period will be prolonged by additional timed action addition time.
