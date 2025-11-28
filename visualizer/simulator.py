"""
X86-64 Assembly Simulator
Simulates execution of x86-64 assembly instructions step by step
"""

import re
from typing import Dict, List, Tuple, Any

class X86Simulator:
    def __init__(self):
        # 64-bit registers
        self.registers = {
            'rax': 0, 'rbx': 0, 'rcx': 0, 'rdx': 0,
            'rsi': 0, 'rdi': 0, 'rbp': 0, 'rsp': 0x7fff_ffff_fff0,
            'r8': 0, 'r9': 0, 'r10': 0, 'r11': 0,
            'r12': 0, 'r13': 0, 'r14': 0, 'r15': 0,
            'rip': 0  # Instruction pointer
        }

        # SSE registers for floating point
        self.xmm_registers = {f'xmm{i}': 0.0 for i in range(16)}

        # Flags
        self.flags = {
            'ZF': 0,  # Zero flag
            'SF': 0,  # Sign flag
            'CF': 0,  # Carry flag
            'OF': 0   # Overflow flag
        }

        # Memory (simplified - using dict)
        self.memory = {}
        self.stack = {}  # Stack memory

        # Data section
        self.data_section = {}

        # Labels mapping
        self.labels = {}

        # Instructions list
        self.instructions = []
        self.current_instruction = 0

        # Execution history for stepping backward
        self.history = []

        # Call stack for function tracking
        self.call_stack = []

        # Variables mapping (address -> name)
        self.variables = {}

    def load_assembly(self, assembly_code: str):
        """Load assembly code and parse it"""
        lines = assembly_code.strip().split('\n')
        self.instructions = []
        self.labels = {}
        in_data_section = False
        instruction_index = 0

        for line in lines:
            line = line.strip()

            # Skip empty lines and comments
            if not line or line.startswith('#') or line.startswith(';'):
                continue

            # Check for section directives
            if '.data' in line:
                in_data_section = True
                continue
            elif '.text' in line or '.global' in line or '.intel_syntax' in line:
                in_data_section = False
                continue

            # Handle data section
            if in_data_section:
                if ':' in line:
                    parts = line.split(':', 1)
                    label = parts[0].strip()
                    if len(parts) > 1:
                        data = parts[1].strip()
                        self.data_section[label] = data
                continue

            # Handle labels
            if line.endswith(':'):
                label = line[:-1].strip()
                self.labels[label] = instruction_index
                continue

            # Store instruction
            self.instructions.append(line)
            instruction_index += 1

        self.current_instruction = 0

    def save_state(self):
        """Save current state to history"""
        state = {
            'registers': self.registers.copy(),
            'xmm_registers': self.xmm_registers.copy(),
            'flags': self.flags.copy(),
            'stack': self.stack.copy(),
            'memory': self.memory.copy(),
            'current_instruction': self.current_instruction,
            'call_stack': self.call_stack.copy()
        }
        self.history.append(state)

    def restore_state(self):
        """Restore previous state from history"""
        if not self.history:
            return False

        state = self.history.pop()
        self.registers = state['registers']
        self.xmm_registers = state['xmm_registers']
        self.flags = state['flags']
        self.stack = state['stack']
        self.memory = state['memory']
        self.current_instruction = state['current_instruction']
        self.call_stack = state['call_stack']
        return True

    def get_value(self, operand: str) -> int:
        """Get value from operand (register, memory, or immediate)"""
        operand = operand.strip()

        # Immediate value
        if operand.isdigit() or (operand.startswith('-') and operand[1:].isdigit()):
            return int(operand)

        # Register
        if operand in self.registers:
            return self.registers[operand]

        # Memory reference [reg + offset]
        if '[' in operand:
            match = re.match(r'\[(\w+)\s*([+-])\s*(\d+)\]', operand)
            if match:
                reg, op, offset = match.groups()
                addr = self.registers.get(reg, 0)
                offset_val = int(offset)
                if op == '-':
                    addr -= offset_val
                else:
                    addr += offset_val
                return self.stack.get(addr, 0)

            # Simple memory reference [reg]
            match = re.match(r'\[(\w+)\]', operand)
            if match:
                reg = match.group(1)
                addr = self.registers.get(reg, 0)
                return self.stack.get(addr, 0)

        # Label reference
        if operand in self.data_section:
            return 0  # Placeholder for data section address

        return 0

    def set_value(self, operand: str, value: int):
        """Set value to operand (register or memory)"""
        operand = operand.strip()

        # Register
        if operand in self.registers:
            self.registers[operand] = value & 0xFFFFFFFFFFFFFFFF  # 64-bit mask
            return

        # Memory reference
        if '[' in operand:
            match = re.match(r'\[(\w+)\s*([+-])\s*(\d+)\]', operand)
            if match:
                reg, op, offset = match.groups()
                addr = self.registers.get(reg, 0)
                offset_val = int(offset)
                if op == '-':
                    addr -= offset_val
                else:
                    addr += offset_val
                self.stack[addr] = value
                return

            # Simple memory reference
            match = re.match(r'\[(\w+)\]', operand)
            if match:
                reg = match.group(1)
                addr = self.registers.get(reg, 0)
                self.stack[addr] = value
                return

    def execute_instruction(self, instruction: str) -> bool:
        """Execute a single instruction. Returns False if execution should stop."""
        parts = instruction.split(None, 1)
        if not parts:
            return True

        opcode = parts[0].lower()
        operands = parts[1].split(',') if len(parts) > 1 else []
        operands = [op.strip() for op in operands]

        # MOV instruction
        if opcode == 'mov':
            dest, src = operands[0], operands[1]
            value = self.get_value(src)
            self.set_value(dest, value)

        # PUSH instruction
        elif opcode == 'push':
            value = self.get_value(operands[0])
            self.registers['rsp'] -= 8
            self.stack[self.registers['rsp']] = value

        # POP instruction
        elif opcode == 'pop':
            value = self.stack.get(self.registers['rsp'], 0)
            self.set_value(operands[0], value)
            self.registers['rsp'] += 8

        # Arithmetic instructions
        elif opcode == 'add':
            dest, src = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 + val2
            self.set_value(dest, result)

        elif opcode == 'sub':
            dest, src = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 - val2
            self.set_value(dest, result)

        elif opcode == 'imul':
            dest, src = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 * val2
            self.set_value(dest, result)

        elif opcode == 'idiv':
            divisor = self.get_value(operands[0])
            if divisor != 0:
                dividend = self.registers['rax']
                self.registers['rax'] = dividend // divisor
                self.registers['rdx'] = dividend % divisor

        elif opcode == 'inc':
            val = self.get_value(operands[0])
            self.set_value(operands[0], val + 1)

        elif opcode == 'dec':
            val = self.get_value(operands[0])
            self.set_value(operands[0], val - 1)

        elif opcode == 'neg':
            val = self.get_value(operands[0])
            self.set_value(operands[0], -val)

        # Comparison and test
        elif opcode == 'cmp':
            val1 = self.get_value(operands[0])
            val2 = self.get_value(operands[1])
            result = val1 - val2
            self.flags['ZF'] = 1 if result == 0 else 0
            self.flags['SF'] = 1 if result < 0 else 0
            self.flags['CF'] = 1 if val1 < val2 else 0

        elif opcode == 'test':
            val1 = self.get_value(operands[0])
            val2 = self.get_value(operands[1])
            result = val1 & val2
            self.flags['ZF'] = 1 if result == 0 else 0
            self.flags['SF'] = 1 if result < 0 else 0

        # Jumps
        elif opcode in ['jmp', 'je', 'jz', 'jne', 'jnz', 'jl', 'jle', 'jg', 'jge']:
            label = operands[0]
            should_jump = False

            if opcode == 'jmp':
                should_jump = True
            elif opcode in ['je', 'jz']:
                should_jump = self.flags['ZF'] == 1
            elif opcode in ['jne', 'jnz']:
                should_jump = self.flags['ZF'] == 0
            elif opcode == 'jl':
                should_jump = self.flags['SF'] != self.flags['OF']
            elif opcode == 'jle':
                should_jump = self.flags['ZF'] == 1 or self.flags['SF'] != self.flags['OF']
            elif opcode == 'jg':
                should_jump = self.flags['ZF'] == 0 and self.flags['SF'] == self.flags['OF']
            elif opcode == 'jge':
                should_jump = self.flags['SF'] == self.flags['OF']

            if should_jump and label in self.labels:
                self.current_instruction = self.labels[label] - 1  # -1 because we'll increment

        # CALL instruction
        elif opcode == 'call':
            label = operands[0]
            # Push return address
            self.registers['rsp'] -= 8
            self.stack[self.registers['rsp']] = self.current_instruction + 1
            self.call_stack.append(label)

            if label in self.labels:
                self.current_instruction = self.labels[label] - 1

        # RET instruction
        elif opcode == 'ret':
            if self.call_stack:
                self.call_stack.pop()
            # Pop return address
            return_addr = self.stack.get(self.registers['rsp'], 0)
            self.registers['rsp'] += 8
            if return_addr == 0:  # Return from main
                return False
            self.current_instruction = return_addr - 1

        # LEA instruction
        elif opcode == 'lea':
            dest = operands[0]
            src = operands[1]
            # Simplified - just load a fake address
            if '[' in src:
                match = re.match(r'\[(\w+)\s*([+-])\s*(\d+)\]', src)
                if match:
                    reg, op, offset = match.groups()
                    addr = self.registers.get(reg, 0)
                    offset_val = int(offset)
                    if op == '-':
                        addr -= offset_val
                    else:
                        addr += offset_val
                    self.set_value(dest, addr)

        # Set instructions
        elif opcode in ['setl', 'setle', 'setg', 'setge', 'sete', 'setne', 'setz']:
            dest = operands[0]
            result = 0

            if opcode == 'setl':
                result = 1 if self.flags['SF'] != self.flags['OF'] else 0
            elif opcode == 'setle':
                result = 1 if self.flags['ZF'] == 1 or self.flags['SF'] != self.flags['OF'] else 0
            elif opcode == 'setg':
                result = 1 if self.flags['ZF'] == 0 and self.flags['SF'] == self.flags['OF'] else 0
            elif opcode == 'setge':
                result = 1 if self.flags['SF'] == self.flags['OF'] else 0
            elif opcode in ['sete', 'setz']:
                result = 1 if self.flags['ZF'] == 1 else 0
            elif opcode == 'setne':
                result = 1 if self.flags['ZF'] == 0 else 0

            # Set only the lower byte
            self.set_value(dest, result)

        # MOVZX - move with zero extension
        elif opcode == 'movzx':
            dest, src = operands[0], operands[1]
            value = self.get_value(src) & 0xFF  # Get lower byte
            self.set_value(dest, value)

        # CDQ/CQO - sign extension
        elif opcode in ['cdq', 'cqo', 'cdqe']:
            if self.registers['rax'] < 0:
                self.registers['rdx'] = -1
            else:
                self.registers['rdx'] = 0

        # XOR instruction
        elif opcode == 'xor':
            dest, src = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 ^ val2
            self.set_value(dest, result)

        # AND instruction
        elif opcode == 'and':
            dest, src = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 & val2
            self.set_value(dest, result)

        # OR instruction
        elif opcode == 'or':
            dest, src = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 | val2
            self.set_value(dest, result)

        # SSE instructions (simplified)
        elif opcode.startswith('movsd') or opcode.startswith('movq'):
            # Simplified floating point handling
            pass

        return True

    def step(self) -> bool:
        """Execute one instruction. Returns False if execution finished."""
        if self.current_instruction >= len(self.instructions):
            return False

        self.save_state()

        instruction = self.instructions[self.current_instruction]
        should_continue = self.execute_instruction(instruction)

        self.current_instruction += 1

        return should_continue and self.current_instruction < len(self.instructions)

    def step_back(self) -> bool:
        """Step back one instruction"""
        return self.restore_state()

    def run_until_breakpoint(self, breakpoint_line: int = -1) -> bool:
        """Run until breakpoint or end"""
        while self.current_instruction < len(self.instructions):
            if self.current_instruction == breakpoint_line:
                return True
            if not self.step():
                return False
        return False

    def get_state(self) -> Dict[str, Any]:
        """Get current execution state for visualization"""
        # Get stack contents
        stack_view = []
        for addr in sorted(self.stack.keys(), reverse=True):
            stack_view.append({
                'address': hex(addr),
                'value': self.stack[addr],
                'is_rbp': addr == self.registers['rbp'],
                'is_rsp': addr == self.registers['rsp']
            })

        return {
            'registers': {k: hex(v) if isinstance(v, int) else v for k, v in self.registers.items()},
            'xmm_registers': self.xmm_registers,
            'flags': self.flags,
            'stack': stack_view,
            'current_instruction': self.current_instruction,
            'instruction': self.instructions[self.current_instruction] if self.current_instruction < len(self.instructions) else 'END',
            'call_stack': self.call_stack,
            'can_step_back': len(self.history) > 0,
            'can_step_forward': self.current_instruction < len(self.instructions)
        }
