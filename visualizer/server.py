"""
Web Server for Compiler Visualizer
Provides interactive visualization of register and stack execution
"""

from flask import Flask, render_template, request, jsonify
from flask_cors import CORS
import subprocess
import os
import sys
from simulator import X86Simulator

app = Flask(__name__)
CORS(app)

# Store simulator instances per session (simplified - using global for demo)
simulators = {}

@app.route('/')
def index():
    """Main page"""
    return render_template('index.html')

@app.route('/compile', methods=['POST'])
def compile_code():
    """Compile C code to assembly"""
    data = request.json
    source_code = data.get('code', '')

    # Save source code to temp file
    temp_file = '/tmp/temp_program.c'
    asm_file = '/tmp/temp_output.s'

    with open(temp_file, 'w') as f:
        f.write(source_code)

    # Get the compiler path (assume we're in visualizer/ subdirectory)
    compiler_path = os.path.join(os.path.dirname(__file__), '..', 'compiler')

    # Compile the code
    try:
        result = subprocess.run(
            [compiler_path, temp_file, '-o', asm_file],
            capture_output=True,
            text=True,
            timeout=10
        )

        if result.returncode != 0:
            return jsonify({
                'success': False,
                'error': result.stderr
            })

        # Read the generated assembly
        with open(asm_file, 'r') as f:
            assembly = f.read()

        return jsonify({
            'success': True,
            'assembly': assembly
        })

    except subprocess.TimeoutExpired:
        return jsonify({
            'success': False,
            'error': 'Compilation timeout'
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        })

@app.route('/load_assembly', methods=['POST'])
def load_assembly():
    """Load assembly code into simulator"""
    data = request.json
    assembly = data.get('assembly', '')
    session_id = data.get('session_id', 'default')

    sim = X86Simulator()
    sim.load_assembly(assembly)
    simulators[session_id] = sim

    return jsonify({
        'success': True,
        'state': sim.get_state(),
        'total_instructions': len(sim.instructions),
        'instructions': sim.instructions
    })

@app.route('/step', methods=['POST'])
def step():
    """Execute one instruction step"""
    data = request.json
    session_id = data.get('session_id', 'default')

    if session_id not in simulators:
        return jsonify({'success': False, 'error': 'No simulator loaded'})

    sim = simulators[session_id]
    can_continue = sim.step()

    return jsonify({
        'success': True,
        'state': sim.get_state(),
        'can_continue': can_continue
    })

@app.route('/step_back', methods=['POST'])
def step_back():
    """Step back one instruction"""
    data = request.json
    session_id = data.get('session_id', 'default')

    if session_id not in simulators:
        return jsonify({'success': False, 'error': 'No simulator loaded'})

    sim = simulators[session_id]
    success = sim.step_back()

    return jsonify({
        'success': success,
        'state': sim.get_state() if success else None
    })

@app.route('/run', methods=['POST'])
def run():
    """Run until completion or breakpoint"""
    data = request.json
    session_id = data.get('session_id', 'default')
    breakpoint = data.get('breakpoint', -1)

    if session_id not in simulators:
        return jsonify({'success': False, 'error': 'No simulator loaded'})

    sim = simulators[session_id]

    # Execute steps
    steps = 0
    max_steps = 10000  # Prevent infinite loops

    while steps < max_steps and sim.current_instruction < len(sim.instructions):
        if sim.current_instruction == breakpoint:
            break
        if not sim.step():
            break
        steps += 1

    return jsonify({
        'success': True,
        'state': sim.get_state(),
        'steps_executed': steps
    })

@app.route('/reset', methods=['POST'])
def reset():
    """Reset simulator to beginning"""
    data = request.json
    session_id = data.get('session_id', 'default')

    if session_id not in simulators:
        return jsonify({'success': False, 'error': 'No simulator loaded'})

    sim = simulators[session_id]
    # Restore to initial state by stepping back all the way
    while sim.step_back():
        pass

    return jsonify({
        'success': True,
        'state': sim.get_state()
    })

@app.route('/get_state', methods=['POST'])
def get_state():
    """Get current simulator state"""
    data = request.json
    session_id = data.get('session_id', 'default')

    if session_id not in simulators:
        return jsonify({'success': False, 'error': 'No simulator loaded'})

    sim = simulators[session_id]

    return jsonify({
        'success': True,
        'state': sim.get_state()
    })

if __name__ == '__main__':
    print("=" * 60)
    print("C Compiler Visualizer")
    print("Interactive tool for visualizing registers and stack execution")
    print("=" * 60)
    print("\nServer starting on http://localhost:8080")
    print("Open your browser and navigate to http://localhost:8080")
    print("\nPress Ctrl+C to stop the server")
    print("=" * 60)

    app.run(debug=True, host='0.0.0.0', port=8080)
