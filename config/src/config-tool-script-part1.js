// pinCfgC Configuration Tool - JavaScript
// Part 1: Data structures and state management

const configState = {
    authPassword: '',      // Plain text password (for display only, not stored)
    authPasswordHash: '',  // SHA256 hash of password (used in output)
    global: {
        CD: { value: '330', enabled: false },
        CM: { value: '620', enabled: false },
        CR: { value: '150', enabled: false },
        CN: { value: '1000', enabled: false },
        CA: { value: '1966080', enabled: false }
    },
    switches: [],
    inputs: [],
    triggers: [],
    measurementSources: [],
    sensorReporters: []
};

// SHA256 hash function (Web Crypto API)
async function sha256(message) {
    if (!message) return '';
    const msgBuffer = new TextEncoder().encode(message);
    const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    const hashHex = hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
    return hashHex;
}

// Input event types
const INPUT_EVENT_TYPES = {
    '0': 'Down',
    '1': 'Up',
    '2': 'Longpress',
    '3': 'Multiclick',
    '4': 'All'
};

// Switch action types
const SWITCH_ACTIONS = {
    '0': 'Toggle',
    '1': 'Turn On',
    '2': 'Turn Off',
    '3': 'Forward'
};

// Measurement types
const MEASUREMENT_TYPES = {
    '0': 'CPU Temperature',
    '1': 'Analog',
    '3': 'I2C',
    '4': 'SPI',
    '5': 'Loop Time'
};

// MySensors V_TYPES and S_TYPES will be auto-generated from MyMessage.h by build script

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    initializeAuthPassword();
    initializeGlobalConfig();
    setupCollapsible();
    setupEventListeners();
    loadFromLocalStorage();
    updateSectionVisibility();
});

async function initializeAuthPassword() {
    const passwordInput = document.getElementById('authPassword');
    const hashInput = document.getElementById('authPasswordHash');
    const copyHashBtn = document.getElementById('copyHashBtn');
    const togglePasswordBtn = document.getElementById('togglePassword');
    
    // If we have a stored hash but no password, show in hash field
    if (configState.authPasswordHash) {
        hashInput.value = configState.authPasswordHash;
    }
    
    // Password visibility toggle
    if (togglePasswordBtn) {
        togglePasswordBtn.addEventListener('click', function() {
            const isPassword = passwordInput.type === 'password';
            passwordInput.type = isPassword ? 'text' : 'password';
            this.classList.toggle('active', isPassword);
            this.title = isPassword ? 'Hide password' : 'Show password';
        });
    }
    
    // Hash password on input change
    passwordInput.addEventListener('input', async function() {
        const password = this.value;
        configState.authPassword = password;
        
        if (password) {
            const hash = await sha256(password);
            configState.authPasswordHash = hash;
            hashInput.value = hash;
        } else {
            configState.authPasswordHash = '';
            hashInput.value = '';
        }
        saveToLocalStorage();
    });
    
    // Allow direct hash input (for pasting existing hash)
    hashInput.addEventListener('change', function() {
        // If user manually enters a hash (64 hex chars), use it directly
        const value = this.value.trim();
        if (/^[a-f0-9]{64}$/i.test(value)) {
            configState.authPasswordHash = value.toLowerCase();
            configState.authPassword = ''; // Clear password since we're using direct hash
            passwordInput.value = '';
            saveToLocalStorage();
        }
    });
    
    // Remove readonly to allow manual hash input
    hashInput.removeAttribute('readonly');
    hashInput.placeholder = 'Auto-generated or paste existing hash (64 hex chars)';
    
    // Copy hash button
    if (copyHashBtn) {
        copyHashBtn.addEventListener('click', function() {
            if (hashInput.value) {
                copyToClipboard(hashInput.value);
                const originalText = this.textContent;
                this.textContent = '✓ Copied!';
                setTimeout(() => { this.textContent = originalText; }, 2000);
            }
        });
    }
}

function setupCollapsible() {
    // Setup collapsible for all sections - all start collapsed
    const sections = [
        { headerId: 'authHeader', contentId: 'authContent' },
        { headerId: 'globalConfigHeader', contentId: 'globalConfigContent' },
        { headerId: 'switchesHeader', contentId: 'switchesContent' },
        { headerId: 'inputsHeader', contentId: 'inputsContent' },
        { headerId: 'triggersHeader', contentId: 'triggersContent' },
        { headerId: 'msHeader', contentId: 'msContent' },
        { headerId: 'srHeader', contentId: 'srContent' }
    ];
    
    sections.forEach(section => {
        const header = document.getElementById(section.headerId);
        const content = document.getElementById(section.contentId);
        const btn = header.querySelector('.btn-collapse');
        
        // Start all sections collapsed
        content.classList.add('collapsed');
        btn.classList.add('collapsed');
        
        // Add click handler
        header.addEventListener('click', function() {
            content.classList.toggle('collapsed');
            btn.classList.toggle('collapsed');
        });
    });
}

function updateSectionVisibility() {
    // Expand sections that have content
    if (configState.authPassword || configState.authPasswordHash) {
        expandSection('authHeader', 'authContent');
    }
    
    // Check if global config has any enabled values
    const hasGlobalConfig = Object.values(configState.global).some(item => item.enabled && item.value);
    if (hasGlobalConfig) {
        expandSection('globalConfigHeader', 'globalConfigContent');
    }
    
    if (configState.switches.length > 0) {
        expandSection('switchesHeader', 'switchesContent');
    }
    
    if (configState.inputs.length > 0) {
        expandSection('inputsHeader', 'inputsContent');
    }
    
    if (configState.triggers.length > 0) {
        expandSection('triggersHeader', 'triggersContent');
    }
    
    if (configState.measurementSources.length > 0) {
        expandSection('msHeader', 'msContent');
    }
    
    if (configState.sensorReporters.length > 0) {
        expandSection('srHeader', 'srContent');
    }
}

function expandSection(headerId, contentId) {
    const content = document.getElementById(contentId);
    const btn = document.getElementById(headerId).querySelector('.btn-collapse');
    content.classList.remove('collapsed');
    btn.classList.remove('collapsed');
}

function initializeGlobalConfig() {
    const globalForm = document.getElementById('globalConfigForm');
    const globalFields = [
        { key: 'CD', label: 'Debounce (ms)', type: 'number', min: 0, max: 10000 },
        { key: 'CM', label: 'Multiclick (ms)', type: 'number', min: 0, max: 10000 },
        { key: 'CR', label: 'Relay Impulse (ms)', type: 'number', min: 50, max: 10000 },
        { key: 'CN', label: 'Feedback (ms)', type: 'number', min: 0, max: 100000 },
        { key: 'CA', label: 'Announcement (ms)', type: 'number', min: 0, max: 10000000 }
    ];

    globalFields.forEach(field => {
        const formGroup = document.createElement('div');
        formGroup.className = 'form-group';
        formGroup.style.display = 'flex';
        formGroup.style.alignItems = 'center';
        formGroup.style.gap = '10px';
        
        // Checkbox
        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.id = `global_${field.key}_enabled`;
        checkbox.checked = configState.global[field.key].enabled;
        checkbox.style.width = 'auto';
        checkbox.style.cursor = 'pointer';
        
        // Label with key prefix
        const label = document.createElement('label');
        label.textContent = `${field.key} - ${field.label}`;
        label.style.flex = '0 0 auto';
        label.style.minWidth = '200px';
        label.style.cursor = 'pointer';
        label.addEventListener('click', function() {
            checkbox.click();
        });
        
        // Input
        const input = document.createElement('input');
        input.type = field.type;
        input.className = 'number-input';
        input.id = `global_${field.key}`;
        input.value = configState.global[field.key].value;
        input.min = field.min;
        input.max = field.max;
        input.style.flex = '1';
        input.disabled = !configState.global[field.key].enabled;
        
        checkbox.addEventListener('change', function() {
            configState.global[field.key].enabled = this.checked;
            input.disabled = !this.checked;
            saveToLocalStorage();
        });
        
        input.addEventListener('change', function() {
            configState.global[field.key].value = this.value;
            saveToLocalStorage();
        });
        
        formGroup.appendChild(checkbox);
        formGroup.appendChild(label);
        formGroup.appendChild(input);
        globalForm.appendChild(formGroup);
    });
}

function setupEventListeners() {
    // Tab navigation
    setupTabListeners();
    
    // Switch controls
    document.getElementById('addSwitchBtn').addEventListener('click', addSwitch);
    
    // Input controls
    document.getElementById('addInputBtn').addEventListener('click', addInput);
    
    // Trigger controls
    document.getElementById('addTriggerBtn').addEventListener('click', addTrigger);
    
    // Measurement Source controls
    document.getElementById('addMsBtn').addEventListener('click', addMeasurementSource);
    
    // Sensor Reporter controls
    document.getElementById('addSrBtn').addEventListener('click', addSensorReporter);
    
    // Toolbar buttons
    document.getElementById('saveBtn').addEventListener('click', saveConfiguration);
    document.getElementById('loadBtn').addEventListener('click', loadConfiguration);
    document.getElementById('clearBtn').addEventListener('click', clearAll);
    
    // Output controls
    document.getElementById('generateBtn').addEventListener('click', generateOutput);
    document.getElementById('parseBtn').addEventListener('click', parseAndLoadConfiguration);
    document.getElementById('validateBtn').addEventListener('click', validateConfiguration);
    document.getElementById('copyAllBtn').addEventListener('click', copyAllOutput);
    document.getElementById('copyFullCliBtn').addEventListener('click', copyFullCliMessage);
    
    // Update size indicator when textarea is manually edited
    const fullOutput = document.getElementById('fullOutput');
    fullOutput.addEventListener('input', function() {
        // Remove newlines to get actual config size (they're just for display)
        const actualSize = this.value.replace(/\n/g, '').length;
        updateConfigSize(actualSize);
    });
    
    // Line break control - regenerate 18-char lines when changed
    const lineBreakControl = document.getElementById('lineBreakCount');
    if (lineBreakControl) {
        lineBreakControl.addEventListener('input', function() {
            // Regenerate 18-char lines with new break count
            generateOutput();
        });
    }
    
    // Setup Commands Tab
    if (typeof setupCommandsTab === 'function') {
        setupCommandsTab();
    }
    
    // Setup Errors Tab
    if (typeof setupErrorsTab === 'function') {
        setupErrorsTab();
    }
}

function setupTabListeners() {
    const tabBtns = document.querySelectorAll('.tab-btn');
    const tabContents = document.querySelectorAll('.tab-content');
    
    tabBtns.forEach(btn => {
        btn.addEventListener('click', function() {
            const targetTab = this.dataset.tab;
            
            // Remove active from all tabs and contents
            tabBtns.forEach(b => b.classList.remove('active'));
            tabContents.forEach(c => c.classList.remove('active'));
            
            // Add active to clicked button and corresponding content
            this.classList.add('active');
            const targetContent = document.getElementById(`tab-${targetTab}`);
            if (targetContent) {
                targetContent.classList.add('active');
            }
        });
    });
}

function addSwitch() {
    const type = document.getElementById('switchType').value;
    const switchObj = {
        id: Date.now(),
        type: type,
        name: `o${String(configState.switches.length + 1).padStart(2, '0')}`,
        pin: '',
        feedbackPin: type === 'SF' || type === 'SIF' || type === 'STF' ? '' : undefined,
        duration: type === 'ST' || type === 'STF' ? '5000' : undefined
    };
    
    configState.switches.push(switchObj);
    renderSwitch(switchObj);
    expandSection('switchesHeader', 'switchesContent');
    saveToLocalStorage();
}

function renderSwitch(switchObj) {
    const container = document.getElementById('switchesList');
    
    const card = document.createElement('div');
    card.className = 'item-card';
    card.dataset.id = switchObj.id;
    
    const header = document.createElement('div');
    header.className = 'item-header';
    
    const title = document.createElement('span');
    title.className = 'item-title';
    title.textContent = `${switchObj.type} - ${switchObj.name}`;
    
    const actions = document.createElement('div');
    actions.className = 'item-actions';
    
    const removeBtn = document.createElement('button');
    removeBtn.className = 'btn-small btn-remove';
    removeBtn.textContent = '✕ Remove';
    removeBtn.addEventListener('click', () => removeSwitch(switchObj.id));
    
    actions.appendChild(removeBtn);
    header.appendChild(title);
    header.appendChild(actions);
    
    const fields = document.createElement('div');
    fields.className = 'item-fields';
    
    // Name field
    fields.appendChild(createFormField('Name', 'text', switchObj.name, (value) => {
        switchObj.name = value;
        title.textContent = `${switchObj.type} - ${switchObj.name}`;
        saveToLocalStorage();
    }));
    
    // Pin field
    fields.appendChild(createFormField('Pin', 'number', switchObj.pin, (value) => {
        switchObj.pin = value;
        saveToLocalStorage();
    }, { min: 0, max: 255 }));
    
    // Feedback pin (if applicable)
    if (switchObj.feedbackPin !== undefined) {
        fields.appendChild(createFormField('Feedback Pin', 'number', switchObj.feedbackPin, (value) => {
            switchObj.feedbackPin = value;
            saveToLocalStorage();
        }, { min: 0, max: 255 }));
    }
    
    // Duration (if applicable)
    if (switchObj.duration !== undefined) {
        fields.appendChild(createFormField('Duration (ms)', 'number', switchObj.duration, (value) => {
            switchObj.duration = value;
            saveToLocalStorage();
        }, { 
            min: PINCFG_LIMITS.TIMED_SWITCH_MIN_PERIOD_MS || 50, 
            max: PINCFG_LIMITS.TIMED_SWITCH_MAX_PERIOD_MS || 600000 
        }));
    }
    
    card.appendChild(header);
    card.appendChild(fields);
    container.appendChild(card);
}

function removeSwitch(id) {
    const index = configState.switches.findIndex(s => s.id === id);
    if (index !== -1) {
        configState.switches.splice(index, 1);
        document.querySelector(`#switchesList [data-id="${id}"]`).remove();
        saveToLocalStorage();
    }
}

function addInput() {
    const inputObj = {
        id: Date.now(),
        name: `i${String(configState.inputs.length + 1).padStart(2, '0')}`,
        pin: ''
    };
    
    configState.inputs.push(inputObj);
    renderInput(inputObj);
    expandSection('inputsHeader', 'inputsContent');
    saveToLocalStorage();
}

function renderInput(inputObj) {
    const container = document.getElementById('inputsList');
    
    const card = document.createElement('div');
    card.className = 'item-card';
    card.dataset.id = inputObj.id;
    
    const header = document.createElement('div');
    header.className = 'item-header';
    
    const title = document.createElement('span');
    title.className = 'item-title';
    title.textContent = `Input - ${inputObj.name}`;
    
    const actions = document.createElement('div');
    actions.className = 'item-actions';
    
    const removeBtn = document.createElement('button');
    removeBtn.className = 'btn-small btn-remove';
    removeBtn.textContent = '✕ Remove';
    removeBtn.addEventListener('click', () => removeInput(inputObj.id));
    
    actions.appendChild(removeBtn);
    header.appendChild(title);
    header.appendChild(actions);
    
    const fields = document.createElement('div');
    fields.className = 'item-fields';
    
    fields.appendChild(createFormField('Name', 'text', inputObj.name, (value) => {
        inputObj.name = value;
        title.textContent = `Input - ${inputObj.name}`;
        saveToLocalStorage();
    }));
    
    fields.appendChild(createFormField('Pin', 'number', inputObj.pin, (value) => {
        inputObj.pin = value;
        saveToLocalStorage();
    }, { min: 0, max: 255 }));
    
    card.appendChild(header);
    card.appendChild(fields);
    container.appendChild(card);
}

function removeInput(id) {
    const index = configState.inputs.findIndex(i => i.id === id);
    if (index !== -1) {
        configState.inputs.splice(index, 1);
        document.querySelector(`#inputsList [data-id="${id}"]`).remove();
        saveToLocalStorage();
    }
}
