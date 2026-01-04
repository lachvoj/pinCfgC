// Part 5: CLI Commands Tab functionality

// CLI_COMMANDS will be auto-generated from Cli.c by build script
// Format: [{name: 'GET_CFG', hasParam: false, description: '...'}, ...]

function setupCommandsTab() {
    // Populate command dropdown
    const cmdSelect = document.getElementById('cmdSelect');
    if (cmdSelect && typeof CLI_COMMANDS !== 'undefined') {
        CLI_COMMANDS.forEach(cmd => {
            const option = document.createElement('option');
            option.value = cmd.name;
            option.textContent = cmd.name;
            cmdSelect.appendChild(option);
        });
    }
    
    // Setup password sync between tabs
    setupCommandPasswordSync();
    
    // Setup password visibility toggle
    setupCommandPasswordToggle();
    
    // Setup new password hash generation for CHANGE_PWD
    setupNewPasswordHashGeneration();
    
    // Setup command selection handler
    if (cmdSelect) {
        cmdSelect.addEventListener('change', onCommandChange);
    }
    
    // Setup generate button
    const generateBtn = document.getElementById('generateCmdBtn');
    if (generateBtn) {
        generateBtn.addEventListener('click', generateCommand);
    }
    
    // Setup copy all button
    const copyAllBtn = document.getElementById('copyCmdAllBtn');
    if (copyAllBtn) {
        copyAllBtn.addEventListener('click', copyAllCommandLines);
    }
    
    // Setup copy full CLI message button
    const copyFullCliBtn = document.getElementById('copyCmdFullCliBtn');
    if (copyFullCliBtn) {
        copyFullCliBtn.addEventListener('click', copyCmdFullCliMessage);
    }
    
    // Setup line break change handler
    const lineBreakInput = document.getElementById('cmdLineBreakCount');
    if (lineBreakInput) {
        lineBreakInput.addEventListener('change', function() {
            // Regenerate if there's a command selected
            const cmdSelect = document.getElementById('cmdSelect');
            if (cmdSelect && cmdSelect.value) {
                generateCommand();
            }
        });
    }
}

function setupCommandPasswordSync() {
    const configPwdInput = document.getElementById('authPassword');
    const configHashInput = document.getElementById('authPasswordHash');
    const cmdPwdInput = document.getElementById('cmdAuthPassword');
    const cmdHashInput = document.getElementById('cmdAuthPasswordHash');
    
    // Sync from config tab to command tab
    if (configPwdInput) {
        configPwdInput.addEventListener('input', function() {
            if (cmdPwdInput) cmdPwdInput.value = this.value;
        });
    }
    
    if (configHashInput) {
        const syncHash = function() {
            if (cmdHashInput) cmdHashInput.value = configHashInput.value;
        };
        configHashInput.addEventListener('input', syncHash);
        configHashInput.addEventListener('change', syncHash);
    }
    
    // Sync from command tab to config tab and configState
    if (cmdPwdInput) {
        cmdPwdInput.addEventListener('input', async function() {
            const password = this.value;
            configState.authPassword = password;
            if (configPwdInput) configPwdInput.value = password;
            
            if (password) {
                const hash = await sha256(password);
                configState.authPasswordHash = hash;
                if (cmdHashInput) cmdHashInput.value = hash;
                if (configHashInput) configHashInput.value = hash;
            } else {
                configState.authPasswordHash = '';
                if (cmdHashInput) cmdHashInput.value = '';
                if (configHashInput) configHashInput.value = '';
            }
            saveToLocalStorage();
        });
    }
    
    if (cmdHashInput) {
        cmdHashInput.addEventListener('change', function() {
            const value = this.value.trim();
            if (/^[a-f0-9]{64}$/i.test(value)) {
                configState.authPasswordHash = value.toLowerCase();
                configState.authPassword = '';
                if (configHashInput) configHashInput.value = value.toLowerCase();
                if (configPwdInput) configPwdInput.value = '';
                if (cmdPwdInput) cmdPwdInput.value = '';
                saveToLocalStorage();
            }
        });
    }
    
    // Initialize command tab with current values
    if (cmdPwdInput && configState.authPassword) {
        cmdPwdInput.value = configState.authPassword;
    }
    if (cmdHashInput && configState.authPasswordHash) {
        cmdHashInput.value = configState.authPasswordHash;
    }
}

function setupCommandPasswordToggle() {
    const toggleBtn = document.getElementById('toggleCmdPassword');
    const pwdInput = document.getElementById('cmdAuthPassword');
    
    if (toggleBtn && pwdInput) {
        toggleBtn.addEventListener('click', function() {
            const isPassword = pwdInput.type === 'password';
            pwdInput.type = isPassword ? 'text' : 'password';
            this.classList.toggle('active', isPassword);
            this.title = isPassword ? 'Hide password' : 'Show password';
        });
    }
    
    // Toggle for new password field
    const toggleNewBtn = document.getElementById('toggleNewPassword');
    const newPwdInput = document.getElementById('newPassword');
    
    if (toggleNewBtn && newPwdInput) {
        toggleNewBtn.addEventListener('click', function() {
            const isPassword = newPwdInput.type === 'password';
            newPwdInput.type = isPassword ? 'text' : 'password';
            this.classList.toggle('active', isPassword);
            this.title = isPassword ? 'Hide password' : 'Show password';
        });
    }
}

function setupNewPasswordHashGeneration() {
    const newPwdInput = document.getElementById('newPassword');
    const newHashInput = document.getElementById('newPasswordHash');
    
    if (newPwdInput && newHashInput) {
        newPwdInput.addEventListener('input', async function() {
            const password = this.value;
            if (password) {
                const hash = await sha256(password);
                newHashInput.value = hash;
            } else {
                newHashInput.value = '';
            }
        });
    }
}

function onCommandChange() {
    const cmdSelect = document.getElementById('cmdSelect');
    const descDiv = document.getElementById('cmdDescription');
    const changePwdParams = document.getElementById('cmdChangePwdParams');
    
    if (!cmdSelect) return;
    
    const selectedCmd = cmdSelect.value;
    
    // Update description
    if (descDiv && typeof CLI_COMMANDS !== 'undefined') {
        const cmdInfo = CLI_COMMANDS.find(c => c.name === selectedCmd);
        descDiv.textContent = cmdInfo ? cmdInfo.description : '';
    }
    
    // Show/hide CHANGE_PWD parameters
    if (changePwdParams) {
        changePwdParams.style.display = (selectedCmd === 'CHANGE_PWD:') ? 'block' : 'none';
    }
    
    // Clear output when command changes
    const linesOutput = document.getElementById('cmdLinesOutput');
    if (linesOutput) {
        linesOutput.innerHTML = '<p style="color: #858585; padding: 10px;">Select a command and click "Generate Command"</p>';
    }
}

function generateCommand() {
    const cmdSelect = document.getElementById('cmdSelect');
    const linesOutput = document.getElementById('cmdLinesOutput');
    
    if (!cmdSelect || !linesOutput) return;
    
    const selectedCmd = cmdSelect.value;
    
    if (!selectedCmd) {
        linesOutput.innerHTML = '<p style="color: #f48771; padding: 10px;">Please select a command first</p>';
        return;
    }
    
    if (!configState.authPasswordHash) {
        linesOutput.innerHTML = '<p style="color: #f48771; padding: 10px;">Please enter a password for authentication</p>';
        return;
    }
    
    // Build command string
    let cmdString = '';
    
    if (selectedCmd === 'CHANGE_PWD:') {
        const newHashInput = document.getElementById('newPasswordHash');
        if (!newHashInput || !newHashInput.value) {
            linesOutput.innerHTML = '<p style="color: #f48771; padding: 10px;">Please enter a new password</p>';
            return;
        }
        cmdString = `CHANGE_PWD:${newHashInput.value}`;
    } else {
        cmdString = selectedCmd;
    }
    
    // Build full message: #[hash/CMD:command]#
    const fullMessage = `#[${configState.authPasswordHash}/CMD:${cmdString}]#`;
    
    // Generate chunked lines
    generateCommandLines(fullMessage);
}

function generateCommandLines(message) {
    const container = document.getElementById('cmdLinesOutput');
    if (!container) return;
    
    container.innerHTML = '';
    
    // Get chunk size
    const lineBreakControl = document.getElementById('cmdLineBreakCount');
    const chunkSize = lineBreakControl ? parseInt(lineBreakControl.value) : 18;
    
    // Split into chunks
    const chunks = [];
    for (let i = 0; i < message.length; i += chunkSize) {
        chunks.push(message.substring(i, i + chunkSize));
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

function copyAllCommandLines() {
    const container = document.getElementById('cmdLinesOutput');
    if (!container) return;
    
    const lines = container.querySelectorAll('.line-text');
    let fullText = '';
    lines.forEach(line => {
        fullText += line.textContent;
    });
    
    if (fullText) {
        copyToClipboard(fullText);
        
        const copyBtn = document.getElementById('copyCmdAllBtn');
        if (copyBtn) {
            const originalText = copyBtn.textContent;
            copyBtn.textContent = '✓ Copied!';
            setTimeout(() => { copyBtn.textContent = originalText; }, 2000);
        }
    }
}

function copyCmdFullCliMessage() {
    const container = document.getElementById('cmdLinesOutput');
    if (!container) return;
    
    const lines = container.querySelectorAll('.line-text');
    let fullText = '';
    lines.forEach(line => {
        fullText += line.textContent;
    });
    
    if (fullText) {
        copyToClipboard(fullText);
        
        const copyBtn = document.getElementById('copyCmdFullCliBtn');
        if (copyBtn) {
            const originalText = copyBtn.textContent;
            copyBtn.textContent = '✓ Copied!';
            setTimeout(() => { copyBtn.textContent = originalText; }, 2000);
        }
    } else {
        alert('Generate command first!');
    }
}
