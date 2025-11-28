// Kotlin Compiler Visualizer - Main JavaScript

const API_BASE = '';
const SESSION_ID = 'session_' + Date.now();

let assemblyCode = '';
let totalInstructions = 0;
let previousState = null;

// DOM Elements
const codeEditor = document.getElementById('codeEditor');
const compileBtn = document.getElementById('compileBtn');
const loadBtn = document.getElementById('loadBtn');
const stepBtn = document.getElementById('stepBtn');
const stepBackBtn = document.getElementById('stepBackBtn');
const runBtn = document.getElementById('runBtn');
const resetBtn = document.getElementById('resetBtn');
const assemblyView = document.querySelector('.assembly-pre');
const currentInstr = document.getElementById('currentInstr');
const instrCounter = document.getElementById('instrCounter');
const totalInstrs = document.getElementById('totalInstrs');
const registersGrid = document.getElementById('registersGrid');
const flagsGrid = document.getElementById('flagsGrid');
const stackView = document.getElementById('stackView');
const statusBar = document.getElementById('statusBar');

// Event Listeners
compileBtn.addEventListener('click', compileCode);
loadBtn.addEventListener('click', loadAssembly);
stepBtn.addEventListener('click', stepForward);
stepBackBtn.addEventListener('click', stepBackward);
runBtn.addEventListener('click', runAll);
resetBtn.addEventListener('click', resetSimulator);

// Utility Functions
function setStatus(message, type = 'info') {
    statusBar.textContent = message;
    statusBar.className = 'status-bar';
    if (type !== 'info') {
        statusBar.classList.add(type);
    }
}

function enableControls(enabled) {
    stepBtn.disabled = !enabled;
    stepBackBtn.disabled = !enabled;
    runBtn.disabled = !enabled;
    resetBtn.disabled = !enabled;
}

// Compile Code
async function compileCode() {
    const code = codeEditor.value;

    if (!code.trim()) {
        setStatus('Please enter some code', 'error');
        return;
    }

    setStatus('Compiling...', 'warning');
    compileBtn.disabled = true;

    try {
        const response = await fetch(`${API_BASE}/compile`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ code })
        });

        const data = await response.json();

        if (data.success) {
            assemblyCode = data.assembly;
            displayAssembly(assemblyCode);
            setStatus('Compilation successful! Click "Load Assembly" to start debugging.', 'success');
            loadBtn.disabled = false;
            compileBtn.disabled = false;
        } else {
            setStatus('Compilation failed: ' + data.error, 'error');
            assemblyView.textContent = 'Error:\n' + data.error;
            compileBtn.disabled = false;
        }
    } catch (error) {
        setStatus('Error: ' + error.message, 'error');
        compileBtn.disabled = false;
    }
}

// Display Assembly Code
function displayAssembly(assembly) {
    const lines = assembly.split('\n');
    let html = '';

    lines.forEach((line, index) => {
        html += `<span class="line" data-line="${index}">${escapeHtml(line)}</span>\n`;
    });

    assemblyView.innerHTML = html;
}

function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}

// Load Assembly into Simulator
async function loadAssembly() {
    if (!assemblyCode) {
        setStatus('No assembly code to load', 'error');
        return;
    }

    setStatus('Loading assembly into simulator...', 'warning');
    loadBtn.disabled = true;

    try {
        const response = await fetch(`${API_BASE}/load_assembly`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                assembly: assemblyCode,
                session_id: SESSION_ID
            })
        });

        const data = await response.json();

        if (data.success) {
            totalInstructions = data.total_instructions;
            totalInstrs.textContent = totalInstructions;
            updateUI(data.state);
            enableControls(true);
            setStatus('Simulator loaded. Use controls to step through execution.', 'success');
        } else {
            setStatus('Error loading assembly: ' + data.error, 'error');
            loadBtn.disabled = false;
        }
    } catch (error) {
        setStatus('Error: ' + error.message, 'error');
        loadBtn.disabled = false;
    }
}

// Step Forward
async function stepForward() {
    setStatus('Executing step...', 'warning');
    stepBtn.disabled = true;

    try {
        const response = await fetch(`${API_BASE}/step`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ session_id: SESSION_ID })
        });

        const data = await response.json();

        if (data.success) {
            updateUI(data.state);

            if (!data.can_continue) {
                setStatus('Execution completed!', 'success');
                stepBtn.disabled = true;
                runBtn.disabled = true;
            } else {
                setStatus('Step executed', 'success');
                stepBtn.disabled = false;
            }
        } else {
            setStatus('Error: ' + data.error, 'error');
            stepBtn.disabled = false;
        }
    } catch (error) {
        setStatus('Error: ' + error.message, 'error');
        stepBtn.disabled = false;
    }
}

// Step Backward
async function stepBackward() {
    setStatus('Stepping back...', 'warning');

    try {
        const response = await fetch(`${API_BASE}/step_back`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ session_id: SESSION_ID })
        });

        const data = await response.json();

        if (data.success && data.state) {
            updateUI(data.state);
            setStatus('Stepped back one instruction', 'success');
            stepBtn.disabled = false;
            runBtn.disabled = false;
        } else {
            setStatus('Cannot step back further', 'warning');
        }
    } catch (error) {
        setStatus('Error: ' + error.message, 'error');
    }
}

// Run All
async function runAll() {
    setStatus('Running...', 'warning');
    runBtn.disabled = true;

    try {
        const response = await fetch(`${API_BASE}/run`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ session_id: SESSION_ID })
        });

        const data = await response.json();

        if (data.success) {
            updateUI(data.state);
            setStatus(`Execution completed! (${data.steps_executed} steps)`, 'success');
            stepBtn.disabled = true;
            runBtn.disabled = true;
        } else {
            setStatus('Error: ' + data.error, 'error');
            runBtn.disabled = false;
        }
    } catch (error) {
        setStatus('Error: ' + error.message, 'error');
        runBtn.disabled = false;
    }
}

// Reset Simulator
async function resetSimulator() {
    setStatus('Resetting...', 'warning');

    try {
        const response = await fetch(`${API_BASE}/reset`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ session_id: SESSION_ID })
        });

        const data = await response.json();

        if (data.success) {
            updateUI(data.state);
            setStatus('Simulator reset to beginning', 'success');
            stepBtn.disabled = false;
            runBtn.disabled = false;
        } else {
            setStatus('Error: ' + data.error, 'error');
        }
    } catch (error) {
        setStatus('Error: ' + error.message, 'error');
    }
}

// Update UI with current state
function updateUI(state) {
    // Update current instruction
    const instrLine = state.current_instruction;
    instrCounter.textContent = instrLine + 1;

    // Highlight current instruction in assembly
    document.querySelectorAll('.assembly-pre .line').forEach((line, index) => {
        if (index === instrLine) {
            line.classList.add('current');
            line.scrollIntoView({ behavior: 'smooth', block: 'center' });
        } else {
            line.classList.remove('current');
        }
    });

    // Display current instruction
    currentInstr.innerHTML = `<code>${escapeHtml(state.instruction)}</code>`;

    // Update registers
    updateRegisters(state.registers);

    // Update flags
    updateFlags(state.flags);

    // Update stack
    updateStack(state.stack);

    previousState = state;
}

// Update Registers Display
function updateRegisters(registers) {
    const importantRegs = ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp',
                          'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15'];

    let html = '';

    importantRegs.forEach(reg => {
        const value = registers[reg] || '0x0';
        const changed = previousState &&
                       previousState.registers[reg] !== value;

        html += `
            <div class="register ${changed ? 'changed' : ''}">
                <div class="register-name">${reg.toUpperCase()}</div>
                <div class="register-value">${value}</div>
            </div>
        `;
    });

    registersGrid.innerHTML = html;
}

// Update Flags Display
function updateFlags(flags) {
    let html = '';

    Object.keys(flags).forEach(flag => {
        const value = flags[flag];
        const isSet = value === 1;

        html += `
            <div class="flag ${isSet ? 'set' : ''}">
                <div class="flag-name">${flag}</div>
                <div class="flag-value">${value}</div>
            </div>
        `;
    });

    flagsGrid.innerHTML = html;
}

// Update Stack Display
function updateStack(stack) {
    if (!stack || stack.length === 0) {
        stackView.innerHTML = '<div style="padding: 20px; text-align: center; color: #999;">Stack is empty</div>';
        return;
    }

    let html = '';

    stack.forEach(item => {
        const classes = [];
        if (item.is_rsp) classes.push('rsp');
        if (item.is_rbp) classes.push('rbp');

        const marker = item.is_rsp ? ' ← RSP' : item.is_rbp ? ' ← RBP' : '';

        html += `
            <div class="stack-item ${classes.join(' ')}">
                <div class="stack-address">${item.address}${marker}</div>
                <div class="stack-value">${typeof item.value === 'number' ? '0x' + item.value.toString(16) : item.value}</div>
            </div>
        `;
    });

    stackView.innerHTML = html;
}

// Initialize
setStatus('Ready. Write your Kotlin code and click "Compile".');
