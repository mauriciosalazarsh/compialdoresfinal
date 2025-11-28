#!/bin/bash

# C Compiler Visualizer - Start Script
# CS3402 Compiladores - UTEC

echo "========================================"
echo "Visualizador de Compilador C a x86-64"
echo "CS3402 Compiladores - UTEC"
echo "========================================"
echo ""

# Check if compiler exists
if [ ! -f "../compiler" ]; then
    echo "[INFO] Compilador no encontrado. Compilando..."
    cd ..
    make
    cd visualizer
    echo "[OK] Compilador construido exitosamente"
fi

# Check if Python dependencies are installed
echo "[INFO] Verificando dependencias de Python..."
python3 -c "import flask" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "[INFO] Instalando dependencias de Python..."
    pip3 install -r requirements.txt
fi

echo ""
echo "[OK] Dependencias listas"
echo ""
echo "[INFO] Iniciando servidor..."
echo ""
echo "Abra su navegador en: http://localhost:8080"
echo ""
echo "Presione Ctrl+C para detener el servidor"
echo ""
echo "========================================"

# Start the server
python3 server.py
