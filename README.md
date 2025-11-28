# Compilador C a x86-64

Proyecto Final - CS3402 Compiladores, UTEC

## Descripcion

Compilador que traduce un subconjunto del lenguaje C a codigo ensamblador x86-64. Implementado en C++ con arquitectura modular: scanner, parser, semantic analyzer y code generator.

## Caracteristicas

### Implementacion Base
- Declaracion de variables (int)
- Operaciones aritmeticas (+, -, *, /, %)
- Operaciones relacionales (<, >, <=, >=, ==, !=)
- Operaciones logicas (&&, ||, !)
- Sentencias if-else
- Bucles while y for
- Definicion y llamada de funciones

### Implementacion Adicional: Tipos Numericos
- int: entero 32 bits con signo
- long: entero 64 bits con signo
- unsigned int: entero 32 bits sin signo
- float: punto flotante 64 bits

### Extensiones
- Expresiones ternarias (operador ?:)
- Alias de tipo (typedef)

### Optimizaciones
- Constant folding
- Dead code elimination

## Estructura del Proyecto

```
Final/
├── src/
│   ├── main.cpp
│   ├── scanner.cpp
│   ├── parser.cpp
│   ├── semantic.cpp
│   └── codegen.cpp
├── include/
│   ├── scanner.h
│   ├── parser.h
│   ├── ast.h
│   ├── semantic.h
│   └── codegen.h
├── tests/
│   ├── test_func[1-3].c    (3 casos de funciones)
│   ├── test_base[1-5].c    (5 casos base)
│   ├── test_ext[1-5].c     (5 casos extensiones)
│   └── test_opt[1-5].c     (5 casos optimizacion)
├── visualizer/             (bonus)
├── Makefile
├── run_x86.sh
└── compiler
```

## Compilacion del Compilador

```bash
make
```

Para limpiar archivos generados:
```bash
make clean
```

## Ejecucion

### Linux (x86-64)

```bash
# Compilar un archivo fuente
./compiler archivo.c

# Ensamblar y ejecutar
gcc -no-pie output.s -o program
./program
```

### macOS (Apple Silicon M1/M2/M3)

El compilador genera codigo x86-64, incompatible con ARM. Se requiere Docker.

Requisitos:
- Docker Desktop: https://www.docker.com/products/docker-desktop

```bash
# Ejecutar directamente con Docker
./run_x86.sh archivo.c
```

Si no tienes permisos:
```bash
chmod +x run_x86.sh
```

### Windows

#### Opcion 1: WSL2 (Recomendado)

Instalacion de WSL2:
```powershell
# En PowerShell como Administrador
wsl --install
# Reiniciar PC
```

En Ubuntu (WSL2):
```bash
# Instalar dependencias
sudo apt update
sudo apt install g++ make gcc -y

# Ir al proyecto
cd /mnt/c/Users/TU_USUARIO/ruta/al/proyecto/Final

# Compilar el compilador
make

# Compilar y ejecutar
./compiler tests/test_base1.c
gcc -no-pie output.s -o program
./program
```

#### Opcion 2: Docker Desktop

```powershell
# En PowerShell, desde la carpeta del proyecto

# Compilar el compilador
docker run --rm -v ${PWD}:/work -w /work gcc:latest make

# Compilar y ejecutar un test
docker run --rm -v ${PWD}:/work -w /work gcc:latest bash -c "./compiler tests/test_base1.c && gcc -no-pie output.s -o program && ./program"
```

## Casos de Prueba

### Funciones
| Test | Descripcion | Salida |
|------|-------------|--------|
| test_func1.c | Funcion suma | 15 |
| test_func2.c | Funcion cuadrado | 49 |
| test_func3.c | Factorial recursivo | 120 |

### Base
| Test | Descripcion | Salida |
|------|-------------|--------|
| test_base1.c | Aritmetica | 30, 10, 200, 2 |
| test_base2.c | If-else | 10, 1 |
| test_base3.c | While | 45 |
| test_base4.c | For | 45 |
| test_base5.c | Expresiones complejas | 11, 16, 4 |

### Extensiones
| Test | Descripcion | Salida |
|------|-------------|--------|
| test_ext1.c | Ternario | 10, 5 |
| test_ext2.c | Unsigned int | 150 |
| test_ext3.c | Long int | 3000000 |
| test_ext4.c | Float | 6.280000 |
| test_ext5.c | Typedef | 30, 1000000, 3.140000 |

### Optimizacion
| Test | Descripcion | Salida |
|------|-------------|--------|
| test_opt1.c | Constant folding | 5, 50, 25 |
| test_opt2.c | Dead code elimination | 30 |
| test_opt3.c | Strength reduction | 16, 32 |
| test_opt4.c | Common subexpression | 16 |
| test_opt5.c | Loop optimization | 100 |

## Ejecutar Todos los Tests

### Linux/WSL2
```bash
for test in tests/test_*.c; do
    echo "=== $test ==="
    ./compiler "$test"
    gcc -no-pie output.s -o program
    ./program
done
```

### macOS (Docker)
```bash
for test in tests/test_*.c; do
    echo "=== $test ==="
    ./run_x86.sh "$test"
done
```

## Visualizador (Bonus)

Herramienta web para visualizar el estado de registros y pila paso a paso.

```bash
cd visualizer
pip3 install -r requirements.txt
python3 server.py
# Abrir http://localhost:8080
```

## Arquitectura del Compilador

1. Scanner: tokenizacion del codigo fuente
2. Parser: construccion del AST (parser recursivo descendente)
3. Semantic Analyzer: verificacion de tipos, tabla de simbolos
4. Code Generator: generacion de codigo x86-64 (sintaxis Intel, System V ABI)

## Convenciones x86-64

| Registro | Uso |
|----------|-----|
| rax | Valor de retorno |
| rdi, rsi, rdx, rcx | Argumentos 1-4 |
| rbp | Base pointer |
| rsp | Stack pointer |
| xmm0-xmm1 | Punto flotante |

## Solucion de Problemas

### macOS: "Docker not running"
Abrir Docker Desktop y esperar a que inicie.

### macOS: "permission denied: ./run_x86.sh"
```bash
chmod +x run_x86.sh
```

### Windows WSL2: "command not found: make"
```bash
sudo apt install make g++ gcc -y
```

### Error: "gcc: output.s: No such file"
Primero compilar con el compilador:
```bash
./compiler archivo.c
gcc -no-pie output.s -o program
```
