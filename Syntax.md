# Corth Syntax

## Examples:

### Print an integer to the console
`69 #`

##### Expected console output:
`69`

### Print the sum of two integers to the console
Corth uses [reverse Polish notation](https://en.wikipedia.org/wiki/Reverse_Polish_notation), which is easy to read once you understand what's going on. \
The basic gist is that the operator comes after the operands. Let's take a look. \
`34 35 + #`

##### Expected console output:
`69`

### Stack protection: The Validator
Corth has a built-in validator that will not allow you to do things that may crash your computer by accessing the wrong memory. \
It will trigger whenever you try to pop off the stack before pushing anything, for example. \
`#`

This program would compile to nothing but the boiler-plate assembly, as the validator would remove invalid tokens (such as the `dump` operator trying to pop off the stack before anything was pushed on to it). \
While it would compile, it would generate a warning while doing so, stating that the stack protection was invoked by the validator.

## Operators:
|Operator|Meaning|Description|
|---|---|---|
|`+`|Addition|Pop two values off the stack, then push the sum.|
|`-`|Subtraction|Pop two values off the stack, then push the difference.|
|`*`|Multiplication|Pop two values off the stack, then push the product.|
|`/`|Division|Pop two values off the stack, then push the quotient.|
|`#`|Dump|Pop one value off the stack, then print it to the console.|

## Proposed Operators:
|Operator Candidates|Meaning|Description|
|---|---|---|
|`.`|Peek|Print the value at the top of the stack without popping.|

## Proposed Features:
- Keywords
  - Reserved words that the compiler will give special meaning (i.e. `if`, `for`, etc).
  - Proposed Keywords:
    - Operator replacements (give the option to use either a `+` or "plus", etc).
	- Conditional branching (if-else-endif)

## Added Features:
- [DONE] Update the PrintUsage() to accurately depict the different flags and options as well as the difference between them.
  - A flag is a command line argument that has inherent meaning itself, and does not require an additional argument (i.e. `-v` or -sim)).
  - An option is a command line argument that requires an input following it (i.e. `-a nasm` or `-l ld`).
  - DONE: 16:13 on 2021-26-12