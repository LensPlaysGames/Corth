# Corth
It's like [Porth](https://gitlab.com/tsoding/porth/-/tree/master/), which is like [Forth](https://www.forth.com/forth/) but written in Python, but written in C++. But I don't actually know for sure since I never programmed in Porth or Forth, I only heard that they are some sort of stack-based programming language. Corth is also stack-based programming language. Which makes it just like Porth, just like Forth, am I rite?

Corth is:
- [x] A [stack based](https://en.wikipedia.org/wiki/Stack-oriented_programming) programming language
- [x] [Turing complete](https://en.wikipedia.org/wiki/Turing_completeness) (see [rule 110 example](./examples/rule110.corth))
- [x] Compileable to a dynamic executable linked with the C RunTime, [cross-platform](#cross-platform-anchor)
- [x] Totally awesome

## Get the latest release [here](https://github.com/LensPlaysGames/Corth/releases)

### [How To Use](#how-to-use-anchor)

###### WARNING: Corth uses the [system](https://en.cppreference.com/w/cpp/utility/program/system) function within C++ to run external commands on your computer. These commands can be affected by user input, so running corth has the potential to run any command on your system if you tell it to, including malicious ones. Be sure to check and double check any commands you see that use the `-a` or `-l` compiler options, as these tell Corth to run a different command than the default. Every command run by Corth is echoed to the standard out with a '[CMD]' prefix.

---

## How to write in Corth
Corth, like Porth (like Forth), is a stack based language. \
This means that in order to do any operation, a value must be pushed onto the stack. \
Think of this as a literal stack of objects. \
Likely, the operation will take it's arguments from the top of the stack, or `pop` from the stack, and sometimes it will return a value or two back onto it.

### A simple example
Take a look at this example in Corth: \
`34 35 + #` \
It is a basic Corth program that will add two numbers together, and then display the sum to the console.

Expected output for the above program when run: \
<samp>69</samp>

Let's break down how it works piece-by-piece:
| Step | Code | Description                                                                                                                     |
|------|------|---------------------------------------------------------------------------------------------------------------------------------|
|    1 | `34` | A value is pushed on to the stack, meaning stacked on top.                                                                      |
|    2 | `35` | Another value is pushed on to the stack.                                                                                        |
|    3 |  `+` | The `+` symbol will pop the two most-recent values off the stack, add them, then push the sum on to the stack.                  |
|    4 |  `#` | The `#` symbol will dump from the stack, aka pop a value off and then print it to the console.                                  |

Stack breakdown by step:
1. { 34 }
2. { 34, 35 }
3. { 69 }
4. { }

#### Best practices in Corth indicate that the stack should be empty by the end of the program.

### Conditional Branching
Let's look at a slightly more complicated example program: \
<code>500 80 - 420 = if</code> \
<code>&nbsp;&nbsp;69 #</code> \
<code>else</code> \
<code>&nbsp;&nbsp;420 #</code> \
<code>endif</code>

This program should first evaulate `500 - 80`, then compare if that sum is equal to `420`. If true, print <samp>69</samp>. If false, print <samp>420</samp>. \
Expected output: \
<samp>69</samp>

Let's break down how it works piece-by-piece:
| Step | Code | Description                                                                                                                     |
|------|------|---------------------------------------------------------------------------------------------------------------------------------|
|    1 |`500 80 -` | Push two values on to the stack, subtract them, then push the result on to the stack.                                      |
|    4 |`420` | Push `420` onto the stack                                                                                                       |
|    5 |  `=` | Use the equality comparison operator; it pops two values off the stack, compares them, then pushes back a `1` if they are equal or a `0` if they aren't.|
|    6 | `if` | Pop the condition just pushed onto the stack, jump to `else`/`endif` if it is false, otherwise, like in this case, fall-through to next instruction.|
|    7 |`69 #`| Push a value onto the stack, then dump it to console output.                                                                    |
|    8 |`else`| Label to jump to if `if` condition is false. Label to jump over to `endif` if `if` condition is true.                           |
|    9 |`420 #`| This would push a value onto the stack, then dump it to console output, however it will be jumped over due to the `if` condition evaluating to true.|
|   10 |`endif`| Label to jump to if `if` condition is false and no `else` label is present or if `if` condition is true and an `else` label is present.|

### Loops
Corth now fully supports loops! Check out the following simple example: \
<code>1 while dup 30 <= do</code> \
<code>&nbsp;&nbsp;dup #</code> \
<code>&nbsp;&nbsp;1 +</code> \
<code>endwhile</code>

This program will print every whole number from `1` to `30`, each being on a new-line.

Let's break down how it works:
| Step | Code | Description                                                                                                                     |
|------|------|---------------------------------------------------------------------------------------------------------------------------------|
|    1 |  `1` | Simply push a one onto the stack.                                                                                               |
|    2 |`while`| Generate an address to jump to upon reaching endwhile.                                                                         |
|    3 |`dup 30 <=`| Push a boolean condition on the stack comparing whether the last item on the stack (duplicated) is less than or equal to `30`.|
|    4 | `do` | Pop the condition just pushed onto the stack, jump just past `endwhile` (step 7) if it is zero, otherwise, like in this case, fall-through to next instruction.|
|    5 |`dup #`| Duplicate the top-most value onto the stack, then dump the duplicate to console output. This prints the current loop counter, as that is what's on the stack.|
|    6 |`1 +`| Add 1 to top-most value on the stack. This increments the loop counter.                                                          |
|    7 |`endwhile`| Upon reaching, jump back to `while` (step 2) and continue execution from there.                                             |

It is known that this program will trigger a stack validator warning, telling us that the stack at the end of the program is not empty. \
With programs as simple as these, it's okay to do, however best practices indicate that the stack should be empty by the end of the program.

### Hello, World!
String literals are now supported in Corth; this makes printing a string to the console as simple as dumping with the `_s` suffix indicating a string format should be used.

<code>"Hello, World!" dump_s</code>

You can also dump single characters like so (most often used for a newline, as shown): \
<code>10 dump_c</code>

Expected output: \
<samp>Hello, World!</samp>

### Complications
For a more complex example, see [rule 110](./examples/rule110.corth)

### Definitions:
#### Operators
An operator will take value(s) from the stack and optionally push some back on.
The amount of values removed/added from/to the stack by a given operator can be seen by the 'pop' and 'push' amount in the following table.

| Operator | Meaning              | Pop  | Push | Description                                                                                                   |
|:--------:|:---------------------|-----:|-----:|:--------------------------------------------------------------------------------------------------------------|
|    `#`   | Dump                 |    1 |    0 | Humankind's best friend; pops a value off the stack, then prints that to the console. For alternate formats, see [dump keywords](#dump-keywords)|
|    `+`   | Addition             |    2 |    1 | Pops two values off the stack, then pushes the sum.                                                           |
|    `-`   | Subtraction          |    2 |    1 | Pops two values off the stack, then pushes the difference.                                                    |
|    `*`   | Multiplication       |    2 |    1 | Pops two values off the stack, then pushes the product.                                                       |
|    `/`   | Division             |    2 |    1 | Pops two values off the stack, then pushes the quotient.                                                      |
|    `=`   | Equality Comparison  |    2 |    1 | Pops two values off the stack, pushes `1` if they are equal to each other, `0` if not.                        |
|    `>`   | Greater-than         |    2 |    1 | Pops two values off the stack, pushes `1` if the former is greater than the latter, `0` if not.               |
|    `<`   | Less-than            |    2 |    1 | Pops two values off the stack, pushes `1` if the former is less than the latter, `0` if not.                  |
|   `>=`   | Greater-than-or-equal|    2 |    1 | Pops two values off the stack; pushes `1` if the former is greater than or equal to the latter, `0` if not.   |
|   `<=`   | Less-than-or-equal   |    2 |    1 | Pops two values off the stack, pushes `1` if the former is less thatn or equal to the latter, `0` if not.     |
|   `<<`   | Bitwise Shift Left   |    2 |    1 | `{<value-to-shift>, <amount-of-bits-to-shift>} -> {<bit-shifted-value>}`                                      |
|   `>>`   | Bitwise Shift Right  |    2 |    1 | `{<value-to-shift>, <amount-of-bits-to-shift>} -> {<bit-shifted-value>}`                                      |
|   `\|\|` | Bitwise Operator     |    2 |    1 | `{a, b} -> {a \|\| b}` Pops two values off the stack, then pushes back the bitwise-or of those values.        |
|   `&&`   | Bitwise Operator     |    2 |    1 | `{a, b} -> {a && b}` Pops two values off the stack, then pushes back the bitwise-and of those values.         |

#### Keywords
| Keyword  | Meaning | Pop  | Push | Description                                                                                                                |
|:--------:|:--------|-----:|-----:|:---------------------------------------------------------------------------------------------------------------------------|
|`if`      |Conditional Branch   |1|0| `{<condition>} -> { }` Pops a value off the stack, then jumps to `else`/`endif` only if popped value is equal to zero.   |
|`else`    |Conditional Branch   |0|0| Only used between `if` and `endif` keywords to provide an alternate branch; what will be ran if `if` condition is false. |
|`endif`   |Block Ending Symbol  |0|0| Required block-ending-symbol for `if` keyword.                                                                           |
|`dup`     |Duplicate            |1|2| `{a} -> {a, a}` Pops one value off the stack, then pushes it back twice.                                                 |
|`twodup`  |Duplicate Two        |2|4| `{a, b} -> {a, b, a, b}` Pops two values off the stack, then pushes them both back on, twice.                            |
|`mem`     |Memory Access        |0|1| `{ } -> {<memory address>}` Pushes the address of the usable memory in Corth. Hard-coded to 720kb; there will be a CCLI option in the future. To access any address within the memory, simply add the byte offset to the address, like so `mem <byte offset> +`, then use it with the memory access keywords that accept memory addresses as arguments.|
|`storeb`  |Store Byte           |2|0| `{<memory address>, <value>} -> { }` Pops a value and a memory address off the stack, then stores the first byte of that value at that memory address.|
|`loadb`   |Load Byte            |1|1| `{<memory address>} -> {<value>}` Pops a memory address off the stack, then pushes a one byte value read from the memory address on to the stack.|
|`do`      |Conditional Jump     |1|0| `{<condition>} -> { }` Pops one value off the stack, then jumps just past `endwhile` if value is zero.                   |
|`while`   |Block Starting Symbol|0|0| Generates a label for `endwhile` to jump to.                                                                             |
|`endwhile`|Block Ending Symbol  |0|0| Generates a label for `do` to jump to upon false condition.                                                              |
|`drop`    |Stack Manipulation   |1|0|    `{a} -> { }` Drops the top-most item off the stack, leaving no reference to it. Use this to delete unused stack-items.|
|`swap`    |Stack Manipulation   |2|2| `{a, b} -> {b, a}` Pops two values off the stack, then pushes them back in reverse order.                                |
|`over`    |Stack Manipulation   |2|3| `{a, b} -> {a, b, a}` Pushes the stack item below the top on to the top.                                                 |
|`shl`     |Bitwise Operator     |2|1| `{<value-to-shift>, <amount-of-bits-to-shift>} -> {<bit-shifted-value>}` Pops two values off the stack, then pushes back the second value bit-shifted left by the first value.|
|`shr`     |Bitwise Operator     |2|1| `{<value-to-shift>, <amount-of-bits-to-shift>} -> {<bit-shifted-value>}` Pops two values off the stack, then pushes back the second value bit-shifted right by the first value.|
|`or`      |Bitwise Operator     |2|1| `{a, b} -> {a \|\| b}` Pops two values off the stack, then pushes back the bitwise-or of those values.                   |
|`and`     |Bitwise Operator     |2|1| `{a, b} -> {a && b}` Pops two values off the stack, then pushes back the bitwise-and of those values.                    |
|<a name="dump-keywords"></a>`dump`|Pop & Print|1|0| `{a} -> { }` Pops a value off the stack, then prints it formatted as an unsigned integer.                  |
|`dump_c`  |Dump Character       |1|0| `{a} -> { }` Pops a value off the stack, then prints it formatted as a char.                                             |
|`dump_s`  |Dump String          |1|0| `{a} -> { }` Pops a value off the stack, then prints it formatted as a string.                                           |

---

## <a name="how-to-use-anchor"></a><a name="cross-platform-anchor"></a> How to build a Corth program
So, you've written a program, what do you do now that you want to run it?

If you do not already have the Corth executable, you can either download it from the [releases page](https://github.com/LensPlaysGames/Corth/releases) or build it yourself using CMake after cloning the repository (further instructions [down below](#build-corth-how-to-anchor)).

There are two assembly syntaxes Corth supports (for now):

##### Warning! -- When using `-GAS` flag or on Linux, if you specify an output name to Corth with `-o`, it will only affect the generated assembly file name, not the output object or executable file. Look for `a.out` or `a.exe`, etc. This may over-write previously-compiled-programs, so be careful! To accurately rename the output executable, pass the corresponding option to your linker with `-add-lo <option>`, for example: `-add-lo "-o my_program"` along with the normal `-o my_program` to rename the generated assembly file.

### GAS, or the [GNU assembler](https://en.wikipedia.org/wiki/GNU_Assembler).

#### Linux
The whole lot of GNU tools will come with a majority of Linux systems, and if not, will be easily obtainable.

To familiarize yourself with the Corth Command Line Interface (CCLI), run the following command: \
`./Corth -h` or `./Corth --help` \
This will list all of the possible flags and options that may be passed to Corth.

Example Corth source code to executable compilation command on Linux: \
`./Corth -GAS -linux test.corth`

Example cmd with output renamed: \
`./Corth -GAS -linux -add-ao "-o my-output-name" -o my-output-name test.corth`

#### Windows
As for Windows, there is a little funky business... MinGW, the 'normal' installation manager for GNU tools on Windows, doesn't support 64 bits. \
Luckily, there is a community-fix, [TDM-GCC-64](https://jmeubank.github.io/tdm-gcc/), that solves this exact problem, so go donate to this person for doing the hard work that all of us can now use. \
It's a very easy to use installer, and comes with a whole host of very useful 64 bit tools on Windows. Install it at the default location, otherwise Corth will need to be passed the path to the `gcc` executable using the CCLI `-a` option.

To familiarize yourself with the Corth Command Line Interface (CCLI), run the following command: \
`Corth.exe -h` or `Corth.exe --help` \
This will list all of the possible flags and options that may be passed to Corth.

Example command to compile `test.corth` to an executable on Windows: \
`Corth.exe -GAS test.corth`

Example command with output renamed: \
`Corth.exe -GAS -add-ao "-o my-output-name" -o my-output-name test.corth`

### NASM
On Windows you can [download the installer from the official website](https://www.nasm.us/) \
On Mac, you can [download the necessary binaries from the official website](https://www.nasm.us/) \
On Linux, run the following CMD: `apt install nasm` \
If you are on Windows and you didn't install NASM in the default directory the installer prompted, keep in mind the path to the 'nasm' executable file itself. You will need it later for the `-a` or `--assembler-path` command line option.

#### You must ensure that you have some sort of linker on your machine that can link against the standard C runtime of whatever platform you're on.

- [GoLink](http://godevtool.com/) is my recommendation on Windows. \
GoLink is easy to use and fast to setup; simply extract it and it's ready.

- On Linux, this is most likely `ld`, the GNU linker; it comes with most linux distros by default.

Once all the pre-requisites are installed, now comes time to use the CCLI, or Corth Command Line Interface. \
To avoid headache as much as possible, Corth sets default values based on your operating system. \
If you get any errors, there are a multitude of command line options to help rectify the situation. (see [Common Errors](#common-errors-anchor))

### Linux
Open a terminal and navigate to the directory containing the Corth executable. \
To familiarize yourself with the options of the CCLI, run the following command: \
`./Corth -h` or `./Corth --help` \
A lot of options will appear, but most will not be needed unless you are getting errors, as the defaults are platform-specific. \
One required flag on Linux is the `-linux` flag.

Basic Example (given `apt install nasm` was run and `ld` is installed by default): \
`./Corth -o my_program -add-lo "-o my_program" test.corth`

Verbose Example: \
`./Corth -a nasm -ao "-f elf64 corth_program.asm" -l ld -lo " -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -m elf_x86_64 -o corth_program corth_program.o" test.corth`

###  Windows
Open a terminal and navigate to the directory containing `Corth.exe`. \
To familiarize yourself with the options of the CCLI, run the following command: \
`Corth.exe -h` or `Corth.exe --help` \
A lot of different options will come up, with clear explanations on what everything does.

Basic example: \
`Corth.exe -o my_program test.corth`

Or, if Corth is giving errors about not finding assembler/linker: \
`Corth.exe -a /Path/To/NASM/nasm.exe -l /Path/To/GoLink/golink.exe test.corth`

Alternatively, you could add the directory containing the executable to your system's [PATH environment variable](https://www.c-sharpcorner.com/article/add-a-directory-to-path-environment-variable-in-windows-10/): \
`Corth.exe -a nasm.exe -l golink.exe test.corth`

By default, the assembler and linker options are setup for Windows, using NASM and GoLink. \
If your situation is different, make sure to specify the correct options using `-ao` and `-lo` respectively.

### <a name="common-errors-anchor"></a>Common Errors
- "Assembler not found at x"
  - Solution: Specify a valid path, including file name and extension, to the assembler executable using `-a` or `--assembler-path`
- "Linker not found at x"
  - Solution: Specify a valid path, including file name and extension, to the linker executable using `-l Path/To/Linker.exe` or `--linker-path Path/To/Linker.exe`
- The stdout and stderr of any commands run are redirected to a log file in the same directory as the Corth executable. The contents of these files are printed to the console when the verbose flag is passed to Corth through the CCLI with `-v` or `--verbose`.

---

## <a name="build-corth-how-to-anchor"></a>How to build Corth from source
This project uses [CMake](https://cmake.org/) to build Corth for any platform that CMake supports (which is a lot). \
This means Corth source code can be easily built in your favorite IDE that supports C++ (or with make, you linux-folk).

First, on any platform, clone this repository to your local machine.

Run the following command in the repository directory: \
`cmake -S . -B build/`

This will use CMake to build a build system with the default generator on your platform. \
Once complete, open the build directory, and build Corth using the build system you just generated.

Windows example: \
Open Visual Studio solution and build with `F6`

Linux example: \
Open terminal, run `make`
