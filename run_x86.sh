#!/bin/bash
# Script to compile and run x86-64 assembly on macOS ARM using Docker

if [ -z "$1" ]; then
    echo "Uso: ./run_x86.sh <archivo.c>"
    echo "Ejemplo: ./run_x86.sh tests/test_base1.c"
    exit 1
fi

# Compile the source file
./compiler "$1"
if [ $? -ne 0 ]; then
    echo "Error de compilacion"
    exit 1
fi

# Run in Docker with x86-64 emulation
echo ""
echo "Ejecutando en Docker (x86-64)..."
echo "================================"
docker run --rm --platform linux/amd64 -v "$(pwd)":/work -w /work gcc:latest bash -c "gcc -no-pie output.s -o program && ./program"
