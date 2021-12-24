
# Corth
Corth is like Porth, which is like Forth, but it's in Python, but it's in C++, and it compiles directly to executable on multiple platforms (not MacOS (yet)).

## How to write in Corth
Corth, like Porth (like Forth), is a stack based language. \
This means that in order to do any operation, a value must be pushed onto the stack. \
Think of this as a literal stack of objects. \
Likely, the operation will take it's arguments from the top of the stack, or `pop` from the stack.

Take a look at this example in Corth: `34 35 + #` \
It outlines a basic Corth program that will add two numbers and then display them to the console. \
The expected output for the above program when run is `69` \
Let's break down how it works piece-by-piece:
|Step|Code|Description|
|---|---|---|
|1|`34`| A value is pushed on to the stack, meaning stacked on top. |
|2|`35`| Another value is pushed on to the stack. |
|3|`+`| The `+` symbol will pop the two most-recent values off the stack, add them, then push the sum on to the stack. |
|4|`#`| The `#` symbol will dump from the stack, aka pop a value off and then print it to the console. |

Stack breakdown by step:
1. { 34 }
2. { 34, 35 }
3. { 69 }
4. { }

Best practices in Corth indicate that the stack should be empty by the end of the program.

### Definitions:
| Operator | Meaning | Description |
|---|---|---|
|`#`| Dump | Humankind's best friend; pops the most recent value off the stack, then prints that to the console. |
|`+`| Addition | Pops the two most-recent values off the stack, then pushes the sum. |
|`-`| Subtraction | Pops the two most-recent values off the stack, then pushes the difference. |
|`*`| Multiplication | Pops the two most-recent values off the stack, then pushes the product. |
|`/`| Division | Pops the two most-recent values off the stack, then pushes the quotient. |