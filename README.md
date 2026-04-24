# CC-SLR-LR-1-Parser

Implementation of SLR(1) and LR(1) parsers with grammar loading, augmentation, item construction, parsing-table generation, conflict reporting, shift-reduce tracing, and parse-tree generation.

## Team Members
- Member 1: `Ashehal Sadik` - Roll No: `23I-0699`
- Member 2: `Noumaan Siddiqui` - Roll No: `23I-0666`

## Programming Language
- C++17

## Compilation Instructions

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
```

## Execution Instructions

General form:

```bash
./cmake-build-debug/CC_SLR_LR_1_Parser <grammar-file> [--mode slr|lr1|both] [input tokens]
```

Examples:

```bash
./cmake-build-debug/CC_SLR_LR_1_Parser input/tests/grammar2_with_multiplication.txt
./cmake-build-debug/CC_SLR_LR_1_Parser input/tests/grammar2_with_multiplication.txt --mode slr "id + id * id"
./cmake-build-debug/CC_SLR_LR_1_Parser input/tests/grammar2_with_multiplication.txt --mode lr1 "id + id * id"
./cmake-build-debug/CC_SLR_LR_1_Parser input/tests/grammar3_classic_lr1_not_slr.txt --mode both "id = id"
```

## Input File Format Specification

### Grammar File Format
- One production per line
- Format: `NonTerminal -> alternative1 | alternative2 | ...`
- Symbols are whitespace-separated in the RHS
- Non-terminals must start with uppercase and be at least 2 characters (for this project)
- Epsilon can be written as `epsilon` or `@`

Example:

```text
Expr -> Expr + Term | Term
Term -> Term * Factor | Factor
Factor -> ( Expr ) | id
```

## Sample Commands

### Run SLR(1) Parser

```bash
./cmake-build-debug/CC_SLR_LR_1_Parser input/tests/grammar2_with_multiplication.txt --mode slr "id + id * id"
```

### Run LR(1) Parser

```bash
./cmake-build-debug/CC_SLR_LR_1_Parser input/tests/grammar2_with_multiplication.txt --mode lr1 "id + id * id"
```

## Known Limitations
- Grammar symbol tokenization is whitespace-based.
- Non-terminals like `E`, `T`, `F` are rejected by design (assignment rule requires multi-character non-terminals).
- Parse tree output is text-based (indented), not graphical.
- Timing results are machine/load dependent and should be treated as comparative indicators.

## Testing Guidelines

### Test Grammar 1 (Simple Expression)
File: `input/tests/grammar1_simple_expression.txt`

```text
Expr -> Expr + Term | Term
Term -> Factor
Factor -> id
```

### Test Grammar 2 (With Multiplication)
File: `input/tests/grammar2_with_multiplication.txt`

```text
Expr -> Expr + Term | Term
Term -> Term * Factor | Factor
Factor -> ( Expr ) | id
```

### Test Grammar 3 (Classic LR(1)-Not-SLR Example)
File: `input/tests/grammar3_classic_lr1_not_slr.txt`

```text
Start -> Left = Right | Right
Left -> * Right | id
Right -> Left
```

Expected behavior: SLR(1) conflict(s), LR(1) succeeds.

### Test Grammar 4 (Dangling Else)
File: `input/tests/grammar4_dangling_else.txt`

```text
Stmt -> if Expr then Stmt | if Expr then Stmt else Stmt | other
Expr -> id
```

### Test Requirements Checklist
- Valid strings that should be accepted:
  - `id + id` (Grammar 1)
  - `id + id * id` (Grammar 2)
  - `id = id` (Grammar 3, LR(1) mode)
- Invalid strings with syntax errors:
  - `id +` (Grammar 1/2)
  - `* = id` (Grammar 3)
- Strings that expose parser conflicts:
  - `id = id` with Grammar 3 in SLR mode
- Empty input:
  - `""` for each grammar
- Grammar demonstrating LR(1) superiority over SLR(1):
  - Grammar 3
- Complex nested structures:
  - `( id + id ) * id` (Grammar 2)
  - `if id then if id then other else other` (Grammar 4)

