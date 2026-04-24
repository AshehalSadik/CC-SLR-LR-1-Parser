# CC-SLR-LR-1-Parser
A university assignment in which we are tasked with implementing SLR(1) and LR(1) parsers.

## Current milestone (SLR(1) complete)
- Reads a CFG from file (`NonTerminal -> alt1 | alt2`)
- Validates non-terminals (must start with uppercase and be length >= 2)
- Normalizes epsilon (`@` or `epsilon`) to `epsilon`
- Augments grammar with `S' -> S`
- Computes and prints FIRST and FOLLOW sets
- Builds LR(0) canonical collection (CLOSURE/GOTO)
- Builds SLR(1) ACTION/GOTO table using FOLLOW sets
- Detects shift/reduce and reduce/reduce conflicts
- Runs stack-based shift-reduce parsing with per-step trace
- Builds and prints parse tree for accepted input

## Quick run
```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
./cmake-build-debug/CC_SLR_LR_1_Parser input/grammar.txt
```

## Parse an input string
```bash
./cmake-build-debug/CC_SLR_LR_1_Parser input/grammar.txt "id + id * id"
```

