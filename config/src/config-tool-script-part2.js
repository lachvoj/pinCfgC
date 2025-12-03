// Part 2: Triggers, Measurement Sources, and Sensor Reporters

function addTrigger() {
    const triggerObj = {
        id: Date.now(),
        name: `t${String(configState.triggers.length + 1).padStart(2, '0')}`,
        inputName: '',
        eventType: '1',
        eventCount: '1',
        actions: []
    };
    
    configState.triggers.push(triggerObj);
    renderTrigger(triggerObj);
    expandSection('triggersHeader', 'triggersContent');
    saveToLocalStorage();
}

function renderTrigger(triggerObj) {
    const container = document.getElementById('triggersList');
    
    const card = document.createElement('div');
    card.className = 'item-card';
    card.dataset.id = triggerObj.id;
    
    const header = document.createElement('div');
    header.className = 'item-header';
    
    const title = document.createElement('span');
    title.className = 'item-title';
    title.textContent = `Trigger - ${triggerObj.name}`;
    
    const actions = document.createElement('div');
    actions.className = 'item-actions';
    
    const removeBtn = document.createElement('button');
    removeBtn.className = 'btn-small btn-remove';
    removeBtn.textContent = '✕ Remove';
    removeBtn.addEventListener('click', () => removeTrigger(triggerObj.id));
    
    actions.appendChild(removeBtn);
    header.appendChild(title);
    header.appendChild(actions);
    
    const fields = document.createElement('div');
    fields.className = 'item-fields';
    
    fields.appendChild(createFormField('Name', 'text', triggerObj.name, (value) => {
        triggerObj.name = value;
        title.textContent = `Trigger - ${triggerObj.name}`;
        saveToLocalStorage();
    }));
    
    // Input name select
    const inputSelect = createSelectField('Input', triggerObj.inputName, 
        configState.inputs.map(i => ({ value: i.name, label: i.name })),
        (value) => {
            triggerObj.inputName = value;
            saveToLocalStorage();
        });
    fields.appendChild(inputSelect);
    
    // Event type select
    const eventOptions = Object.entries(INPUT_EVENT_TYPES).map(([k, v]) => ({ value: k, label: `${k} - ${v}` }));
    const eventSelect = createSelectField('Event Type', triggerObj.eventType, eventOptions, (value) => {
        triggerObj.eventType = value;
        saveToLocalStorage();
    });
    fields.appendChild(eventSelect);
    
    fields.appendChild(createFormField('Event Count', 'number', triggerObj.eventCount, (value) => {
        triggerObj.eventCount = value;
        saveToLocalStorage();
    }, { min: 0, max: 10 }));
    
    card.appendChild(header);
    card.appendChild(fields);
    
    // Actions section
    const actionsDiv = document.createElement('div');
    actionsDiv.className = 'trigger-actions';
    
    const actionsTitle = document.createElement('h4');
    actionsTitle.textContent = 'Switch Actions';
    actionsDiv.appendChild(actionsTitle);
    
    const actionsList = document.createElement('div');
    actionsList.id = `actions_${triggerObj.id}`;
    
    triggerObj.actions.forEach((action, idx) => {
        actionsList.appendChild(renderTriggerAction(triggerObj, action, idx));
    });
    
    actionsDiv.appendChild(actionsList);
    
    const addActionBtn = document.createElement('button');
    addActionBtn.className = 'btn-small btn-add-action';
    addActionBtn.textContent = '+ Add Action';
    addActionBtn.addEventListener('click', () => {
        const maxActions = PINCFG_LIMITS.TRIGGER_MAX_SWITCHES || 5;
        if (triggerObj.actions.length >= maxActions) {
            alert(`Maximum ${maxActions} actions per trigger (PINCFG_TRIGGER_MAX_SWITCHES_D)`);
            return;
        }
        triggerObj.actions.push({ switchName: '', action: '0' });
        const newAction = renderTriggerAction(triggerObj, triggerObj.actions[triggerObj.actions.length - 1], triggerObj.actions.length - 1);
        actionsList.appendChild(newAction);
        saveToLocalStorage();
    });
    actionsDiv.appendChild(addActionBtn);
    
    card.appendChild(actionsDiv);
    container.appendChild(card);
}

function renderTriggerAction(triggerObj, action, index) {
    const actionItem = document.createElement('div');
    actionItem.className = 'action-item';
    
    // Switch select
    const switchSelect = document.createElement('select');
    switchSelect.className = 'select-input';
    switchSelect.style.flex = '1';
    
    const emptyOption = document.createElement('option');
    emptyOption.value = '';
    emptyOption.textContent = 'Select Switch...';
    switchSelect.appendChild(emptyOption);
    
    configState.switches.forEach(sw => {
        const option = document.createElement('option');
        option.value = sw.name;
        option.textContent = sw.name;
        option.selected = sw.name === action.switchName;
        switchSelect.appendChild(option);
    });
    
    switchSelect.addEventListener('change', function() {
        action.switchName = this.value;
        saveToLocalStorage();
    });
    
    // Action type select
    const actionSelect = document.createElement('select');
    actionSelect.className = 'select-input';
    actionSelect.style.flex = '1';
    
    Object.entries(SWITCH_ACTIONS).forEach(([k, v]) => {
        const option = document.createElement('option');
        option.value = k;
        option.textContent = `${k} - ${v}`;
        option.selected = k === action.action;
        actionSelect.appendChild(option);
    });
    
    actionSelect.addEventListener('change', function() {
        action.action = this.value;
        saveToLocalStorage();
    });
    
    const removeBtn = document.createElement('button');
    removeBtn.className = 'btn-small btn-remove';
    removeBtn.textContent = '✕';
    removeBtn.addEventListener('click', () => {
        triggerObj.actions.splice(index, 1);
        actionItem.remove();
        saveToLocalStorage();
    });
    
    actionItem.appendChild(switchSelect);
    actionItem.appendChild(actionSelect);
    actionItem.appendChild(removeBtn);
    
    return actionItem;
}

function removeTrigger(id) {
    const index = configState.triggers.findIndex(t => t.id === id);
    if (index !== -1) {
        configState.triggers.splice(index, 1);
        document.querySelector(`#triggersList [data-id="${id}"]`).remove();
        saveToLocalStorage();
    }
}

function addMeasurementSource() {
    const type = document.getElementById('msType').value;
    const msObj = {
        id: Date.now(),
        type: type,
        name: `ms${configState.measurementSources.length + 1}`,
        pin: type === '1' ? '' : undefined,
        i2cAddr: type === '3' ? '' : undefined,
        register: type === '3' ? '' : undefined,
        dataSize: type === '3' ? '2' : undefined,
        cmd2: type === '3' ? '' : undefined,
        cmd3: type === '3' ? '' : undefined,
        spiCs: type === '4' ? '' : undefined,
        spiCmd: type === '4' ? '' : undefined,
        spiDataSize: type === '4' ? '2' : undefined,
        spiDelay: type === '4' ? '0' : undefined
    };
    
    configState.measurementSources.push(msObj);
    renderMeasurementSource(msObj);
    expandSection('msHeader', 'msContent');
    saveToLocalStorage();
}

function renderMeasurementSource(msObj) {
    const container = document.getElementById('msList');
    
    const card = document.createElement('div');
    card.className = 'item-card';
    card.dataset.id = msObj.id;
    
    const header = document.createElement('div');
    header.className = 'item-header';
    
    const title = document.createElement('span');
    title.className = 'item-title';
    title.textContent = `${MEASUREMENT_TYPES[msObj.type]} - ${msObj.name}`;
    
    const actions = document.createElement('div');
    actions.className = 'item-actions';
    
    const removeBtn = document.createElement('button');
    removeBtn.className = 'btn-small btn-remove';
    removeBtn.textContent = '✕ Remove';
    removeBtn.addEventListener('click', () => removeMeasurementSource(msObj.id));
    
    actions.appendChild(removeBtn);
    header.appendChild(title);
    header.appendChild(actions);
    
    const fields = document.createElement('div');
    fields.className = 'item-fields';
    
    fields.appendChild(createFormField('Name', 'text', msObj.name, (value) => {
        msObj.name = value;
        title.textContent = `${MEASUREMENT_TYPES[msObj.type]} - ${msObj.name}`;
        updateAllMeasurementSourceDropdowns();
        saveToLocalStorage();
    }));
    
    // Type-specific fields
    if (msObj.type === '1') { // Analog
        fields.appendChild(createFormField('Pin', 'number', msObj.pin, (value) => {
            msObj.pin = value;
            saveToLocalStorage();
        }, { min: 0, max: 255 }));
    } else if (msObj.type === '3') { // I2C
        fields.appendChild(createFormField('I2C Address (hex)', 'text', msObj.i2cAddr, (value) => {
            msObj.i2cAddr = value;
            saveToLocalStorage();
        }));
        fields.appendChild(createFormField('Register/Cmd1', 'text', msObj.register, (value) => {
            msObj.register = value;
            saveToLocalStorage();
        }));
        fields.appendChild(createFormField('Data Size (bytes)', 'number', msObj.dataSize, (value) => {
            msObj.dataSize = value;
            saveToLocalStorage();
        }, { min: 1, max: 6 }));
        fields.appendChild(createFormField('Cmd2 (optional)', 'text', msObj.cmd2, (value) => {
            msObj.cmd2 = value;
            saveToLocalStorage();
        }));
        fields.appendChild(createFormField('Cmd3 (optional)', 'text', msObj.cmd3, (value) => {
            msObj.cmd3 = value;
            saveToLocalStorage();
        }));
    } else if (msObj.type === '4') { // SPI
        fields.appendChild(createFormField('CS Pin', 'number', msObj.spiCs, (value) => {
            msObj.spiCs = value;
            saveToLocalStorage();
        }, { min: 0, max: 255 }));
        fields.appendChild(createFormField('Command (optional)', 'text', msObj.spiCmd, (value) => {
            msObj.spiCmd = value;
            saveToLocalStorage();
        }));
        fields.appendChild(createFormField('Data Size (bytes)', 'number', msObj.spiDataSize, (value) => {
            msObj.spiDataSize = value;
            saveToLocalStorage();
        }, { min: 1, max: 8 }));
        fields.appendChild(createFormField('Delay (ms)', 'number', msObj.spiDelay, (value) => {
            msObj.spiDelay = value;
            saveToLocalStorage();
        }, { min: 0, max: 1000 }));
    }
    
    card.appendChild(header);
    card.appendChild(fields);
    container.appendChild(card);
}

function removeMeasurementSource(id) {
    const index = configState.measurementSources.findIndex(ms => ms.id === id);
    if (index !== -1) {
        configState.measurementSources.splice(index, 1);
        document.querySelector(`#msList [data-id="${id}"]`).remove();
        saveToLocalStorage();
    }
}

function addSensorReporter() {
    const srObj = {
        id: Date.now(),
        name: `sensor${configState.sensorReporters.length + 1}`,
        measurementName: '',
        vType: '6',
        sType: '6',
        enableable: false,
        cumulative: false,
        samplingInterval: String(PINCFG_LIMITS.SENSOR_SAMPLING_INTV_MS || 1000),
        reportingInterval: String(PINCFG_LIMITS.SENSOR_REPORTING_INTV_SEC || 300),
        scale: String(PINCFG_LIMITS.SENSOR_SCALE || 1.0),
        offset: String(PINCFG_LIMITS.SENSOR_OFFSET || 0.0),
        precision: String(PINCFG_LIMITS.SENSOR_PRECISION_DEFAULT || 0),
        byteOffset: '0',
        byteCount: '0'
    };
    
    configState.sensorReporters.push(srObj);
    renderSensorReporter(srObj);
    expandSection('srHeader', 'srContent');
    saveToLocalStorage();
}

function renderSensorReporter(srObj) {
    const container = document.getElementById('srList');
    
    const card = document.createElement('div');
    card.className = 'item-card';
    card.dataset.id = srObj.id;
    
    const header = document.createElement('div');
    header.className = 'item-header';
    
    const title = document.createElement('span');
    title.className = 'item-title';
    title.textContent = `Sensor - ${srObj.name}`;
    
    const actions = document.createElement('div');
    actions.className = 'item-actions';
    
    const removeBtn = document.createElement('button');
    removeBtn.className = 'btn-small btn-remove';
    removeBtn.textContent = '✕ Remove';
    removeBtn.addEventListener('click', () => removeSensorReporter(srObj.id));
    
    actions.appendChild(removeBtn);
    header.appendChild(title);
    header.appendChild(actions);
    
    const fields = document.createElement('div');
    fields.className = 'item-fields';
    
    // Group 1: Name and Measurement Source
    const group1 = document.createElement('fieldset');
    group1.className = 'sensor-field-group';
    const legend1 = document.createElement('legend');
    legend1.textContent = 'Basic Info';
    group1.appendChild(legend1);
    
    const nameField = createFormField('Name', 'text', srObj.name, (value) => {
        srObj.name = value;
        title.textContent = `Sensor - ${srObj.name}`;
        saveToLocalStorage();
    });
    const nameLabel = nameField.querySelector('label');
    if (nameLabel) {
        nameLabel.title = 'Unique identifier for this sensor reporter';
        nameLabel.style.cursor = 'help';
    }
    group1.appendChild(nameField);
    
    const msSelect = createSelectField('Measurement Source', srObj.measurementName,
        configState.measurementSources.map(ms => ({ value: ms.name, label: ms.name })),
        (value) => {
            srObj.measurementName = value;
            // Check if selected measurement source is Loop Time type
            const selectedMs = configState.measurementSources.find(ms => ms.name === value);
            const isLoopTime = selectedMs && selectedMs.type === '5';
            
            const cumulativeCheckbox = document.getElementById(`sr_${srObj.id}_cumulative`);
            const samplingGroup = document.getElementById(`sr_${srObj.id}_sampling_group`);
            
            if (isLoopTime) {
                // Loop Time: enable cumulative and disable editing
                srObj.cumulative = true;
                if (cumulativeCheckbox) {
                    cumulativeCheckbox.checked = true;
                    cumulativeCheckbox.disabled = true;
                }
                // Disable sampling input (measured every loop)
                if (samplingGroup) {
                    const samplingInput = samplingGroup.querySelector('input');
                    if (samplingInput) samplingInput.disabled = true;
                }
            } else {
                // Other types: enable cumulative editing
                if (cumulativeCheckbox) {
                    cumulativeCheckbox.disabled = false;
                }
                // Enable/disable sampling based on cumulative state
                if (samplingGroup) {
                    const samplingInput = samplingGroup.querySelector('input');
                    if (samplingInput) samplingInput.disabled = !srObj.cumulative;
                }
            }
            saveToLocalStorage();
        });
    const msSelectLabel = msSelect.querySelector('label');
    if (msSelectLabel) {
        msSelectLabel.title = 'References measurement source (MS) by name';
        msSelectLabel.style.cursor = 'help';
    }
    group1.appendChild(msSelect);
    fields.appendChild(group1);
    
    // Group 2: V_TYPE and S_TYPE
    const group2 = document.createElement('fieldset');
    group2.className = 'sensor-field-group';
    const legend2 = document.createElement('legend');
    legend2.textContent = 'MySensors Types';
    group2.appendChild(legend2);
    
    // V_TYPE to S_TYPE mapping (common pairings)
    const vTypeToSType = {
        '0': '6',   // V_TEMP -> S_TEMP
        '1': '7',   // V_HUM -> S_HUM
        '2': '3',   // V_STATUS -> S_BINARY
        '3': '0',   // V_PERCENTAGE -> S_DOOR
        '4': '8',   // V_PRESSURE -> S_BARO
        '16': '1',  // V_TRIPPED -> S_MOTION
        '23': '16', // V_LIGHT_LEVEL -> S_LIGHT_LEVEL
        '37': '30', // V_LEVEL -> S_MULTIMETER
        '38': '30', // V_VOLTAGE -> S_MULTIMETER
        '39': '30'  // V_CURRENT -> S_MULTIMETER
    };
    
    const vTypeOptions = Object.entries(V_TYPES).map(([k, v]) => ({ value: k, label: `${k} - ${v}` }));
    const sTypeOptions = Object.entries(S_TYPES).map(([k, v]) => ({ value: k, label: `${k} - ${v}` }));
    
    const vTypeSelect = createSelectField('V_TYPE', srObj.vType, vTypeOptions, (value) => {
        srObj.vType = value;
        // Auto-select S_TYPE based on V_TYPE if mapping exists
        if (vTypeToSType[value]) {
            srObj.sType = vTypeToSType[value];
            const sTypeSelectEl = document.getElementById(`sr_${srObj.id}_stype`);
            if (sTypeSelectEl) sTypeSelectEl.value = srObj.sType;
        }
        saveToLocalStorage();
    });
    const vTypeLabel = vTypeSelect.querySelector('label');
    if (vTypeLabel) {
        vTypeLabel.title = 'MySensors variable type (data format)';
        vTypeLabel.style.cursor = 'help';
    }
    group2.appendChild(vTypeSelect);
    
    const sTypeSelect = createSelectField('S_TYPE', srObj.sType, sTypeOptions, (value) => {
        srObj.sType = value;
        saveToLocalStorage();
    });
    const sTypeLabel = sTypeSelect.querySelector('label');
    if (sTypeLabel) {
        sTypeLabel.title = 'MySensors sensor type (device category)';
        sTypeLabel.style.cursor = 'help';
    }
    // Add ID for auto-selection
    const sTypeSelectEl = sTypeSelect.querySelector('select');
    if (sTypeSelectEl) sTypeSelectEl.id = `sr_${srObj.id}_stype`;
    group2.appendChild(sTypeSelect);
    fields.appendChild(group2);
    
    // Group 3: Behavior and Timing
    const group3 = document.createElement('fieldset');
    group3.className = 'sensor-field-group behavior-group';
    const legend3 = document.createElement('legend');
    legend3.textContent = 'Behavior & Timing';
    group3.appendChild(legend3);
    
    // Enableable checkbox
    const enableableGroup = document.createElement('div');
    enableableGroup.className = 'form-group';
    enableableGroup.style.display = 'flex';
    enableableGroup.style.alignItems = 'center';
    enableableGroup.style.gap = '10px';
    
    const enableableCheckbox = document.createElement('input');
    enableableCheckbox.type = 'checkbox';
    enableableCheckbox.id = `sr_${srObj.id}_enableable`;
    enableableCheckbox.checked = srObj.enableable;
    enableableCheckbox.style.width = 'auto';
    enableableCheckbox.style.cursor = 'pointer';
    
    const enableableLabel = document.createElement('label');
    enableableLabel.textContent = 'Enableable';
    enableableLabel.htmlFor = `sr_${srObj.id}_enableable`;
    enableableLabel.style.cursor = 'pointer';
    enableableLabel.title = 'Can be enabled/disabled at runtime (increases memory)';
    
    enableableCheckbox.addEventListener('change', function() {
        srObj.enableable = this.checked;
        saveToLocalStorage();
    });
    
    enableableGroup.appendChild(enableableCheckbox);
    enableableGroup.appendChild(enableableLabel);
    group3.appendChild(enableableGroup);
    
    // Cumulative checkbox
    const cumulativeGroup = document.createElement('div');
    cumulativeGroup.className = 'form-group';
    cumulativeGroup.style.display = 'flex';
    cumulativeGroup.style.alignItems = 'center';
    cumulativeGroup.style.gap = '10px';
    
    const cumulativeCheckbox = document.createElement('input');
    cumulativeCheckbox.type = 'checkbox';
    cumulativeCheckbox.id = `sr_${srObj.id}_cumulative`;
    cumulativeCheckbox.checked = srObj.cumulative;
    cumulativeCheckbox.style.width = 'auto';
    cumulativeCheckbox.style.cursor = 'pointer';
    
    // Check if measurement source is Loop Time type and disable cumulative editing
    const selectedMs = configState.measurementSources.find(ms => ms.name === srObj.measurementName);
    const isLoopTime = selectedMs && selectedMs.type === '5';
    if (isLoopTime) {
        srObj.cumulative = true;
        cumulativeCheckbox.checked = true;
        cumulativeCheckbox.disabled = true;
    }
    
    const cumulativeLabel = document.createElement('label');
    cumulativeLabel.textContent = 'Cumulative';
    cumulativeLabel.htmlFor = `sr_${srObj.id}_cumulative`;
    cumulativeLabel.style.cursor = 'pointer';
    cumulativeLabel.title = 'Sample at intervals and report average (vs. single reading)';
    
    const samplingField = createFormField('Sampling (ms)', 'number', srObj.samplingInterval, (value) => {
        srObj.samplingInterval = value;
        saveToLocalStorage();
    }, { 
        min: PINCFG_LIMITS.SENSOR_SAMPLING_INTV_MIN_MS || 100, 
        max: PINCFG_LIMITS.SENSOR_SAMPLING_INTV_MAX_MS || 5000,
        integer: true
    });
    samplingField.id = `sr_${srObj.id}_sampling_group`;
    const samplingLabel = samplingField.querySelector('label');
    if (samplingLabel) {
        samplingLabel.title = 'How often to read measurement in cumulative mode (100-5000 ms)';
        samplingLabel.style.cursor = 'help';
    }
    
    cumulativeCheckbox.addEventListener('change', function() {
        srObj.cumulative = this.checked;
        // Disable sampling field when cumulative is unchecked (and not Loop Time)
        const samplingInput = samplingField.querySelector('input');
        if (samplingInput && !isLoopTime) {
            samplingInput.disabled = !this.checked;
            if (!this.checked) {
                const minSampling = PINCFG_LIMITS.SENSOR_SAMPLING_INTV_MIN_MS || 100;
                samplingInput.value = minSampling.toString();
                srObj.samplingInterval = minSampling.toString();
            }
        }
        saveToLocalStorage();
    });
    
    cumulativeGroup.appendChild(cumulativeCheckbox);
    cumulativeGroup.appendChild(cumulativeLabel);
    group3.appendChild(cumulativeGroup);
    
    group3.appendChild(samplingField);
    
    const reportingField = createFormField('Reporting (sec)', 'number', srObj.reportingInterval, (value) => {
        srObj.reportingInterval = value;
        saveToLocalStorage();
    }, { 
        min: PINCFG_LIMITS.SENSOR_REPORTING_INTV_MIN_SEC || 1, 
        max: PINCFG_LIMITS.SENSOR_REPORTING_INTV_MAX_SEC || 3600,
        integer: true
    });
    const reportingLabel = reportingField.querySelector('label');
    if (reportingLabel) {
        reportingLabel.title = 'How often to send value to controller (1-3600 seconds)';
        reportingLabel.style.cursor = 'help';
    }
    group3.appendChild(reportingField);
    
    // Set initial state
    const samplingInput = samplingField.querySelector('input');
    if (samplingInput) {
        // For Loop Time, always disable sampling input; otherwise follow cumulative state
        samplingInput.disabled = isLoopTime || !srObj.cumulative;
    }
    fields.appendChild(group3);
    
    // Group 4: Value Transformation
    const transformGroup = document.createElement('fieldset');
    transformGroup.className = 'sensor-field-group';
    
    const legend4 = document.createElement('legend');
    legend4.textContent = 'Value Transformation';
    legend4.title = 'Formula: displayed = (raw × scale) + offset, formatted to N decimal places';
    legend4.style.cursor = 'help';
    transformGroup.appendChild(legend4);
    
    const scaleField = createFormField('Scale', 'number', srObj.scale, (value) => {
        srObj.scale = value;
        saveToLocalStorage();
    }, { 
        min: PINCFG_LIMITS.SENSOR_SCALE_MIN || -1000, 
        max: PINCFG_LIMITS.SENSOR_SCALE_MAX || 1000,
        maxDecimals: 6,
        notZero: true
    });
    const scaleLabel = scaleField.querySelector('label');
    if (scaleLabel) {
        scaleLabel.title = 'Multiplicative factor (e.g., TMP102: 0.0625 for °C/LSB)';
        scaleLabel.style.cursor = 'help';
    }
    transformGroup.appendChild(scaleField);
    
    const offsetField = createFormField('Offset', 'number', srObj.offset, (value) => {
        srObj.offset = value;
        saveToLocalStorage();
    }, { 
        min: PINCFG_LIMITS.SENSOR_OFFSET_MIN || -1000, 
        max: PINCFG_LIMITS.SENSOR_OFFSET_MAX || 1000,
        maxDecimals: 6
    });
    const offsetLabel = offsetField.querySelector('label');
    if (offsetLabel) {
        offsetLabel.title = 'Additive adjustment (e.g., -2.1 for calibration)';
        offsetLabel.style.cursor = 'help';
    }
    transformGroup.appendChild(offsetField);
    
    const precisionField = createFormField('Precision', 'number', srObj.precision, (value) => {
        srObj.precision = value;
        saveToLocalStorage();
    }, { 
        min: PINCFG_LIMITS.SENSOR_PRECISION_MIN || 0, 
        max: PINCFG_LIMITS.SENSOR_PRECISION_MAX || 6,
        integer: true
    });
    const precisionLabel = precisionField.querySelector('label');
    if (precisionLabel) {
        precisionLabel.title = 'Decimal places 0-6 (e.g., 2 for 12.34)';
        precisionLabel.style.cursor = 'help';
    }
    transformGroup.appendChild(precisionField);
    
    fields.appendChild(transformGroup);
    
    // Group 5: Multi-value Sensors
    const group5 = document.createElement('fieldset');
    group5.className = 'sensor-field-group';
    const legend5 = document.createElement('legend');
    legend5.textContent = 'Multi-value Sensors';
    legend5.title = 'For sensors that return multiple values in one measurement';
    legend5.style.cursor = 'help';
    group5.appendChild(legend5);
    
    const byteOffsetField = createFormField('Byte Offset', 'number', srObj.byteOffset, (value) => {
        srObj.byteOffset = value;
        saveToLocalStorage();
    }, { min: 0, max: 5, integer: true });
    const byteOffsetLabel = byteOffsetField.querySelector('label');
    if (byteOffsetLabel) {
        byteOffsetLabel.title = 'Starting byte index for extraction (0-5, default 0)';
        byteOffsetLabel.style.cursor = 'help';
    }
    group5.appendChild(byteOffsetField);
    
    const byteCountField = createFormField('Byte Count', 'number', srObj.byteCount, (value) => {
        srObj.byteCount = value;
        saveToLocalStorage();
    }, { min: 0, max: 6, integer: true });
    const byteCountLabel = byteCountField.querySelector('label');
    if (byteCountLabel) {
        byteCountLabel.title = 'Number of bytes to extract (1-6, 0=all)';
        byteCountLabel.style.cursor = 'help';
    }
    group5.appendChild(byteCountField);
    
    fields.appendChild(group5);
    
    card.appendChild(header);
    card.appendChild(fields);
    container.appendChild(card);
}

function removeSensorReporter(id) {
    const index = configState.sensorReporters.findIndex(sr => sr.id === id);
    if (index !== -1) {
        configState.sensorReporters.splice(index, 1);
        document.querySelector(`#srList [data-id="${id}"]`).remove();
        saveToLocalStorage();
    }
}

function updateAllMeasurementSourceDropdowns() {
    // Update measurement source dropdown options in all sensor reporters
    configState.sensorReporters.forEach(sr => {
        const card = document.querySelector(`#srList [data-id="${sr.id}"]`);
        if (card) {
            // Find all form groups and locate the one with "Measurement Source" label
            const formGroups = card.querySelectorAll('.form-group');
            for (const formGroup of formGroups) {
                const label = formGroup.querySelector('label');
                if (label && label.textContent === 'Measurement Source') {
                    const select = formGroup.querySelector('select');
                    if (select) {
                        const currentValue = select.value;
                        
                        // Clear existing options
                        select.innerHTML = '';
                        
                        // Add empty option
                        const emptyOption = document.createElement('option');
                        emptyOption.value = '';
                        emptyOption.textContent = 'Select...';
                        select.appendChild(emptyOption);
                        
                        // Add updated measurement source options
                        configState.measurementSources.forEach(ms => {
                            const option = document.createElement('option');
                            option.value = ms.name;
                            option.textContent = ms.name;
                            option.selected = ms.name === currentValue;
                            select.appendChild(option);
                        });
                    }
                    break; // Found and updated, move to next sensor reporter
                }
            }
        }
    });
}
