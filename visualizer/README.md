# 🔍 Kotlin Compiler Visualizer

**Herramienta interactiva para visualizar el estado de los registros y la pila de ejecución paso a paso**

Esta aplicación web permite depurar y entender el código ensamblador x86-64 generado por el compilador de Kotlin de manera visual e interactiva.

## 🌟 Características

### Visualización Completa
- ✅ **Registros x86-64**: Visualización en tiempo real de todos los registros (RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15)
- ✅ **Flags**: Estado de los flags del procesador (ZF, SF, CF, OF)
- ✅ **Pila de Ejecución**: Vista detallada de la pila con indicadores de RSP y RBP
- ✅ **Código Ensamblador**: Resaltado de la instrucción actual

### Controles de Ejecución
- ▶️ **Step Forward**: Ejecuta una instrucción a la vez
- ⏪ **Step Back**: Retrocede a la instrucción anterior (con historial completo)
- ⏩ **Run All**: Ejecuta todo el programa hasta el final
- 🔄 **Reset**: Reinicia la ejecución desde el principio

### Interfaz Moderna
- 🎨 Diseño moderno y responsivo
- 🌈 Colores que resaltan cambios en registros
- 📱 Compatible con dispositivos móviles
- ⚡ Actualización en tiempo real

## 📋 Requisitos

- Python 3.8 o superior
- Flask y dependencias (ver requirements.txt)
- Compilador de Kotlin (debe estar compilado en el directorio padre)

## 🚀 Instalación

### 1. Instalar dependencias de Python

```bash
cd visualizer
pip install -r requirements.txt
```

### 2. Verificar que el compilador esté compilado

```bash
cd ..
make
```

## ▶️ Uso

### 1. Iniciar el servidor

```bash
cd visualizer
python server.py
```

El servidor se iniciará en `http://localhost:5000`

### 2. Abrir en el navegador

Abre tu navegador y navega a:
```
http://localhost:5000
```

### 3. Usar la aplicación

1. **Escribir código Kotlin** en el editor (o usar el ejemplo por defecto)
2. **Hacer clic en "Compile"** para compilar el código
3. **Hacer clic en "Load Assembly"** para cargar el código en el simulador
4. **Usar los controles** para ejecutar paso a paso:
   - **Step Forward**: Avanza una instrucción
   - **Step Back**: Retrocede una instrucción
   - **Run All**: Ejecuta todo
   - **Reset**: Reinicia

## 🎯 Ejemplo de Uso

```kotlin
fun main() {
    var x: Int = 10
    var y: Int = 20
    var sum: Int = x + y
    println(sum)
}
```

### Visualización:

1. **Compilar** el código
2. **Cargar** el ensamblador
3. **Observar** cómo:
   - Se inicializan las variables
   - Se cargan valores en registros
   - Se realizan operaciones aritméticas
   - Se gestiona la pila
   - Se llaman funciones

## 📊 Capturas de Funcionalidades

### Panel de Registros
Muestra todos los registros x86-64 con:
- Valores en hexadecimal
- Resaltado de cambios (verde cuando cambia)
- Agrupación por tipo

### Panel de Pila
Visualiza la pila con:
- Direcciones de memoria
- Valores almacenados
- Indicadores de RSP (Stack Pointer) - amarillo
- Indicadores de RBP (Base Pointer) - azul

### Panel de Código
- Editor de código Kotlin con syntax highlighting
- Visualización del código ensamblador generado
- Resaltado de la instrucción actual (azul)

## 🛠️ Arquitectura

```
visualizer/
├── server.py          # Servidor Flask
├── simulator.py       # Simulador de x86-64
├── requirements.txt   # Dependencias Python
├── static/
│   ├── app.js        # Lógica de la aplicación
│   └── style.css     # Estilos CSS
└── templates/
    └── index.html    # Página principal
```

### Flujo de Datos:

1. **Frontend (HTML/JS)** → Envía código Kotlin
2. **Servidor (Flask)** → Llama al compilador
3. **Compilador (C++)** → Genera ensamblador
4. **Simulador (Python)** → Interpreta y ejecuta
5. **Frontend** → Visualiza el estado

## 🎓 Uso Educativo

Esta herramienta es ideal para:

- ✅ **Aprender ensamblador x86-64**: Ver cómo se traducen construcciones de alto nivel
- ✅ **Depurar código**: Encontrar errores en la generación de código
- ✅ **Entender la pila**: Visualizar push/pop y frames de funciones
- ✅ **Optimización**: Comparar diferentes implementaciones
- ✅ **Enseñanza**: Demostrar conceptos de compiladores

## 🔧 Características Técnicas

### Simulador x86-64
- Interpreta instrucciones ensamblador
- Mantiene estado completo de registros
- Simula la pila de memoria
- Soporta saltos y llamadas a funciones
- Historial completo para stepping backward

### Instrucciones Soportadas
- Movimiento: `mov`, `push`, `pop`, `lea`
- Aritmética: `add`, `sub`, `imul`, `idiv`, `inc`, `dec`, `neg`
- Lógica: `and`, `or`, `xor`, `test`
- Comparación: `cmp`
- Saltos: `jmp`, `je`, `jz`, `jne`, `jnz`, `jl`, `jle`, `jg`, `jge`
- Control: `call`, `ret`
- Set: `setl`, `setle`, `setg`, `setge`, `sete`, `setne`
- Extensión: `movzx`, `cdq`, `cqo`, `cdqe`

## 🎁 Bonus del Proyecto

Esta herramienta cumple con los requisitos del **bonus de 3 puntos** del proyecto:

> "Si algún grupo desea implementar una app para su compilador, que incluya una
> herramienta interactiva para visualizar el estado de los registros y la pila de ejecución
> paso a paso, se otorgará un bonus de hasta 3 puntos a cada integrante en la
> calificación del Examen 3."

### Características del Bonus Implementadas:
- ✅ Herramienta interactiva completa
- ✅ Visualización de todos los registros
- ✅ Visualización de la pila de ejecución
- ✅ Ejecución paso a paso (forward y backward)
- ✅ Interfaz web moderna y responsive
- ✅ Integración completa con el compilador

## 🐛 Troubleshooting

### Error: "No module named 'flask'"
```bash
pip install -r requirements.txt
```

### Error: "Compiler not found"
Asegúrate de que el compilador esté compilado:
```bash
cd ..
make
```

### El servidor no inicia
Verifica que el puerto 5000 no esté en uso:
```bash
lsof -i :5000
```

## 📝 Notas

- El simulador es una implementación simplificada de x86-64
- Algunas instrucciones complejas están simplificadas
- La memoria es simulada (no usa memoria real del sistema)
- Los floats se manejan de forma simplificada

## 🚀 Mejoras Futuras

- [ ] Soporte para breakpoints en líneas específicas
- [ ] Exportar historial de ejecución
- [ ] Comparación side-by-side de ejecuciones
- [ ] Gráficos de uso de registros
- [ ] Detección de optimizaciones potenciales
- [ ] Soporte para SSE/AVX instructions completas

## 👥 Autor

Proyecto desarrollado para CS3402 - Compiladores
Universidad de Ingeniería y Tecnología (UTEC)

## 📄 Licencia

Este proyecto es parte del curso de Compiladores y está destinado para uso educativo.
