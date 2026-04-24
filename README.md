# CC-SLR-LR-1-Parser
A university assignment in which we are tasked with implementing SLR(1) and LR(1) parsers.

## Current milestone (Step 1)
- Reads a CFG from file (`NonTerminal -> alt1 | alt2`)
- Validates non-terminals (must start with uppercase and be length >= 2)
- Normalizes epsilon (`@` or `epsilon`) to `epsilon`
- Augments grammar with `S' -> S`
- Computes and prints FIRST and FOLLOW sets

## Quick run
```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
./cmake-build-debug/CC_SLR_LR_1_Parser input/grammar.txt
```

