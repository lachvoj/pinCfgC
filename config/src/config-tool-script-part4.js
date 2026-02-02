// Part 4: Configuration generation, validation, and output

function generateOutput() {
    const lines = [];
    
    // Global configuration (only if enabled)
    if (configState.global.CD.enabled && configState.global.CD.value) {
        lines.push(`CD,${configState.global.CD.value}/`);
    }
    if (configState.global.CM.enabled && configState.global.CM.value) {
        lines.push(`CM,${configState.global.CM.value}/`);
    }
    if (configState.global.CR.enabled && configState.global.CR.value) {
        lines.push(`CR,${configState.global.CR.value}/`);
    }
    if (configState.global.CN.enabled && configState.global.CN.value) {
        lines.push(`CN,${configState.global.CN.value}/`);
    }
    if (configState.global.CA.enabled && configState.global.CA.value) {
        lines.push(`CA,${configState.global.CA.value}/`);
    }
    
    // Group switches by type
    const switchesByType = {};
    configState.switches.forEach(sw => {
        if (!switchesByType[sw.type]) {
            switchesByType[sw.type] = [];
        }
        switchesByType[sw.type].push(sw);
    });
    
    // Generate switch lines
    Object.keys(switchesByType).forEach(type => {
        const switches = switchesByType[type];
        const parts = [type];
        
        switches.forEach(sw => {
            parts.push(sw.name);
            parts.push(sw.pin);
            if (sw.feedbackPin !== undefined) {
                parts.push(sw.feedbackPin);
            }
            if (sw.duration !== undefined) {
                parts.push(sw.duration);
            }
        });
        
        lines.push(parts.join(',') + '/');
    });
    
    // Generate input lines
    if (configState.inputs.length > 0) {
        const parts = ['I'];
        configState.inputs.forEach(inp => {
            parts.push(inp.name);
            parts.push(inp.pin);
        });
        lines.push(parts.join(',') + '/');
    }
    
    // Generate trigger lines
    configState.triggers.forEach(tr => {
        if (!tr.inputName || tr.actions.length === 0) return;
        
        const parts = ['T', tr.name, tr.inputName, tr.eventType, tr.eventCount];
        tr.actions.forEach(action => {
            if (action.switchName) {
                parts.push(action.switchName);
                parts.push(action.action);
            }
        });
        lines.push(parts.join(',') + '/');
    });
    
    // Generate measurement source lines
    configState.measurementSources.forEach(ms => {
        const parts = ['MS', ms.type, ms.name];
        
        if (ms.type === '1') { // Analog
            if (ms.pin) parts.push(ms.pin);
        } else if (ms.type === '3') { // I2C
            if (ms.i2cAddr) parts.push(ms.i2cAddr);
            if (ms.register) parts.push(ms.register);
            if (ms.dataSize) parts.push(ms.dataSize);
            // Cache parameter (index 6, optional, empty value uses default)
            if (ms.cache !== undefined && ms.cache !== '') {
                parts.push(ms.cache);
            }
            if (ms.cmd2) parts.push(ms.cmd2);
            if (ms.cmd3) parts.push(ms.cmd3);
        } else if (ms.type === '4') { // SPI
            if (ms.spiCs) parts.push(ms.spiCs);
            if (ms.spiCmd) parts.push(ms.spiCmd);
            if (ms.spiDataSize) parts.push(ms.spiDataSize);
            if (ms.spiDelay) parts.push(ms.spiDelay);
        }
        
        lines.push(parts.join(',') + '/');
    });
    
    // Generate sensor reporter lines
    configState.sensorReporters.forEach(sr => {
        if (!sr.measurementName) return;
        
        const parts = [
            'SR',
            sr.name,
            sr.measurementName,
            sr.vType,
            sr.sType,
            sr.enableable ? '1' : '0',
            sr.cumulative ? '1' : '0',
            sr.samplingInterval,
            sr.reportingInterval
        ];
        
        // Optional parameters (indices 9-17): scale, offset, precision, unit, byteOffset, byteCount, bitShift, bitMask, endianness
        const scale = sr.scale || '1.0';
        const offset = sr.offset || '0.0';
        const precision = sr.precision || '0';
        const unit = sr.unit || '';
        const byteOffset = sr.byteOffset || '0';
        const byteCount = sr.byteCount || '0';
        const bitShift = sr.bitShift || '0';
        const bitMask = sr.bitMask || '';
        const endianness = sr.endianness || '0';
        
        // Include optional parameters only if any are non-default
        const scaleNonDefault = scale !== '1.0' && scale !== '1';
        const offsetNonDefault = offset !== '0.0' && offset !== '0';
        const precisionNonDefault = precision !== '0';
        const unitNonDefault = unit !== '';
        const byteOffsetNonDefault = byteOffset !== '0';
        const byteCountNonDefault = byteCount !== '0';
        const bitShiftNonDefault = bitShift !== '0';
        const bitMaskNonDefault = bitMask !== '';
        const endiannessNonDefault = endianness !== '0';
        
        // Build optional params array and trim trailing defaults
        if (scaleNonDefault || offsetNonDefault || precisionNonDefault || unitNonDefault || 
            byteOffsetNonDefault || byteCountNonDefault || bitShiftNonDefault || bitMaskNonDefault || endiannessNonDefault) {
            const optionalParams = [scale, offset, precision, unit, byteOffset, byteCount, bitShift, bitMask, endianness];
            const isNonDefault = [scaleNonDefault, offsetNonDefault, precisionNonDefault, unitNonDefault, 
                                  byteOffsetNonDefault, byteCountNonDefault, bitShiftNonDefault, bitMaskNonDefault, endiannessNonDefault];
            
            // Find the last non-default parameter
            let lastNonDefault = -1;
            for (let i = isNonDefault.length - 1; i >= 0; i--) {
                if (isNonDefault[i]) {
                    lastNonDefault = i;
                    break;
                }
            }
            
            // Add only up to the last non-default parameter
            for (let i = 0; i <= lastNonDefault; i++) {
                parts.push(optionalParams[i]);
            }
        }
        
        lines.push(parts.join(',') + '/');
    });
    
    const fullConfig = lines.join('');
    
    // Format for display (with newlines for readability)
    const displayConfig = lines.join('\n');
    document.getElementById('fullOutput').value = displayConfig;
    
    // Update size indicator (without markers, they're added in 18-char lines)
    updateConfigSize(fullConfig.length);
    
    // Generate 18-character lines (markers added here)
    generate18CharLines(fullConfig);
}

function updateConfigSize(size) {
    const sizeIndicator = document.getElementById('configSize');
    const maxSize = PINCFG_LIMITS.CONFIG_MAX_SZ || 480;
    
    sizeIndicator.textContent = `${size} / ${maxSize} bytes`;
    
    // Remove all classes first
    sizeIndicator.classList.remove('warning', 'error');
    
    // Add appropriate class based on size
    if (size > maxSize) {
        sizeIndicator.classList.add('error');
    } else if (size > maxSize * 0.9) { // Warning at 90%
        sizeIndicator.classList.add('warning');
    }
}

function generate18CharLines(config) {
    const container = document.getElementById('linesOutput');
    container.innerHTML = '';
    
    if (!config) {
        container.innerHTML = '<p style="color: #858585; padding: 10px;">Generate configuration first</p>';
        return;
    }
    
    // Get chunk size from control (default 18 if not set)
    const lineBreakControl = document.getElementById('lineBreakCount');
    const chunkSize = lineBreakControl ? parseInt(lineBreakControl.value) : 18;
    
    // Remove newlines and create continuous string
    let continuous = config.replace(/\n/g, '');
    
    // Add opening marker
    let fullContinuous = '#[';
    
    // Add password authentication if set (unified format: hash/CFG:config)
    if (configState.authPasswordHash) {
        fullContinuous += `${configState.authPasswordHash}/CFG:`;
    } else {
        fullContinuous += 'CFG:';
    }
    
    // Add the configuration
    fullContinuous += continuous;
    
    // Add closing marker
    fullContinuous += ']#';
    
    continuous = fullContinuous;
    
    // Split into chunks of configurable size
    const chunks = [];
    for (let i = 0; i < continuous.length; i += chunkSize) {
        chunks.push(continuous.substring(i, i + chunkSize));
    }
    
    chunks.forEach((chunk, index) => {
        const lineItem = document.createElement('div');
        lineItem.className = 'line-item';
        
        const lineText = document.createElement('span');
        lineText.className = 'line-text';
        lineText.textContent = chunk;
        
        const copyBtn = document.createElement('button');
        copyBtn.className = 'line-copy-btn';
        copyBtn.textContent = '📋 Copy';
        copyBtn.addEventListener('click', function() {
            copyToClipboard(chunk);
            this.textContent = '✓ Copied';
            this.classList.add('copied');
            setTimeout(() => {
                this.textContent = '📋 Copy';
                this.classList.remove('copied');
            }, 2000);
        });
        
        lineItem.appendChild(lineText);
        lineItem.appendChild(copyBtn);
        container.appendChild(lineItem);
    });
}

function validateConfiguration() {
    const textarea = document.getElementById('fullOutput');
    const configText = textarea.value.trim();
    
    // If textarea is empty, generate first
    if (!configText) {
        generateOutput();
        return;
    }
    
    const errors = [];
    const warnings = [];
    
    // Parse the configuration text for validation
    const lines = configText.split('\n').map(l => l.trim()).filter(l => l);
    const validatedConfig = {
        switches: new Map(),
        inputs: new Map(),
        triggers: [],
        measurementSources: new Map(),
        sensorReporters: []
    };
    
    // Remove markers and parse
    let content = configText.replace(/^#\[/, '').replace(/\]#$/, '').trim();
    
    // Skip password and CFG: prefix (new format: hash/CFG:config)
    content = content.replace(/^[a-fA-F0-9]{64}\/CFG:/, '');
    // Also handle case without password
    content = content.replace(/^CFG:/, '');
    
    // Parse each line
    const configLines = content.split(/\n/).map(l => l.trim()).filter(l => l && !l.startsWith('#'));
    
    configLines.forEach((line, lineNum) => {
        line = line.replace(/\/$/, '');
        const parts = line.split(',').map(p => p.trim());
        if (parts.length === 0) return;
        
        const type = parts[0];
        const lineRef = `Line ${lineNum + 1}`;
        
        try {
            if (type === 'S' || type === 'SF' || type === 'SI' || type === 'SIF' || type === 'ST' || type === 'STF') {
                if (parts.length < 3) {
                    errors.push(`${lineRef}: Switch requires at least 3 fields (type,name,pin)`);
                } else {
                    const name = parts[1];
                    if (validatedConfig.switches.has(name)) {
                        errors.push(`${lineRef}: Duplicate switch name "${name}"`);
                    }
                    validatedConfig.switches.set(name, { type, parts });
                }
            } else if (type === 'I') {
                if (parts.length < 3) {
                    errors.push(`${lineRef}: Input requires at least 3 fields (type,name,pin)`);
                } else {
                    const name = parts[1];
                    if (validatedConfig.inputs.has(name)) {
                        errors.push(`${lineRef}: Duplicate input name "${name}"`);
                    }
                    validatedConfig.inputs.set(name, { parts });
                }
            } else if (type === 'T') {
                if (parts.length < 5) {
                    errors.push(`${lineRef}: Trigger requires at least 5 fields`);
                } else {
                    const inputName = parts[2];
                    validatedConfig.triggers.push({ lineRef, inputName, parts });
                }
            } else if (type === 'MS') {
                if (parts.length < 3) {
                    errors.push(`${lineRef}: Measurement source requires at least 3 fields`);
                } else {
                    const msType = parts[1];  // MS type (0=CPU, 1=Analog, 3=I2C, 4=SPI, 5=Loop)
                    const name = parts[2];     // MS name
                    if (validatedConfig.measurementSources.has(name)) {
                        errors.push(`${lineRef}: Duplicate measurement source name "${name}"`);
                    }
                    validatedConfig.measurementSources.set(name, { msType, parts });
                }
            } else if (type === 'SR') {
                if (parts.length < 9) {
                    errors.push(`${lineRef}: Sensor reporter requires at least 9 fields (SR,name,ms,vType,sType,enable,cumul,sampling,reporting)`);
                } else {
                    const msName = parts[2];
                    
                    // Validate sampling interval (index 7) - must be integer
                    const samplingMs = parseFloat(parts[7]);
                    if (!Number.isInteger(samplingMs)) {
                        errors.push(`${lineRef}: Sampling interval must be an integer (no decimals), got: ${parts[7]}`);
                    }
                    const samplingMin = PINCFG_LIMITS.SENSOR_SAMPLING_INTV_MIN_MS || 100;
                    const samplingMax = PINCFG_LIMITS.SENSOR_SAMPLING_INTV_MAX_MS || 5000;
                    if (samplingMs < samplingMin || samplingMs > samplingMax) {
                        errors.push(`${lineRef}: Sampling interval must be between ${samplingMin} and ${samplingMax} ms`);
                    }
                    
                    // Validate reporting interval (index 8) - must be integer
                    const reportingSec = parseFloat(parts[8]);
                    if (!Number.isInteger(reportingSec)) {
                        errors.push(`${lineRef}: Reporting interval must be an integer (no decimals), got: ${parts[8]}`);
                    }
                    const reportingMin = PINCFG_LIMITS.SENSOR_REPORTING_INTV_MIN_SEC || 1;
                    const reportingMax = PINCFG_LIMITS.SENSOR_REPORTING_INTV_MAX_SEC || 3600;
                    if (reportingSec < reportingMin || reportingSec > reportingMax) {
                        errors.push(`${lineRef}: Reporting interval must be between ${reportingMin} and ${reportingMax} seconds`);
                    }
                    
                    // Validate scale (index 9, optional) - max 6 decimals
                    if (parts.length >= 10 && parts[9] !== undefined && parts[9] !== '') {
                        const scale = parseFloat(parts[9]);
                        if (isNaN(scale)) {
                            errors.push(`${lineRef}: Invalid scale value: ${parts[9]}`);
                        } else if (scale === 0) {
                            errors.push(`${lineRef}: Scale cannot be 0 (would zero out measurement). Use 1.0 for no scaling`);
                        } else {
                            const scaleMin = PINCFG_LIMITS.SENSOR_SCALE_MIN || -1000;
                            const scaleMax = PINCFG_LIMITS.SENSOR_SCALE_MAX || 1000;
                            if (scale < scaleMin || scale > scaleMax) {
                                errors.push(`${lineRef}: Scale must be between ${scaleMin} and ${scaleMax}, got: ${parts[9]}`);
                            }
                            const decimalPart = parts[9].split('.')[1];
                            if (decimalPart && decimalPart.length > 6) {
                                errors.push(`${lineRef}: Scale can have maximum 6 decimal places, got: ${parts[9]}`);
                            }
                        }
                    }
                    
                    // Validate offset (index 10, optional) - max 6 decimals
                    if (parts.length >= 11 && parts[10] !== undefined && parts[10] !== '') {
                        const offset = parseFloat(parts[10]);
                        if (isNaN(offset)) {
                            errors.push(`${lineRef}: Invalid offset value: ${parts[10]}`);
                        } else {
                            const offsetMin = PINCFG_LIMITS.SENSOR_OFFSET_MIN || -1000;
                            const offsetMax = PINCFG_LIMITS.SENSOR_OFFSET_MAX || 1000;
                            if (offset < offsetMin || offset > offsetMax) {
                                errors.push(`${lineRef}: Offset must be between ${offsetMin} and ${offsetMax}, got: ${parts[10]}`);
                            }
                            const decimalPart = parts[10].split('.')[1];
                            if (decimalPart && decimalPart.length > 6) {
                                errors.push(`${lineRef}: Offset can have maximum 6 decimal places, got: ${parts[10]}`);
                            }
                        }
                    }
                    
                    // Validate precision (index 11, optional) - must be integer 0-6
                    if (parts.length >= 12 && parts[11] !== undefined && parts[11] !== '') {
                        const precision = parseFloat(parts[11]);
                        if (isNaN(precision)) {
                            errors.push(`${lineRef}: Invalid precision value: ${parts[11]}`);
                        } else {
                            if (!Number.isInteger(precision)) {
                                errors.push(`${lineRef}: Precision must be an integer (no decimals), got: ${parts[11]}`);
                            }
                            const precisionMin = PINCFG_LIMITS.SENSOR_PRECISION_MIN || 0;
                            const precisionMax = PINCFG_LIMITS.SENSOR_PRECISION_MAX || 6;
                            if (precision < precisionMin || precision > precisionMax) {
                                errors.push(`${lineRef}: Precision must be between ${precisionMin} and ${precisionMax}, got: ${parts[11]}`);
                            }
                        }
                    }
                    
                    // Validate byte offset (index 12, optional) - must be integer 0-5
                    if (parts.length >= 13 && parts[12] !== undefined && parts[12] !== '') {
                        const byteOffset = parseFloat(parts[12]);
                        if (isNaN(byteOffset)) {
                            errors.push(`${lineRef}: Invalid byte offset value: ${parts[12]}`);
                        } else {
                            if (!Number.isInteger(byteOffset)) {
                                errors.push(`${lineRef}: Byte offset must be an integer (no decimals), got: ${parts[12]}`);
                            }
                            if (byteOffset < 0 || byteOffset > 5) {
                                errors.push(`${lineRef}: Byte offset must be between 0 and 5, got: ${parts[12]}`);
                            }
                        }
                    }
                    
                    // Validate byte count (index 13, optional) - must be integer 0-6
                    if (parts.length >= 14 && parts[13] !== undefined && parts[13] !== '') {
                        const byteCount = parseFloat(parts[13]);
                        if (isNaN(byteCount)) {
                            errors.push(`${lineRef}: Invalid byte count value: ${parts[13]}`);
                        } else {
                            if (!Number.isInteger(byteCount)) {
                                errors.push(`${lineRef}: Byte count must be an integer (no decimals), got: ${parts[13]}`);
                            }
                            if (byteCount < 0 || byteCount > 6) {
                                errors.push(`${lineRef}: Byte count must be between 0 and 6, got: ${parts[13]}`);
                            }
                        }
                    }
                    
                    validatedConfig.sensorReporters.push({ lineRef, msName, parts });
                }
            }
        } catch (err) {
            errors.push(`${lineRef}: Parse error - ${err.message}`);
        }
    });
    
    // Cross-reference validation
    validatedConfig.triggers.forEach(trigger => {
        if (!validatedConfig.inputs.has(trigger.inputName)) {
            errors.push(`${trigger.lineRef}: Trigger references non-existent input "${trigger.inputName}"`);
        }
        // Check action switch references
        for (let i = 5; i < trigger.parts.length; i += 2) {
            const switchName = trigger.parts[i];
            if (switchName && !validatedConfig.switches.has(switchName)) {
                errors.push(`${trigger.lineRef}: Trigger references non-existent switch "${switchName}"`);
            }
        }
    });
    
    validatedConfig.sensorReporters.forEach(sr => {
        if (!validatedConfig.measurementSources.has(sr.msName)) {
            errors.push(`${sr.lineRef}: Sensor reporter references non-existent measurement source "${sr.msName}"`);
        }
    });
    
    // Display results
    const resultsDiv = document.getElementById('validationResults');
    
    if (errors.length === 0 && warnings.length === 0) {
        resultsDiv.className = 'validation-results success';
        resultsDiv.innerHTML = '<h4>✓ Validation Passed</h4><p>Configuration is valid!</p>';
        return;
    } else {
        resultsDiv.className = 'validation-results error';
        let html = '<h4>⚠ Validation Issues</h4>';
        
        if (errors.length > 0) {
            html += '<p><strong>Errors:</strong></p><ul>';
            errors.forEach(err => {
                html += `<li>${err}</li>`;
            });
            html += '</ul>';
        }
        
        if (warnings.length > 0) {
            html += '<p><strong>Warnings:</strong></p><ul>';
            warnings.forEach(warn => {
                html += `<li>${warn}</li>`;
            });
            html += '</ul>';
        }
        
        resultsDiv.innerHTML = html;
        return;
    }
}

function copyAllOutput() {
    const output = document.getElementById('fullOutput').value;
    if (!output) {
        alert('Generate configuration first!');
        return;
    }
    
    copyToClipboard(output);
    
    const btn = document.getElementById('copyAllBtn');
    const originalText = btn.textContent;
    btn.textContent = '✓ Copied!';
    setTimeout(() => {
        btn.textContent = originalText;
    }, 2000);
}

function buildFullCliMessage(config, isConfig = true) {
    if (!config) return null;
    
    // Remove newlines and create continuous string
    let continuous = config.replace(/\n/g, '');
    
    // Build full CLI message: #[hash/TYPE:data]#
    let fullMessage = '#[';
    
    if (configState.authPasswordHash) {
        fullMessage += `${configState.authPasswordHash}/`;
    }
    
    fullMessage += isConfig ? 'CFG:' : 'CMD:';
    fullMessage += continuous;
    fullMessage += ']#';
    
    return fullMessage;
}

function copyFullCliMessage() {
    const output = document.getElementById('fullOutput').value;
    if (!output) {
        alert('Generate configuration first!');
        return;
    }
    
    const fullMessage = buildFullCliMessage(output, true);
    copyToClipboard(fullMessage);
    
    const btn = document.getElementById('copyFullCliBtn');
    const originalText = btn.textContent;
    btn.textContent = '✓ Copied!';
    setTimeout(() => {
        btn.textContent = originalText;
    }, 2000);
}

function copyToClipboard(text) {
    if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(text).catch(err => {
            console.error('Failed to copy:', err);
            fallbackCopyToClipboard(text);
        });
    } else {
        fallbackCopyToClipboard(text);
    }
}

function fallbackCopyToClipboard(text) {
    const textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.style.position = 'fixed';
    textarea.style.opacity = '0';
    document.body.appendChild(textarea);
    textarea.select();
    
    try {
        document.execCommand('copy');
    } catch (err) {
        console.error('Fallback copy failed:', err);
    }
    
    document.body.removeChild(textarea);
}
