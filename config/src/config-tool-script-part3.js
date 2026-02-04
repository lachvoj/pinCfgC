// Part 3: Helper functions and UI utilities

function createFormField(label, type, value, onChange, options = {}) {
    const formGroup = document.createElement('div');
    formGroup.className = 'form-group';
    
    const labelEl = document.createElement('label');
    labelEl.textContent = label;
    
    const input = document.createElement('input');
    input.type = type;
    input.className = type === 'number' ? 'number-input' : 'text-input';
    input.value = value;
    
    if (options.min !== undefined) input.min = options.min;
    if (options.max !== undefined) input.max = options.max;
    if (options.placeholder) input.placeholder = options.placeholder;
    
    // Error message element
    const errorMsg = document.createElement('span');
    errorMsg.className = 'error-text';
    errorMsg.style.color = '#f44336';
    errorMsg.style.fontSize = '0.85em';
    errorMsg.style.display = 'none';
    errorMsg.style.marginTop = '4px';
    
    // Validation function for number inputs
    const validateInput = function() {
        if (type === 'number' && (options.min !== undefined || options.max !== undefined || options.integer || options.maxDecimals !== undefined || options.notZero)) {
            const numValue = parseFloat(input.value);
            
            if (isNaN(numValue)) {
                input.style.border = '2px solid #f44336';
                errorMsg.textContent = 'Invalid number';
                errorMsg.style.display = 'block';
                return false;
            }
            
            // Check for non-zero constraint
            if (options.notZero && numValue === 0) {
                input.style.border = '2px solid #f44336';
                errorMsg.textContent = 'Cannot be 0 (would zero out measurement)';
                errorMsg.style.display = 'block';
                return false;
            }
            
            // Check for integer constraint
            if (options.integer && !Number.isInteger(numValue)) {
                input.style.border = '2px solid #f44336';
                errorMsg.textContent = 'Must be an integer (no decimals)';
                errorMsg.style.display = 'block';
                return false;
            }
            
            // Check for max decimal places
            if (options.maxDecimals !== undefined) {
                const decimalPart = input.value.split('.')[1];
                if (decimalPart && decimalPart.length > options.maxDecimals) {
                    input.style.border = '2px solid #f44336';
                    errorMsg.textContent = `Maximum ${options.maxDecimals} decimal places`;
                    errorMsg.style.display = 'block';
                    return false;
                }
            }
            
            if (options.min !== undefined && numValue < options.min) {
                input.style.border = '2px solid #f44336';
                errorMsg.textContent = `Minimum value: ${options.min}`;
                errorMsg.style.display = 'block';
                return false;
            }
            
            if (options.max !== undefined && numValue > options.max) {
                input.style.border = '2px solid #f44336';
                errorMsg.textContent = `Maximum value: ${options.max}`;
                errorMsg.style.display = 'block';
                return false;
            }
            
            // Valid
            input.style.border = '';
            errorMsg.style.display = 'none';
            return true;
        }
        return true;
    };
    
    // Validate on input (real-time)
    input.addEventListener('input', validateInput);
    
    // Validate and save on change
    input.addEventListener('change', function() {
        if (validateInput()) {
            onChange(this.value);
        }
    });
    
    // Validate on blur (when leaving field)
    input.addEventListener('blur', validateInput);
    
    formGroup.appendChild(labelEl);
    formGroup.appendChild(input);
    formGroup.appendChild(errorMsg);
    
    if (options.helper) {
        const helper = document.createElement('span');
        helper.className = 'helper-text';
        helper.textContent = options.helper;
        formGroup.appendChild(helper);
    }
    
    return formGroup;
}

function createSelectField(label, value, options, onChange) {
    const formGroup = document.createElement('div');
    formGroup.className = 'form-group';
    
    const labelEl = document.createElement('label');
    labelEl.textContent = label;
    
    const select = document.createElement('select');
    select.className = 'select-input';
    
    const emptyOption = document.createElement('option');
    emptyOption.value = '';
    emptyOption.textContent = 'Select...';
    select.appendChild(emptyOption);
    
    options.forEach(opt => {
        const option = document.createElement('option');
        option.value = opt.value;
        option.textContent = opt.label;
        option.selected = opt.value === value;
        select.appendChild(option);
    });
    
    select.addEventListener('change', function() {
        onChange(this.value);
    });
    
    formGroup.appendChild(labelEl);
    formGroup.appendChild(select);
    
    return formGroup;
}

function saveToLocalStorage() {
    try {
        localStorage.setItem('pinCfgC_config', JSON.stringify(configState));
    } catch (e) {
        console.error('Failed to save to localStorage:', e);
    }
}

function loadFromLocalStorage() {
    try {
        const saved = localStorage.getItem('pinCfgC_config');
        if (saved) {
            const loaded = JSON.parse(saved);
            
            // Merge loaded state, handling both old and new formats
            if (loaded.global) {
                Object.keys(loaded.global).forEach(key => {
                    if (typeof loaded.global[key] === 'object' && loaded.global[key].value !== undefined) {
                        // New format with enabled flag
                        configState.global[key] = loaded.global[key];
                    } else {
                        // Old format (just value string)
                        configState.global[key] = {
                            value: loaded.global[key] || configState.global[key].value,
                            enabled: false
                        };
                    }
                });
            }
            
            configState.authPassword = loaded.authPassword || '';
            configState.authPasswordHash = loaded.authPasswordHash || '';
            configState.switches = loaded.switches || [];
            configState.inputs = loaded.inputs || [];
            configState.triggers = loaded.triggers || [];
            configState.measurementSources = loaded.measurementSources || [];
            configState.sensorReporters = loaded.sensorReporters || [];
            
            // Update auth password UI
            const passwordInput = document.getElementById('authPassword');
            const hashInput = document.getElementById('authPasswordHash');
            if (passwordInput) passwordInput.value = configState.authPassword;
            if (hashInput) hashInput.value = configState.authPasswordHash;
            
            // Update global config UI
            Object.keys(configState.global).forEach(key => {
                const input = document.getElementById(`global_${key}`);
                const checkbox = document.getElementById(`global_${key}_enabled`);
                if (input) {
                    input.value = configState.global[key].value;
                    input.disabled = !configState.global[key].enabled;
                }
                if (checkbox) {
                    checkbox.checked = configState.global[key].enabled;
                }
            });
            
            // Render all saved items
            configState.switches.forEach(sw => renderSwitch(sw));
            configState.inputs.forEach(inp => renderInput(inp));
            configState.triggers.forEach(tr => renderTrigger(tr));
            configState.measurementSources.forEach(ms => renderMeasurementSource(ms));
            configState.sensorReporters.forEach(sr => renderSensorReporter(sr));
        }
    } catch (e) {
        console.error('Failed to load from localStorage:', e);
    }
}

function saveConfiguration() {
    // Generate default filename with current date
    const now = new Date();
    const dateStr = now.toISOString().slice(0, 10); // YYYY-MM-DD
    const defaultFilename = `pincfg-config-${dateStr}.json`;
    
    // Prompt user for filename
    const filename = prompt('Enter filename:', defaultFilename);
    if (!filename) return; // User cancelled
    
    // Ensure .json extension
    const finalFilename = filename.endsWith('.json') ? filename : filename + '.json';
    
    const config = JSON.stringify(configState, null, 2);
    const blob = new Blob([config], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    
    const a = document.createElement('a');
    a.href = url;
    a.download = finalFilename;
    a.click();
    
    URL.revokeObjectURL(url);
}

function loadConfiguration() {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = 'application/json';
    
    input.addEventListener('change', function(e) {
        const file = e.target.files[0];
        if (!file) return;
        
        const reader = new FileReader();
        reader.onload = function(event) {
            try {
                const loaded = JSON.parse(event.target.result);
                
                // Clear existing
                clearAll(true);
                
                // Load new config
                Object.assign(configState, loaded);
                
                // Update UI
                const passwordInput = document.getElementById('authPassword');
                const hashInput = document.getElementById('authPasswordHash');
                if (passwordInput) passwordInput.value = configState.authPassword || '';
                if (hashInput) hashInput.value = configState.authPasswordHash || '';
                
                Object.keys(configState.global).forEach(key => {
                    const input = document.getElementById(`global_${key}`);
                    const checkbox = document.getElementById(`global_${key}_enabled`);
                    if (input) {
                        input.value = configState.global[key].value || '';
                        input.disabled = !configState.global[key].enabled;
                    }
                    if (checkbox) {
                        checkbox.checked = configState.global[key].enabled;
                    }
                });
                
                configState.switches.forEach(sw => renderSwitch(sw));
                configState.inputs.forEach(inp => renderInput(inp));
                configState.triggers.forEach(tr => renderTrigger(tr));
                configState.measurementSources.forEach(ms => renderMeasurementSource(ms));
                configState.sensorReporters.forEach(sr => renderSensorReporter(sr));
                
                saveToLocalStorage();
                alert('Configuration loaded successfully!');
            } catch (err) {
                alert('Failed to load configuration: ' + err.message);
            }
        };
        reader.readAsText(file);
    });
    
    input.click();
}

function parseAndLoadConfiguration() {
    const textarea = document.getElementById('fullOutput');
    const configText = textarea.value.trim();
    
    if (!configText) {
        alert('Please enter or paste configuration text first.');
        return;
    }
    
    if (!confirm('This will replace your current configuration. Continue?')) {
        return;
    }
    
    try {
        parseCSVConfiguration(configText);
        updateSectionVisibility();
        alert('Configuration parsed and loaded successfully!');
    } catch (err) {
        alert('Failed to parse configuration: ' + err.message);
    }
}

function parseCSVConfiguration(csv) {
    // Clear existing
    clearAll(true);
    
    // Remove opening/closing markers
    csv = csv.replace(/^#\[/, '').replace(/\]#$/, '');
    
    // Check for PWD command (expects SHA256 hash - 64 hex chars)
    const pwdMatch = csv.match(/^PWD:([^/]+)\//);
    if (pwdMatch) {
        const hashValue = pwdMatch[1];
        // Check if it's a valid SHA256 hash (64 hex characters)
        if (/^[a-f0-9]{64}$/i.test(hashValue)) {
            configState.authPasswordHash = hashValue.toLowerCase();
            configState.authPassword = ''; // Can't reverse hash
            const hashInput = document.getElementById('authPasswordHash');
            if (hashInput) hashInput.value = configState.authPasswordHash;
        } else {
            // Legacy: might be plain text password, hash it
            configState.authPassword = hashValue;
            sha256(hashValue).then(hash => {
                configState.authPasswordHash = hash;
                const hashInput = document.getElementById('authPasswordHash');
                if (hashInput) hashInput.value = hash;
            });
            const passwordInput = document.getElementById('authPassword');
            if (passwordInput) passwordInput.value = hashValue;
        }
        // Remove PWD command from csv
        csv = csv.replace(/^PWD:[^/]+\//, '');
    }
    
    // Also check for old AUTH format for backward compatibility
    const authMatch = csv.match(/^AUTH:([^/]+)\//);
    if (authMatch) {
        const hashValue = authMatch[1];
        if (/^[a-f0-9]{64}$/i.test(hashValue)) {
            configState.authPasswordHash = hashValue.toLowerCase();
            configState.authPassword = '';
            const hashInput = document.getElementById('authPasswordHash');
            if (hashInput) hashInput.value = configState.authPasswordHash;
        } else {
            configState.authPassword = hashValue;
            sha256(hashValue).then(hash => {
                configState.authPasswordHash = hash;
                const hashInput = document.getElementById('authPasswordHash');
                if (hashInput) hashInput.value = hash;
            });
            const passwordInput = document.getElementById('authPassword');
            if (passwordInput) passwordInput.value = hashValue;
        }
        csv = csv.replace(/^AUTH:[^/]+\//, '');
    }
    
    // Split by line and remove comments
    const lines = csv.split('\n')
        .map(line => line.trim())
        .filter(line => line && !line.startsWith('#'));
    
    lines.forEach(line => {
        // Remove trailing /
        line = line.replace(/\/$/, '');
        
        const parts = line.split(',').map(p => p.trim());
        if (parts.length === 0) return;
        
        const type = parts[0];
        
        if (type === 'CD' && parts.length === 2) {
            configState.global.CD = { value: parts[1], enabled: true };
        } else if (type === 'CM' && parts.length === 2) {
            configState.global.CM = { value: parts[1], enabled: true };
        } else if (type === 'CR' && parts.length === 2) {
            configState.global.CR = { value: parts[1], enabled: true };
        } else if (type === 'CN' && parts.length === 2) {
            configState.global.CN = { value: parts[1], enabled: true };
        } else if (type === 'CA' && parts.length === 2) {
            configState.global.CA = { value: parts[1], enabled: true };
        } else if (type === 'S' && parts.length > 1) {
            // Basic switches
            for (let i = 1; i < parts.length; i += 2) {
                if (i + 1 < parts.length) {
                    const sw = {
                        id: Date.now() + i,
                        type: 'S',
                        name: parts[i],
                        pin: parts[i + 1]
                    };
                    configState.switches.push(sw);
                    renderSwitch(sw);
                }
            }
        } else if (type === 'SF' && parts.length > 1) {
            // Switches with feedback
            for (let i = 1; i < parts.length; i += 3) {
                if (i + 2 < parts.length) {
                    const sw = {
                        id: Date.now() + i,
                        type: 'SF',
                        name: parts[i],
                        pin: parts[i + 1],
                        feedbackPin: parts[i + 2]
                    };
                    configState.switches.push(sw);
                    renderSwitch(sw);
                }
            }
        } else if (type === 'SI' && parts.length > 1) {
            // Impulse switches
            for (let i = 1; i < parts.length; i += 2) {
                if (i + 1 < parts.length) {
                    const sw = {
                        id: Date.now() + i,
                        type: 'SI',
                        name: parts[i],
                        pin: parts[i + 1]
                    };
                    configState.switches.push(sw);
                    renderSwitch(sw);
                }
            }
        } else if (type === 'SIF' && parts.length > 1) {
            // Impulse switches with feedback
            for (let i = 1; i < parts.length; i += 3) {
                if (i + 2 < parts.length) {
                    const sw = {
                        id: Date.now() + i,
                        type: 'SIF',
                        name: parts[i],
                        pin: parts[i + 1],
                        feedbackPin: parts[i + 2]
                    };
                    configState.switches.push(sw);
                    renderSwitch(sw);
                }
            }
        } else if (type === 'ST' && parts.length > 1) {
            // Timed switches
            for (let i = 1; i < parts.length; i += 3) {
                if (i + 2 < parts.length) {
                    const sw = {
                        id: Date.now() + i,
                        type: 'ST',
                        name: parts[i],
                        pin: parts[i + 1],
                        duration: parts[i + 2]
                    };
                    configState.switches.push(sw);
                    renderSwitch(sw);
                }
            }
        } else if (type === 'STF' && parts.length > 1) {
            // Timed switches with feedback
            for (let i = 1; i < parts.length; i += 4) {
                if (i + 3 < parts.length) {
                    const sw = {
                        id: Date.now() + i,
                        type: 'STF',
                        name: parts[i],
                        pin: parts[i + 1],
                        duration: parts[i + 2],
                        feedbackPin: parts[i + 3]
                    };
                    configState.switches.push(sw);
                    renderSwitch(sw);
                }
            }
        } else if (type === 'I' && parts.length > 1) {
            // Inputs
            for (let i = 1; i < parts.length; i += 2) {
                if (i + 1 < parts.length) {
                    const inp = {
                        id: Date.now() + i,
                        name: parts[i],
                        pin: parts[i + 1]
                    };
                    configState.inputs.push(inp);
                    renderInput(inp);
                }
            }
        } else if (type === 'T' && parts.length >= 6) {
            // Triggers
            const tr = {
                id: Date.now() + Math.random(),
                name: parts[1],
                eventSource: parts[2],  // Can be input or sensor name
                eventType: parts[3],
                eventData: parts[4],     // Decimal for value-based, integer for multiclick
                actions: []
            };
            
            // Backward compatibility: also set inputName
            tr.inputName = parts[2];
            
            for (let i = 5; i < parts.length; i += 2) {
                if (i + 1 < parts.length) {
                    tr.actions.push({
                        switchName: parts[i],
                        action: parts[i + 1]
                    });
                }
            }
            
            configState.triggers.push(tr);
            renderTrigger(tr);
        } else if (type === 'MS' && parts.length >= 3) {
            // Measurement sources
            const msType = parts[1];
            const ms = {
                id: Date.now() + Math.random(),
                type: msType,
                name: parts[2]
            };
            
            if (msType === '1' && parts.length >= 4) { // Analog
                ms.pin = parts[3];
            } else if (msType === '3') { // I2C
                if (parts.length >= 5) {
                    ms.i2cAddr = parts[3];
                    ms.register = parts[4];
                    ms.dataSize = parts[5] || '2';
                    ms.cache = parts[6] || '';      // Index 6 - cache time in ms (optional, empty = default 100ms)
                    ms.cmd2 = parts[7] || '';
                    ms.cmd3 = parts[8] || '';
                }
            } else if (msType === '4') { // SPI
                if (parts.length >= 6) {
                    ms.spiCs = parts[3];
                    ms.spiCmd = parts[4] || '';
                    ms.spiDataSize = parts[5] || '2';
                    ms.spiDelay = parts[6] || '0';
                }
            }
            // AHT10/AHT20 (type 7) - no extra parameters needed
            
            configState.measurementSources.push(ms);
            renderMeasurementSource(ms);
        } else if (type === 'SR' && parts.length >= 9) {
            // Sensor reporters - minimum 9 fields, optional scale/offset/precision/unit/byteOffset/byteCount/bitShift/bitMask/endianness
            const sr = {
                id: Date.now() + Math.random(),
                name: parts[1],
                measurementName: parts[2],
                vType: parts[3],
                sType: parts[4],
                enableable: parts[5] === '1' || parts[5] === 'true' || parts[5] === true,
                cumulative: parts[6] === '1' || parts[6] === 'true' || parts[6] === true,
                samplingInterval: parts[7],
                reportingInterval: parts[8],
                scale: parts[9] || '1.0',      // Index 9
                offset: parts[10] || '0.0',    // Index 10
                precision: parts[11] || '0',   // Index 11
                unit: parts[12] || '',         // Index 12
                byteOffset: parts[13] || '0',  // Index 13
                byteCount: parts[14] || '0',   // Index 14
                bitShift: parts[15] || '0',    // Index 15
                bitMask: parts[16] || '',      // Index 16
                endianness: parts[17] || '0'   // Index 17
            };
            
            configState.sensorReporters.push(sr);
            renderSensorReporter(sr);
        }
    });
    
    // Update global config UI
    Object.keys(configState.global).forEach(key => {
        const input = document.getElementById(`global_${key}`);
        const checkbox = document.getElementById(`global_${key}_enabled`);
        if (input) {
            input.value = configState.global[key].value || '';
            input.disabled = !configState.global[key].enabled;
        }
        if (checkbox) {
            checkbox.checked = configState.global[key].enabled;
        }
    });
    
    saveToLocalStorage();
}

function clearAll(silent = false) {
    if (!silent && !confirm('Are you sure you want to clear all configuration?')) {
        return;
    }
    
    configState.switches = [];
    configState.inputs = [];
    configState.triggers = [];
    configState.measurementSources = [];
    configState.sensorReporters = [];
    
    document.getElementById('switchesList').innerHTML = '';
    document.getElementById('inputsList').innerHTML = '';
    document.getElementById('triggersList').innerHTML = '';
    document.getElementById('msList').innerHTML = '';
    document.getElementById('srList').innerHTML = '';
    
    document.getElementById('fullOutput').value = '';
    document.getElementById('linesOutput').innerHTML = '';
    document.getElementById('validationResults').style.display = 'none';
    
    saveToLocalStorage();
}
