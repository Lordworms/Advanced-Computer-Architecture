#include <fstream>
#include <iostream>

#include "pin.H"
std::ofstream outFile;

KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
                                 "results.log",
                                 "Output file for the memops tool");

VOID RecordMemOp(VOID *ip, VOID *addr, UINT32 size, BOOL isWrite) {
  outFile << (isWrite ? "MW," : "MR,") << ip << "," << addr << "," << size
          << std::endl;
}

VOID Trace(TRACE trace, VOID *v) {
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {
      UINT32 memOperands = INS_MemoryOperandCount(ins);
      for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
        UINT32 refSize = INS_MemoryOperandSize(ins, memOp);
        if (INS_MemoryOperandIsRead(ins, memOp)) {
          INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemOp,
                                   IARG_INST_PTR, IARG_MEMORYOP_EA, memOp,
                                   IARG_UINT32, refSize, IARG_BOOL, FALSE,
                                   IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp)) {
          INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemOp,
                                   IARG_INST_PTR, IARG_MEMORYOP_EA, memOp,
                                   IARG_UINT32, refSize, IARG_BOOL, TRUE,
                                   IARG_END);
        }
      }
    }
  }
}

VOID Fini(INT32 code, VOID *v) {
  if (outFile.is_open()) {
    outFile.close();
  }
}
int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; ++i) {
    std::cout << "the " << i << "th argument is " << argv[i] << '\n';
  }
  if (PIN_Init(argc, argv)) {
    std::cerr << "This tool logs memory operations of a program." << std::endl;
    std::cerr << "Usage: pin -t obj-intel64/memops.so -o <output file> -- "
                 "<target program>"
              << std::endl;
    return 1;
  }
  outFile.open(KnobOutputFile.Value().c_str());

  TRACE_AddInstrumentFunction(Trace, 0);
  PIN_AddFiniFunction(Fini, 0);
  PIN_StartProgram();

  return 0;
}