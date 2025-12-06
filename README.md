# λóγος (Logos) – Lenguaje de Programación para Lógica Proposicional

λóγος (Logos) es un **lenguaje de programación de dominio específico (DSL)** orientado a la **evaluación de expresiones lógicas y booleanas**, desarrollado con fines académicos. El compilador ha sido implementado en **C++**, utilizando **ANTLR4 para el análisis sintáctico** y **LLVM para la generación y ejecución de código intermedio (LLVM IR)** con soporte **Just-In-Time (JIT)**.

El proyecto permite validar proposiciones lógicas, reglas de inferencia y simular circuitos digitales básicos como sumadores, multiplexores y comparadores.

## Objetivo del proyecto

Diseñar e implementar un compilador académico moderno que permita comprender de forma integral los procesos de análisis léxico, sintáctico y semántico, así como la generación de código intermedio y su ejecución mediante LLVM.
---

## Características principales

- Operaciones lógicas:
  - AND (`&&`)
  - OR (`||`)
  - NOT (`!`)
  - XOR (`^^`)
- Operadores avanzados:
  - Implicación (`=>`)
  - Bicondicional (`<=>`)
- Validación de:
  - Leyes de De Morgan
  - Modus Ponens y Modus Tollens
  - Silogismos
  - Falacias lógicas
- Simulación de **circuitos lógicos combinacionales**:
  - Half Adder
  - Full Adder
  - Multiplexor 2:1
  - Comparadores
- Soporte para:
  - Variables booleanas
  - Condicionales (`if`)
  - Bucles (`while`)
- Generación de **LLVM IR**
- Ejecución mediante **JIT**
- Tabla de símbolos con control semántico
- Detección de **errores léxicos, sintácticos y semánticos**

---

##  Tecnologías utilizadas

- **C++17**
- **ANTLR4**
- **LLVM**
- **CMake**
- **Clang / GCC**

---

## Estructura del proyecto

logos-lang/
│── src/
│ │── CMakeLists.txt
│ │── Logos.g4 # Gramática del lenguaje (ANTLR4)
│ │── LogosDriver.h # Driver del compilador
│ │── LogosMain.cpp # Punto de entrada del programa
│ │── FindANTLR.cmake # Módulo de CMake para ANTLR
│ │── problema1.logos # Casos de prueba válidos
│ │── problema2.logos
│ │── problema3.logos
│ │── problema4.logos
│ │── problemaincorrecto1.logos # Casos de prueba con errores
│ │── problemaincorrecto2.logos
│ │── .gitignore
│
└── README.md

---

## Requisitos

- C++17 o superior  
- ANTLR4 (versión 4.x)  
- LLVM (14 o superior recomendado)  
- CMake 3.20+  
- Clang o GCC  

---

## Compilación

Desde la carpeta raíz del proyecto:

```bash
mkdir build
cd build
cmake ../src
make

Ejecución

./Logos archivo.logos jit
Ejemplo:
./Logos problema1.logos jit
