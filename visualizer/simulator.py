"""
X86-64 Assembly Simulator (AT&T Syntax)
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
        self.instruction_lines = []  # Map instruction index to original line number
        self.current_instruction = 0

        # Execution history for stepping backward
        self.history = []

        # Call stack for function tracking
        self.call_stack = []

        # Variables mapping (address -> name)
        self.variables = {}

        # Program output (captured from printf calls)
        self.output = []

    def load_assembly(self, assembly_code: str):
        """Load assembly code and parse it"""
        lines = assembly_code.strip().split('\n')
        self.instructions = []
        self.instruction_lines = []
        self.labels = {}
        in_data_section = False
        instruction_index = 0

        for original_line_index, line in enumerate(lines):
            line = line.strip()

            # Skip empty lines and comments
            if not line or line.isspace() or line.startswith('#') or line.startswith(';'):
                continue

            # Check for section directives
            if '.data' in line:
                in_data_section = True
                continue
            elif '.text' in line or '.global' in line or '.att_syntax' in line or '.section' in line:
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
            self.instruction_lines.append(original_line_index)
            instruction_index += 1

        # Start at main if it exists, otherwise start at 0
        self.current_instruction = self.labels.get('main', 0)

    def save_state(self):
        """Save current state to history"""
        state = {
            'registers': self.registers.copy(),
            'xmm_registers': self.xmm_registers.copy(),
            'flags': self.flags.copy(),
            'stack': self.stack.copy(),
            'memory': self.memory.copy(),
            'current_instruction': self.current_instruction,
            'call_stack': self.call_stack.copy(),
            'output': self.output.copy()
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
        self.output = state['output']
        return True

    def parse_register(self, reg: str) -> str:
        """Parse AT&T register (remove % prefix) and normalize to 64-bit"""
        reg = reg.strip()
        if reg.startswith('%'):
            reg = reg[1:]

        # Convert 32-bit to 64-bit
        reg32_to_64 = {
            'eax': 'rax', 'ebx': 'rbx', 'ecx': 'rcx', 'edx': 'rdx',
            'esi': 'rsi', 'edi': 'rdi', 'ebp': 'rbp', 'esp': 'rsp',
            'r8d': 'r8', 'r9d': 'r9', 'r10d': 'r10', 'r11d': 'r11',
            'r12d': 'r12', 'r13d': 'r13', 'r14d': 'r14', 'r15d': 'r15',
            # 8-bit registers
            'al': 'rax', 'bl': 'rbx', 'cl': 'rcx', 'dl': 'rdx',
            'sil': 'rsi', 'dil': 'rdi',
        }
        return reg32_to_64.get(reg, reg)

    def parse_immediate(self, imm: str) -> int:
        """Parse AT&T immediate value (remove $ prefix)"""
        imm = imm.strip()
        if imm.startswith('$'):
            imm = imm[1:]
        return int(imm)

    def get_address(self, operand: str) -> int:
        """Parse memory operand and return the address"""
        operand = operand.strip()
        match = re.match(r'(-?\d+)?\(%(\w+)\)', operand)
        if match:
            offset_str, reg = match.groups()
            offset = int(offset_str) if offset_str else 0
            reg = self.parse_register('%' + reg)
            return self.registers.get(reg, 0) + offset
        return 0

    def get_value(self, operand: str) -> int:
        """Get value from operand (register, memory, or immediate) - AT&T syntax"""
        operand = operand.strip()

        # Immediate value with $ prefix
        if operand.startswith('$'):
            return self.parse_immediate(operand)

        # Register with % prefix
        if operand.startswith('%'):
            reg = self.parse_register(operand)
            if reg in self.registers:
                return self.registers[reg]
            return 0

        # Memory reference: offset(%reg) or (%reg)
        # Pattern: optional_offset(%reg)
        match = re.match(r'(-?\d+)?\(%(\w+)\)', operand)
        if match:
            offset_str, reg = match.groups()
            offset = int(offset_str) if offset_str else 0
            reg = self.parse_register('%' + reg)
            addr = self.registers.get(reg, 0) + offset
            return self.stack.get(addr, 0)

        # Label with RIP-relative: label(%rip)
        match = re.match(r'([.\w]+)\(%rip\)', operand)
        if match:
            label = match.group(1)
            return 0  # Placeholder for data section

        # Plain number (shouldn't happen in AT&T but handle it)
        if operand.lstrip('-').isdigit():
            return int(operand)

        return 0

    def set_value(self, operand: str, value):
        """Set value to operand (register or memory) - AT&T syntax"""
        operand = operand.strip()

        # Register with % prefix
        if operand.startswith('%'):
            reg = self.parse_register(operand)
            if reg in self.registers:
                # Handle float values (preserve them for simulation)
                if isinstance(value, float):
                    self.registers[reg] = value
                    return
                # For 32-bit registers, zero-extend
                if operand[1:] != reg and not operand[1:].startswith('r'):
                    value = value & 0xFFFFFFFF
                self.registers[reg] = value & 0xFFFFFFFFFFFFFFFF
                return

        # Memory reference: offset(%reg) or (%reg)
        match = re.match(r'(-?\d+)?\(%(\w+)\)', operand)
        if match:
            offset_str, reg = match.groups()
            offset = int(offset_str) if offset_str else 0
            reg = self.parse_register('%' + reg)
            addr = self.registers.get(reg, 0) + offset
            self.stack[addr] = value
            return

    def strip_suffix(self, opcode: str) -> str:
        """Remove size suffix from opcode (q, l, w, b)"""
        if opcode and opcode[-1] in 'qlwb' and len(opcode) > 1:
            # Don't strip if it's part of the instruction name
            base = opcode[:-1]
            if base in ['mov', 'add', 'sub', 'imul', 'idiv', 'push', 'pop',
                       'cmp', 'test', 'xor', 'and', 'or', 'lea', 'inc', 'dec',
                       'neg', 'movzb', 'call', 'ret', 'jmp', 'cqt']:
                return base
        return opcode

    def execute_instruction(self, instruction: str) -> bool:
        """Execute a single instruction (AT&T syntax). Returns False if execution should stop."""
        parts = instruction.split(None, 1)
        if not parts:
            return True

        opcode = self.strip_suffix(parts[0].lower())
        operands_str = parts[1] if len(parts) > 1 else ''

        # Parse operands (handle commas inside parentheses)
        operands = []
        if operands_str:
            # Split by comma but not inside parentheses
            depth = 0
            current = ''
            for char in operands_str:
                if char == '(':
                    depth += 1
                elif char == ')':
                    depth -= 1
                elif char == ',' and depth == 0:
                    operands.append(current.strip())
                    current = ''
                    continue
                current += char
            if current.strip():
                operands.append(current.strip())

        # AT&T syntax: src, dest (opposite of Intel)
        # MOV instruction (skip if xmm registers involved - handled in SSE section)
        if opcode == 'mov' and '%xmm' not in operands_str:
            src, dest = operands[0], operands[1]
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

        # ADD instruction (AT&T: src, dest -> dest = dest + src)
        elif opcode == 'add':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 + val2
            self.set_value(dest, result)

        # SUB instruction (AT&T: src, dest -> dest = dest - src)
        elif opcode == 'sub':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 - val2
            self.set_value(dest, result)

        # IMUL instruction (AT&T: src, dest -> dest = dest * src)
        elif opcode == 'imul':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 * val2
            self.set_value(dest, result)

        # IDIV instruction
        elif opcode == 'idiv':
            divisor = self.get_value(operands[0])
            if divisor != 0:
                dividend = self.registers['rax']
                self.registers['rax'] = dividend // divisor
                self.registers['rdx'] = dividend % divisor

        # INC instruction
        elif opcode == 'inc':
            val = self.get_value(operands[0])
            self.set_value(operands[0], val + 1)

        # DEC instruction
        elif opcode == 'dec':
            val = self.get_value(operands[0])
            self.set_value(operands[0], val - 1)

        # NEG instruction
        elif opcode == 'neg':
            val = self.get_value(operands[0])
            self.set_value(operands[0], -val)

        # CMP instruction (AT&T: src, dest -> compares dest - src)
        elif opcode == 'cmp':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 - val2
            self.flags['ZF'] = 1 if result == 0 else 0
            self.flags['SF'] = 1 if result < 0 else 0
            self.flags['CF'] = 1 if val1 < val2 else 0

        # TEST instruction
        elif opcode == 'test':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
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
                self.current_instruction = self.labels[label] - 1

        # CALL instruction
        elif opcode == 'call':
            label = operands[0]
            # Push return address
            self.registers['rsp'] -= 8
            self.stack[self.registers['rsp']] = self.current_instruction + 1
            self.call_stack.append(label)

            # Capture printf output
            # Capture printf output
            if label == 'printf' or label == 'printf@PLT':
                # Check if float (num vector args in %eax)
                num_vector_args = self.registers.get('rax', 0)
                
                if num_vector_args >= 1:
                     value = self.xmm_registers.get('xmm0', 0.0)
                     self.output.append(f"{value:.6f}")
                else:
                    value = self.registers.get('rsi', 0)
                    # Handle signed 64-bit values
                    if value > 0x7FFFFFFFFFFFFFFF:
                        value = value - 0x10000000000000000
                    elif value > 0x7FFFFFFF and value <= 0xFFFFFFFF:
                        value = value - 0x100000000
                    self.output.append(str(value))

            if label in self.labels:
                self.current_instruction = self.labels[label] - 1

        # RET instruction
        elif opcode == 'ret':
            if self.call_stack:
                self.call_stack.pop()
            return_addr = self.stack.get(self.registers['rsp'], 0)
            self.registers['rsp'] += 8
            if return_addr == 0:
                return False
            self.current_instruction = return_addr - 1

        # LEAVE instruction
        elif opcode == 'leave':
            # mov %rbp, %rsp
            self.registers['rsp'] = self.registers['rbp']
            # pop %rbp
            self.registers['rbp'] = self.stack.get(self.registers['rsp'], 0)
            self.registers['rsp'] += 8

        # LEA instruction (AT&T: src, dest)
        elif opcode == 'lea':
            src, dest = operands[0], operands[1]
            # Handle offset(%reg)
            match = re.match(r'(-?\d+)?\(%(\w+)\)', src)
            if match:
                offset_str, reg = match.groups()
                offset = int(offset_str) if offset_str else 0
                reg = self.parse_register('%' + reg)
                addr = self.registers.get(reg, 0) + offset
                self.set_value(dest, addr)
            # Handle label(%rip)
            elif '(%rip)' in src:
                self.set_value(dest, 0x1000)  # Fake address for strings

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

            self.set_value(dest, result)

        # MOVZB - move with zero extension (AT&T: movzbq %al, %rax)
        elif opcode == 'movzb':
            src, dest = operands[0], operands[1]
            value = self.get_value(src) & 0xFF
            self.set_value(dest, value)

        # CQT/CQTO - sign extension (AT&T name for cqo)
        elif opcode in ['cqt', 'cqto', 'cltq']:
            if self.registers['rax'] < 0:
                self.registers['rdx'] = -1
            else:
                self.registers['rdx'] = 0

        # XOR instruction (AT&T: src, dest)
        elif opcode == 'xor':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 ^ val2
            self.set_value(dest, result)
            self.flags['ZF'] = 1 if result == 0 else 0

        # AND instruction (AT&T: src, dest)
        elif opcode == 'and':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 & val2
            self.set_value(dest, result)

        # OR instruction (AT&T: src, dest)
        elif opcode == 'or':
            src, dest = operands[0], operands[1]
            val1 = self.get_value(dest)
            val2 = self.get_value(src)
            result = val1 | val2
            self.set_value(dest, result)

        # SSE instructions (simplified)
        elif opcode.startswith('movsd') or (opcode in ['mov', 'movq'] and '%xmm' in operands_str):
            src, dest = operands[0], operands[1]
            val = 0.0

            # Get source value
            if '%xmm' in src:
                reg = src.replace('%', '')
                val = self.xmm_registers.get(reg, 0.0)
            elif '(%rip)' in src:
                # Load from data section
                label = src.split('(')[0]
                if label in self.data_section:
                    data_str = self.data_section[label]
                    if '.double' in data_str:
                        try:
                            val = float(data_str.replace('.double', '').strip())
                        except:
                            pass
                    elif '.long' in data_str or '.int' in data_str:
                         try:
                            val = float(data_str.replace('.long', '').replace('.int', '').strip())
                         except:
                            pass
            elif src.startswith('$'):
                # Immediate
                try:
                    val = float(src.replace('$', ''))
                except:
                    pass
            elif re.match(r'-?\d+\(%\w+\)', src):
                # Memory reference like -8(%rbp)
                addr = self.get_address(src)
                val = self.stack.get(addr, 0.0)
            elif '%' in src: # GPR
                reg = self.parse_register(src)
                val = self.registers.get(reg, 0)

            # Store to dest
            if '%xmm' in dest:
                reg = dest.replace('%', '')
                self.xmm_registers[reg] = float(val) if not isinstance(val, float) else val
            elif re.match(r'-?\d+\(%\w+\)', dest):
                # Memory reference like -8(%rbp)
                addr = self.get_address(dest)
                self.stack[addr] = val
            elif '%' in dest: # GPR - preserve float value for simulation purposes
                reg = self.parse_register(dest)
                self.registers[reg] = val

        elif opcode in ['addsd', 'subsd', 'mulsd', 'divsd']:
            src, dest = operands[0], operands[1]
            src_reg = src.replace('%', '')
            dest_reg = dest.replace('%', '')

            val1 = self.xmm_registers.get(dest_reg, 0.0)
            val2 = self.xmm_registers.get(src_reg, 0.0)

            if opcode == 'addsd':
                self.xmm_registers[dest_reg] = val1 + val2
            elif opcode == 'subsd':
                self.xmm_registers[dest_reg] = val1 - val2
            elif opcode == 'mulsd':
                self.xmm_registers[dest_reg] = val1 * val2
            elif opcode == 'divsd':
                if val2 != 0:
                    self.xmm_registers[dest_reg] = val1 / val2

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
        stack_view = []
        for addr in sorted(self.stack.keys(), reverse=True):
            stack_view.append({
                'address': hex(addr),
                'value': self.stack[addr],
                'is_rbp': addr == self.registers['rbp'],
                'is_rsp': addr == self.registers['rsp'],
                'rbp_offset': addr - self.registers['rbp'] if self.registers['rbp'] != 0 else None
            })

        return {
            'registers': {k: hex(v) if isinstance(v, int) else v for k, v in self.registers.items()},
            'xmm_registers': self.xmm_registers,
            'flags': self.flags,
            'stack': stack_view,
            'current_instruction': self.current_instruction,
            'current_line_number': self.instruction_lines[self.current_instruction] if self.current_instruction < len(self.instructions) else -1,
            'instruction': self.instructions[self.current_instruction] if self.current_instruction < len(self.instructions) else 'END',
            'call_stack': self.call_stack,
            'can_step_back': len(self.history) > 0,
            'can_step_forward': self.current_instruction < len(self.instructions),
            'output': self.output
        }
