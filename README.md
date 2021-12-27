# Corth
It's like [Porth](https://gitlab.com/tsoding/porth/-/tree/master/), which is like [Forth](https://www.forth.com/forth/) but written in Python, but written in C++. But I don't actually know for sure since I never programmed in Porth or Forth, I only heard that they are some sort of stack-based programming language. Corth is also stack-based programming language. Which makes it just like Porth, just like Forth, am I rite?

Also, it compiles directly to executable on multiple platforms (not MacOS (yet)).

---

## How to write in Corth
Corth, like Porth (like Forth), is a stack based language. \
This means that in order to do any operation, a value must be pushed onto the stack. \
Think of this as a literal stack of objects. \
Likely, the operation will take it's arguments from the top of the stack, or `pop` from the stack, and sometimes it will return a value or two back onto it.

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

Let's look at a slightly more complicated example program: \
<code>500 80 - 420 = <span style="color:purple">if</span></code> \
<code>&nbsp;&nbsp;69 #</code> \
<code style="color:purple">else</code> \
<code>&nbsp;&nbsp;420 #</code> \
<code style="color:purple">endif</code>

This program should first evaulate `500 - 80`, then compare if that sum is equal to `420`. If true, print <samp>69</samp>. If false, print <samp>420</samp>. \
Expected output: \
<samp>69</samp>

Let's break down how it works piece-by-piece:
| Step | Code | Description                                                                                                                     |
|------|------|---------------------------------------------------------------------------------------------------------------------------------|
|    1 |`500` | A value is pushed on to the stack, meaning stacked on top.                                                                      |
|    2 | `80` | Another value is pushed on to the stack.                                                                                        |
|    3 |  `-` | The `-` symbol will pop the two most-recent values off the stack, subtract them, then push the difference on to the stack.      |
|    4 |`420` | Push `420` onto the stack                                                                                                       |
|    5 |  `=` | Use the equality comparison operator; it pops two values off the stack, compares them, then pushes back a `1` if they are equal or a `0` if they aren't.|
|    6 |<code style="color:purple">if</code>| Pop the condition just pushed onto the stack, jump to <code style="color:purple">else</code>/<code style="color:purple">endif</code> if it is false, otherwise fall-through.|
|    7 |`69 #`| Push a value onto the stack, then dump it to console output.                                                                    |
|    8 |<code style="color:purple">else</code>| Label to jump to if <code style="color:purple">if</code> condition is false.                    |
|    9 |`420 #`| Push a value onto the stack, then dump it to console output.                                                                   |
|   10 |<code style="color:purple">endif</code>| Label to jump to if <code style="color:purple">if</code> condition is false and no <code style="color:purple">else</code> label is present.|

### Definitions:
#### Operators
An operator will take value(s) from the stack and optionally push some back on.
The amount of values removed/added from/to the stack by a given operator can be seen by the 'pop' and 'push' amount in the following table.

| Operator | Meaning              |  Pop | Push | Description                                                                                                 |
|:--------:|:---------------------|-----:|-----:|:------------------------------------------------------------------------------------------------------------|
|    `#`   | Dump                 |    1 |    0 | Humankind's best friend; pops a value off the stack, then prints that to the console.                       |
|    `+`   | Addition             |    2 |    1 | Pops two values off the stack, then pushes the sum.                                                         |
|    `-`   | Subtraction          |    2 |    1 | Pops two values off the stack, then pushes the difference.                                                  |
|    `*`   | Multiplication       |    2 |    1 | Pops two values off the stack, then pushes the product.                                                     |
|    `/`   | Division             |    2 |    1 | Pops two values off the stack, then pushes the quotient.                                                    |
|    `=`   | Equality Comparison  |    2 |    1 | Pops two values off the stack, pushes `1` if they are equal to each other, `0` if not.                      |
|    `>`   | Greater-than         |    2 |    1 | Pops two values off the stack, pushes `1` if the former is greater than the latter, `0` if not.             |
|    `<`   | Less-than            |    2 |    1 | Pops two values off the stack, pushes `1` if the former is less than the latter, `0` if not.                |
|   `>=`   | Greater-than-or-equal|    2 |    1 | Pops two values off the stack; pushes `1` if the former is greater than or equal to the latter, `0` if not. |
|   `<=`   | Less-than-or-equal   |    2 |    1 | Pops two values off the stack, pushes `1` if the former is less thatn or equal to the latter, `0` if not.   |

#### Keywords
| Keyword | Meaning |  Pop | Push | Description |
|:-------:|:--------|-----:|-----:|:------------|
|<code style="color:purple">if</code>|Conditional Branch|1|0|Pops a value off the stack, then jumps to <code style="color:purple">else</code>/<code style="color:purple">endif</code>, unless value is true.|
|<code style="color:purple">else</code>|Conditional Branch|0|0|Only used between <code style="color:purple">if</code> and <code style="color:purple">endif</code> keywords to provide an alternate branch; what will be ran if <code style="color:purple">if</code> condition is false.|
|<code style="color:purple">endif</code>|Block Ending Symbol|0|0|Required block-ending-symbol for <code style="color:purple">if</code> keyword.|

---

## How to build a Corth program
So, you've written a program, what do you do now that you want to run it?

The easiest thing to do is to simulate the program. \
To do this, simply use the `-sim` or `--simulate` command line argument i.e. \
Windows: `Path/To/Corth.exe -sim test.corth` \
Linux: `./Path/To/Corth -sim test.corth`

If you do not already have the Corth executable, you can either download it from the [releases page](https://github.com/LensPlaysGames/Corth/releases) or build it yourself using CMake after cloning the repository (further instructions down below).

If you would like to compile a corth program into an executable file, there are a few more steps.

First, you must ensure that you have [NASM](https://www.nasm.us/) installed somewhere on your machine. \
On Windows you can [download the installer from the official website](https://www.nasm.us/) \
On Mac, you can [download the necessary binaries from the official website][https://www.nasm.us/) \
On Linux, run the following CMD: `apt install nasm` \
NASM is the assembler that this project is built for. Eventually, different types of assembly will be able to be generated. \
If you are on Windows and you didn't install it in the default directory the installer prompted, keep in mind the path to the 'nasm' executable file itself. You will need it later for the `-a` or `--assembler-path` command line option. \
The only assembly syntax supported right now is NASM, but that will change in the future (i.e. [GAS](https://en.wikipedia.org/wiki/GNU_Assembler)).

Second, you must ensure that you have some sort of linker on your machine that can link against the standard C runtime of whatever platform you're on. \
For example, this might be [GoLink](http://godevtool.com/) on Windows. GoLink is easy to use and fast to setup; simply extract it and it's ready. \
On Linux, this is most likely `ld`, the GNU linker; it comes with most linux distros by default.

Once all the pre-requisites are installed, now comes time to use the CCLI, or Corth Command Line Interface. \
Ideally, it will be very similar to the simulate command up above, but sometimes quite a few arguments need to be specified. \
To avoid this as much as possible, Corth sets default values based on your operating system.

##### Warning! -- On Linux, if you specify an output name to Corth with `-o`, it will only affect the generated assembly file name, not the output object or executable file. Look for `a.out` or similar. This may over-write previously-compiled-programs, so be careful! To accurately rename the output executable, pass the corresponding option to your linker with `-add-lo <option>`. \
For example: `-add-lo "-o my_program"`

### On Windows
Open a terminal and navigate to the directory containing Corth.exe. \
To familiarize yourself with the options of the CCLI, run the following command: \
`Corth.exe -h` or `Corth.exe --help` \
A lot of different options will come up, with (hopefully) clear explanations on what everything does. \
Because there are a lot, I will tell you the ones you will most likely need right away:
- `-com` or `--compile`
  - Specifies that Corth needs to generate assembly, assemble it into machine code, then link it into an executable.
- `-a` or `--assembler-path`
  - This does what it sounds like, and allows the user to specify the path to the assembler `.exe` file to run. Set this to the exact path of `nasm.exe` on your machine, including file name and extension. If you downloaded it at the default location, Corth's defaults should work as well, so ideally you won't need to specify this argument.
- `-l` or `--linker-path`
  - Specify the path to the linker `.exe` file to run. This will be the path to `golink.exe` including file name and extension. Again, if you downloaded at the default locations, everything should work fine without specifying any extra arguments.
- `-o` or `--output-name`
  - Specify the name of the generated assembly, object, and executable files.
- `-gen` or `--generate`
  - Tell Corth to not use any command line tools to assemble or link anything; simply generate the output assembly file for the given platform and program. Useful if you would like to use a completely different assembling and linking ecosystem.

Basic example: \
`Corth.exe -com -o my_program test.corth`

Or, if Corth is giving errors about not finding assembler/linker: \
`Corth.exe -com -a /Path/To/NASM/nasm.exe -l /Path/To/GoLink/golink.exe test.corth`

Alternatively, you could add the directory containing the executable to your system's [PATH environment variable](https://www.c-sharpcorner.com/article/add-a-directory-to-path-environment-variable-in-windows-10/): \
`Corth.exe -com -a nasm.exe -l golink.exe test.corth`

By default, the assembler and linker options are setup for Windows, using NASM and GoLink. \
If your situation is different, make sure to specify the correct options using `-ao` and `-lo` respectively.

### On Linux
Open a terminal and navigate to the directory containing the Corth executable. \
To familiarize yourself with the options of the CCLI, run the following command: \
`./Corth -h` or `./Corth --help` \
A lot of options will appear, but most will not be needed unless you are getting errors, as the defaults are platform-specific.
- `-linux` or `-linux64`
  - Tell Corth to generate Linux assembly for NASM.
- `-com` or `--compile`
  - Specifies compilation mode to Corth. This tells corth to generate assembly, assemble it, and link it into an executable.
- `-gen` or `--generate`
  - Tell Corth to not use any command line tools to assemble or link anything; simply generate the output assembly file for the given platform and program. Useful if you would like to use a completely different assembling and linking ecosystem.
- `-o` or `--output-name`
  - Specify the name of the generated assembly files.
- `-add-ao` or `-add-asm-opt`
  - Append a command line argument to assembler options
- `-add-lo` or `-add-link-opt`
  - Append a command line argument to linker options

Basic Example (given `apt install nasm` was run and `ld` is installed by default): \
`./Corth -com -o my_program -add-lo "-o my_program" test.corth`

Verbose Example: \
`./Corth -com -a nasm -ao "-f elf64 corth_program.asm" -l ld -lo " -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -m elf_x86_64 -o corth_program corth_program.o" test.corth`

### Common Errors:
- "Assembler not found at x"
  - Solution: Specify a valid path, including file name and extension, to the assembler executable using `-a` or `--assembler-path`
- "Linker not found at x"
  - Solution: Specify a valid path, including file name and extension, to the linker executable using `-l Path/To/Linker.exe` or `--linker-path Path/To/Linker.exe`

---

## How to build Corth from source
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