/**
 * Logos - Lenguaje de Logica Proposicional
 * Teoria de Compiladores 2025-2
 * 
 * Este compilador transforma programas escritos en el lenguaje Logos
 * a codigo LLVM IR, con soporte para optimizaciones y ejecucion JIT.
 */

#include <iostream>
#include <fstream>

#include "LogosDriver.h"
#include "LogosLexer.h"
#include "LogosParser.h"

using namespace antlr4;
using namespace std;

void printUsage(const char* programName) {
  cerr << "Logos - Compilador de Logica Proposicional" << endl;
  cerr << endl;
  cerr << "Uso: " << programName << " <archivo.logos> [opt] [jit]" << endl;
  cerr << endl;
  cerr << "Argumentos:" << endl;
  cerr << "  <archivo.logos>  : Archivo fuente con codigo Logos" << endl;
  cerr << "  opt (opcional)   : Activar optimizaciones LLVM" << endl;
  cerr << "  jit (opcional)   : Ejecutar con JIT (Just-In-Time)" << endl;
  cerr << endl;
  cerr << "Ejemplos:" << endl;
  cerr << "  " << programName << " example.logos          # Generar IR sin optimizar" << endl;
  cerr << "  " << programName << " example.logos opt      # Generar IR optimizado" << endl;
  cerr << "  " << programName << " example.logos jit      # Ejecutar con JIT" << endl;
  cerr << "  " << programName << " example.logos opt jit  # Optimizar y ejecutar" << endl;
  cerr << endl;
  cerr << "Operadores soportados:" << endl;
  cerr << "  &&   : AND (conjuncion)" << endl;
  cerr << "  ||   : OR (disyuncion)" << endl;
  cerr << "  ^^   : XOR (disyuncion exclusiva)" << endl;
  cerr << "  !    : NOT (negacion)" << endl;
  cerr << "  =>   : IMPLIES (implicacion)" << endl;
  cerr << "  <=>  : IFF (bicondicional)" << endl;
}

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    printUsage(argv[0]);
    return 1;
  }

  // Abrir archivo fuente
  ifstream ifile(argv[1]);
  if (!ifile.is_open()) {
    cerr << "Error: No se pudo abrir el archivo '" << argv[1] << "'" << endl;
    return 1;
  }

  // Parsear argumentos opcionales
  bool enableOptimizations = false;
  bool enableJIT = false;
  
  for (int i = 2; i < argc; i++) {
    string arg = argv[i];
    if (arg == "opt") {
      enableOptimizations = true;
    } else if (arg == "jit") {
      enableJIT = true;
    } else {
      cerr << "Advertencia: Argumento desconocido '" << arg << "'" << endl;
    }
  }
  
  // Mostrar configuracion
  cerr << "; ========================================" << endl;
  cerr << "; Logos Compiler" << endl;
  cerr << "; ========================================" << endl;
  cerr << "; Archivo:        " << argv[1] << endl;
  cerr << "; Optimizaciones: " << (enableOptimizations ? "ACTIVADAS" : "DESACTIVADAS") << endl;
  cerr << "; JIT:            " << (enableJIT ? "ACTIVADO" : "DESACTIVADO") << endl;
  cerr << "; ========================================" << endl;
  cerr << endl;

  // Crear lexer y parser
  ANTLRInputStream input(ifile);
  LogosLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  LogosParser parser(&tokens);

  // Parsear programa
  tree::ParseTree *tree = parser.program();
  
  // Verificar errores de sintaxis
  if (parser.getNumberOfSyntaxErrors() > 0) {
    cerr << "Error: Se encontraron " << parser.getNumberOfSyntaxErrors() 
         << " errores de sintaxis" << endl;
    return 1;
  }

  // Crear driver y ejecutar visitor
  auto driver = new LogosDriver(enableOptimizations, enableJIT);
  driver->visit(tree);

  delete driver;
  ifile.close();

  return 0;
}
