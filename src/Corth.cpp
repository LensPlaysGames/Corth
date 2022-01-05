#include <iostream>    // printf
#include <stdio.h>     // fopen_s
#include <fstream>     // ofstream, ifstream
#include <algorithm>   // remove_if
#include <stdlib.h>    // system, exit

// Custom headers
#include "Errors.h"

// Data types
#include <string>
#include <vector>
#include <map>

// Platform specific includes
#ifdef _WIN64
#include <sstream>
#else
#endif

#ifdef __linux__
#include <unistd.h>
#else
#endif

// TODO:
// Make MEM_CAPACITY accessible through a CCLI option
// Implement store/load64 (32, 16?)
// New keyword: `mod` with matching operator symbol '%' (modulus)

namespace Corth {
    const unsigned int MEM_CAPACITY = 720000;
    std::string SOURCE_PATH = "main.corth";
    std::string OUTPUT_NAME = "corth_program";
    std::string ASMB_PATH = "";
    std::string LINK_PATH = "";
    std::string ASMB_OPTS = "";
    std::string LINK_OPTS = "";
    std::string ASMB_NAME = "";
    std::string LINK_NAME = "";

    enum class MODE {
        COMPILE,
        GENERATE,
        COUNT
    };
    MODE RUN_MODE = MODE::COMPILE;

    enum class PLATFORM {
        WIN64,
        LINUX64,
        COUNT
    };
    PLATFORM RUN_PLATFORM = PLATFORM::WIN64;

    enum class ASM_SYNTAX {
        NASM,
        GAS,
        COUNT
    };
    ASM_SYNTAX ASSEMBLY_SYNTAX = ASM_SYNTAX::NASM;
    
    bool verbose_logging = false;

    // This needs to be changed if operators are added or removed from Corth internally.
    const size_t OP_COUNT = 15;
    bool isoperator(char& c){
		return c == '+'      // addition
			|| c == '-'    // subtraction
			|| c == '*'    // multiplication
			|| c == '/'    // division
			|| c == '='    // equality comparison
			|| c == '<'    // less than (or equal) comparison
			|| c == '>'    // greater than (or equal) comparison
			|| c == '#'    // dump (pop + print)
			|| c == '%'    // modulo
			|| c == '&'    // bitwise and (when combined with another)
			|| c == '|';   // bitwise or (when combined with another)
  }

    enum class Keyword {
        IF,
        ELSE,
        ENDIF,
        DUP,
        TWODUP,
        MEM,
        LOADB,
        STOREB,
        DO,
        WHILE,
        ENDWHILE,
        DUMP,
        DUMP_C,
        DUMP_S,
        DROP,
        SWAP,
        OVER,
        SHL,
        SHR,
        OR,
        AND,
		MOD,
        COUNT
    };

	// TODO: Convert to lookup table
    bool iskeyword(std::string word) {
        static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in iskeyword");
        if (word == "if"
            || word == "else"
            || word == "endif"
            || word == "dup"
            || word == "twodup"
            || word == "mem"
            || word == "loadb"
            || word == "storeb"
            || word == "do"
            || word == "while"
            || word == "endwhile"
            || word == "dump"
            || word == "dump_c"
            || word == "dump_s"
            || word == "drop"
            || word == "swap"
            || word == "over"
            || word == "shl"
            || word == "shr"
            || word == "or"
            || word == "and"
			|| word == "mod")
        {
            return true;
        }
        else {
            return false;
        }
    }

	// TODO: Convert to lookup table
    // This function outlines the corth source input and the output it will generate.
    // case <output>: { return "<input>"; }
    std::string GetKeywordStr(Keyword word) {
        static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in GetKeywordStr");
        switch (word) {
        case Keyword::IF:       { return "if";       }
        case Keyword::ELSE:     { return "else";     }
        case Keyword::ENDIF:    { return "endif";    }
        case Keyword::DUP:      { return "dup";      }
        case Keyword::TWODUP:   { return "twodup";   }
        case Keyword::MEM:      { return "mem";      }
        case Keyword::LOADB:    { return "loadb";    }
        case Keyword::STOREB:   { return "storeb";   }
        case Keyword::DO:       { return "do";       }
        case Keyword::WHILE:    { return "while";    }
        case Keyword::ENDWHILE: { return "endwhile"; }
        case Keyword::DUMP:     { return "dump";     }
        case Keyword::DUMP_C:   { return "dump_c";   }
        case Keyword::DUMP_S:   { return "dump_s";   }
        case Keyword::DROP:     { return "drop";     }
        case Keyword::SWAP:     { return "swap";     }
        case Keyword::OVER:     { return "over";     }
        case Keyword::SHL:      { return "shl";      }
        case Keyword::SHR:      { return "shr";      }  
        case Keyword::OR:       { return "or";       }
        case Keyword::AND:      { return "and";      }
		case Keyword::MOD:      { return "mod";      }
        default:
            Error("UNREACHABLE in GetKeywordStr");
            exit(1);
            return "ERROR";
        }
    }

    enum class TokenType {
        WHITESPACE,
        INT,
        STRING,
        OP,
        KEYWORD,
        COUNT
    };

    std::string TokenTypeStr(TokenType& t) {
        static_assert(static_cast<int>(TokenType::COUNT) == 5, "Exhaustive handling of token types in TokenTypeStr");
        if (t == TokenType::WHITESPACE)   { return "WHITESPACE"; }
        else if (t == TokenType::INT)     { return "INTEGER";    }
        else if (t == TokenType::STRING)  { return "STRING";     }
        else if (t == TokenType::OP)      { return "OPERATOR";   }
        else if (t == TokenType::KEYWORD) { return "KEYWORD";    }
        Error("UNREACHABLE in TokenTypeStr");
        exit(1);
        return "ERROR";
    }
    
    struct Token {
    public:
        TokenType type;
        std::string text;
        std::string data;
        size_t line_number;
        size_t col_number;

        Token(){
            type = TokenType::WHITESPACE;
            text = "";
            line_number = 1;
            col_number = 1;
        }
    };
    
    struct Program {
        std::string source;
        std::vector<Token> tokens;
    };

    void PrintUsage() {
        printf("\n%s\n", "Usage: `Corth.exe <flags> <options> Path/To/File.corth`");
        printf("    %s\n", "Flags:");
        printf("        %s\n", "-win, -win64             | (default) Generate assembly for Windows 64-bit. If no platform is specified, this is the default.");
        printf("        %s\n", "-linux, -linux64         | Generate assembly for Linux 64-bit.");
        //printf("        %s\n", "-mac, -apple             | Generate assembly for MacOS 64-bit.");
        printf("        %s\n", "-com, --compile          | (default) Compile program from source into executable");
        printf("        %s\n", "-gen, --generate         | Generate assembly, but don't create an executable from it.");
        printf("        %s\n", "-NASM                    | (default) When generating assembly, use NASM syntax. Any OPTIONS set before NASM may or may be over-ridden; best practice is to put it first.");
        printf("        %s\n", "-GAS                     | When generating assembly, use GAS syntax. This is able to be assembled by gcc into an executable. (pass output file name to gcc with `-add-ao \"-o <output-file-name>\" and not the built-in `-o` option`). Any OPTIONS set before GAS may or may be over-ridden; best practice is to put it first.");
        printf("        %s\n", "-v, --verbose            | Enable verbose logging within Corth");
        printf("    %s\n", "Options (latest over-rides):");
        printf("        %s\n", "Usage: <option> <input>");
        printf("        %s\n", "If the <input> contains spaces, be sure to surround it by double quotes");
        printf("        %s\n", "-o, --output-name        | Specify name of generated files. On Linux, affects only generated assembly file; use -add-ao/-add-lo to specify output object and executable file name manually");
        printf("        %s\n", "-a, --assembler-path     | Specify path to assembler (include extension)");
        printf("        %s\n", "-l, --linker-path        | Specify path to linker (include extension)");
        printf("        %s\n", "-ao, --assembler-options | Command line arguments called with assembler");
        printf("        %s\n", "-lo, --linker-options    | Command line arguments called with linker");
        printf("        %s\n", "-add-ao, --add-asm-opt   | Append a command line argument to assembler options");
        printf("        %s\n", "-add-lo, --add-link-opt  | Append a command line argument to linker options");
    }

    std::vector<std::string> string_to_hex(const std::string& input)
    {
        static const char hex_digits[] = "0123456789abcdef";

        std::vector<std::string> output;
        for (unsigned char c : input)
        {
            std::string hex;
            hex.append(1, '0');
            hex.append(1, 'x');
            hex.push_back(hex_digits[c >> 4]);
            hex.push_back(hex_digits[c & 15]);
            output.push_back(hex);
        }
        return output;
    }
    
    void GenerateAssembly_NASM_linux64(Program& prog) {
        std::string asm_file_path = OUTPUT_NAME + ".asm";
        std::fstream asm_file;
        asm_file.open(asm_file_path.c_str(), std::ios::out);
        if (asm_file) {
            Log("Generating NASM elf64 assembly");

            // Save list of defined strings in file to write at the end of the assembly in the `.data` section.
            std::vector<std::string> string_literals;

            // WRITE HEADER TO ASM FILE
            asm_file << "    ;; CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
                     << "    ;; USING `SYSTEM V AMD64 ABI` CALLING CONVENTION (RDI, RSI, RDX, RCX, R8, R9, -> STACK)\n"
                     << "    ;; LINUX SYSTEM CALLS USE R10 INSTEAD OF RCX\n"
                     << "    SECTION .text\n"
                     << "    ;; DEFINE EXTERNAL C RUNTIME SYMBOLS\n"
                     << "    extern exit\n"
                     << "    extern printf\n"
                     << "\n"
                     << "    global _start\n"
                     << "_start:\n";

            // WRITE TOKENS TO ASM FILE MAIN LABEL
            static_assert(static_cast<int>(TokenType::COUNT) == 5, "Exhaustive handling of token types in GenerateAssembly_NASM_linux64");
            size_t instr_ptr = 0;
            size_t instr_ptr_max = prog.tokens.size();
            while (instr_ptr < instr_ptr_max) {
                Token& tok = prog.tokens[instr_ptr];
                // Write assembly to opened file based on token type and value
                if (tok.type == TokenType::INT) {
                    asm_file << "    ;; -- push INT --\n"
                             << "    mov rax, " << tok.text << "\n"
                             << "    push rax\n";
                }
                else if (tok.type == TokenType::STRING) {
                    asm_file << "    ;; -- push STRING --\n"
                             << "    mov rax, str_" << string_literals.size() << '\n'
                             << "    push rax\n";
                    // String value is stored in tok.text
                    string_literals.push_back(tok.text);
                }
                else if (tok.type == TokenType::OP) {
                    static_assert(OP_COUNT == 15, "Exhaustive handling of operators in GenerateAssembly_NASM_linux64");
                    if (tok.text == "+") {
                        asm_file << "    ;; -- add --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    add rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "-") {
                        asm_file << "    ;; -- subtract --\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    sub rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "*") {
                        asm_file << "    ;; -- multiply --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    mul rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "/") {
                        asm_file << "    ;; -- divide --\n"
                                 << "    xor rdx, rdx\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    div rbx\n"
                                 << "    push rax\n";
                    }
					else if (tok.text == "%") {
                        asm_file << "    ;; -- modulo --\n"
                                 << "    xor rdx, rdx\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    div rbx\n"
                                 << "    push rdx\n";
                    }
                    else if (tok.text == "=") {
                        asm_file << "    ;; -- equality condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmove rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == "<") {
                        asm_file << "    ;; -- less than condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovl rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == ">") {
                        asm_file << "    ;; -- greater than condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovg rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == "<=") {
                        asm_file << "    ;; -- less than or equal condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovle rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == ">=") {
                        asm_file << "    ;; -- greater than or equal condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovge rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == "<<") {
                        asm_file << "    ;; -- bitwise-shift left --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shl rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == ">>") {
                        asm_file << "    ;; -- bitwise-shift right --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shr rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == "||") {
                        asm_file << "    ;; -- bitwise or --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    or rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "&&") {
                        asm_file << "    ;; -- bitwise and --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    and rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "#") {
                        // Without clearing rax, seg faults can happen seemingly at random
                        asm_file << "    ;; -- dump --\n"
                                 << "    lea rdi, [rel fmt]\n"
                                 << "    pop rsi\n"
                                 << "    xor rax, rax\n"
                                 << "    call printf\n";
                    }
                }
                else if (tok.type == TokenType::KEYWORD) {
                    static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in GenerateAssembly_NASM_linux64");
                    if (tok.text == GetKeywordStr(Keyword::IF)) {
                        asm_file << "    ;; -- if --\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, 0\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ELSE)) {
                        asm_file << "    ;; -- else --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDIF)) {
                        asm_file << "    ;; -- endif --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUP)) {
                        asm_file << "    ;; -- dup --\n"
                                 << "    pop rax\n"
                                 << "    push rax\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::TWODUP)) {
                        asm_file << "    ;; -- twodup --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    push rbx\n"
                                 << "    push rax\n"
                                 << "    push rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::WHILE)) {
                        asm_file << "    ;; -- while --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DO)) {
                        asm_file << "    ;; -- do --\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, 0\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDWHILE)) {
                        asm_file << "    ;; -- endwhile --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::MEM)) {
                        asm_file << "    ;; -- mem --\n"
                                 << "    push mem\n";
                        // Pushes the relative address of allocated memory onto the stack
                    }
                    else if (tok.text == GetKeywordStr(Keyword::LOADB)) {
                        asm_file << "    ;; -- load byte --\n"
                                 << "    pop rax\n"
                                 << "    xor rbx, rbx\n"
                                 << "    mov bl, [rax]\n"
                                 << "    push rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::STOREB)) {
                        asm_file << "    ;; -- store byte --\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    mov [rax], bl\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP)) {
                        asm_file << "    ;; -- dump --\n"
                                 << "    lea rdi, [rel fmt]\n"
                                 << "    pop rsi\n"
                                 << "    xor rax, rax\n"
                                 << "    call printf\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_C)) {
                        asm_file << "    ;; -- dump character --\n"
                                 << "    lea rdi, [rel fmt_char]\n"
                                 << "    pop rsi\n"
                                 << "    xor rax, rax\n"
                                 << "    call printf\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_S)) {
                        asm_file << "    ;; -- dump string --\n"
                                 << "    lea rdi, [rel fmt_str]\n"
                                 << "    pop rsi\n"
                                 << "    xor rax, rax\n"
                                 << "    call printf\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DROP)) {
                        asm_file << "    ;; -- drop --\n"
                                 << "    pop rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SWAP)) {
                        asm_file << "    ;; -- swap --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    push rax\n"
                                 << "    push rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OVER)) {
                        asm_file << "    ;; -- over --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    push rbx\n"
                                 << "    push rax\n"
                                 << "    push rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHL)) {
                        asm_file << "    ;; -- bitwise-shift left --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shl rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHR)) {
                        asm_file << "    ;; -- bitwise-shift right --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shr rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OR)) {
                        asm_file << "    ;; -- bitwise or --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    or rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::AND)) {
                        asm_file << "    ;; -- bitwise and --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    and rax, rbx\n"
                                 << "    push rax\n";
                    }
					else if (tok.text == GetKeywordStr(Keyword::MOD)) {
                        asm_file << "    ;; -- modulo --\n"
                                 << "    xor rdx, rdx\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    div rbx\n"
                                 << "    push rdx\n";
                    }
                }
                instr_ptr++;
            }
            // WRITE ASM FOOTER (GRACEFUL PROGRAM EXIT, CONSTANTS, MEMORY ALLOCATION)
            asm_file << "    mov rdi, 0\n"
                     << "    call exit\n"
                     << '\n'
                     << "    SECTION .data\n"
                     << "    fmt db '%u', 10, 0\n"
                     << "    fmt_char db '%c', 0\n"
                     << "    fmt_str db '%s', 0\n";

            size_t index = 0;
            for (auto& string : string_literals) {
                std::vector<std::string> hex_chars = string_to_hex(string);
                asm_file << "str_" << index << " db ";
                for (auto& c : hex_chars) {
                    asm_file << c << ',';
                }
                // Null-terminator
                asm_file << "0\n";
                index++;
            }
                
            asm_file << '\n'
                     << "    SECTION .bss\n"
                     << "    mem resb " << MEM_CAPACITY << '\n';
            
            asm_file.close();
                
            Log("NASM elf64 assembly generated at " + asm_file_path);
        }
        else {
            Error("Could not open file for writing. Does directory exist?");
            exit(1);
        }
    }

    void GenerateAssembly_GAS_linux64(Program& prog) {
        std::string asm_file_path = OUTPUT_NAME + ".s";
        std::fstream asm_file;
        asm_file.open(asm_file_path.c_str(), std::ios::out);
        if (asm_file) {
            Log("Generating Linux x64 GAS assembly");

            // Save list of defined strings in file to write at the end of the assembly in the `.data` section.
            std::vector<std::string> string_literals;

            // WRITE HEADER TO ASM FILE
            asm_file << "    # CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
                     << "    # USING `GAS` SYNTAX\n"
                     << "    # USING `SYSTEM V AMD64 ABI` CALLING CONVENTION (RDI, RSI, RDX, RCX, R8, R9, -> STACK)\n"
                     << "    # LINUX SYSTEM CALLS USE R10 INSTEAD OF RCX\n"
                     << "    .text\n"
                     << "    .globl main\n"
                     << "main:\n";

            // WRITE TOKENS TO ASM FILE MAIN LABEL
            static_assert(static_cast<int>(TokenType::COUNT) == 5, "Exhaustive handling of token types in GenerateAssembly_GAS_linux64");
            size_t instr_ptr = 0;
            size_t instr_ptr_max = prog.tokens.size();
            while (instr_ptr < instr_ptr_max) {
                Token& tok = prog.tokens[instr_ptr];
                // Write assembly to opened file based on token type and value
                if (tok.type == TokenType::INT) {
                    asm_file << "    # -- push INT --\n"
                             << "    mov $"  << tok.text << ", %rax" << "\n"
                             << "    push %rax\n";
                }
                else if (tok.type == TokenType::STRING) {
                    asm_file << "    # -- push STRING --\n"
                             << "    lea str_" << string_literals.size() << "(%rip), %rax\n"
                             << "    push %rax\n";
                    // String value is stored in tok.text
                    string_literals.push_back(tok.text);
                }
                else if (tok.type == TokenType::OP) {
                    static_assert(OP_COUNT == 15, "Exhaustive handling of operators in GenerateAssembly_GAS_linux64");
                    if (tok.text == "+") {
                        asm_file << "    # -- add --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    add %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "-") {
                        asm_file << "    # -- subtract --\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    sub %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "*") {
                        asm_file << "    # -- multiply --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    mul %rbx\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "/") {
                        asm_file << "    # -- divide --\n"
                                 << "    xor %rdx, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    div %rbx\n"
                                 << "    push %rax\n";
                    }
					else if (tok.text == "%") {
                        asm_file << "    # -- modulo --\n"
                                 << "    xor %rdx, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    div %rbx\n"
                                 << "    push %rdx\n";
                    }
                    else if (tok.text == "=") {
                        asm_file << "    # -- equality condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmove %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == "<") {
                        asm_file << "    # -- less than condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovl %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == ">") {
                        asm_file << "    # -- greater than condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovg %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == "<=") {
                        asm_file << "    # -- less than or equal condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovle %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == ">=") {
                        asm_file << "    # -- greater than or equal condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovge %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == "<<") {
                        asm_file << "    # -- bitwise-shift left --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shl %cl, %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == ">>") {
                        asm_file << "    # -- bitwise-shift right --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shr %cl, %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == "||") {
                        asm_file << "    # -- bitwise or --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    or %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "&&") {
                        asm_file << "    # -- bitwise and --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    and %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "#") {
                        asm_file << "    # -- dump --\n"
                                 << "    lea fmt(%rip), %rdi\n"
                                 << "    pop %rsi\n"
                                 << "    xor %rax, %rax\n"
                                 << "    call printf\n";
                    }
                }
                else if (tok.type == TokenType::KEYWORD) {
                    static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in GenerateAssembly_GAS_linux64");
                    if (tok.text == GetKeywordStr(Keyword::IF)) {
                        asm_file << "    # -- if --\n"
                                 << "    pop %rax\n"
                                 << "    cmp $0, %rax\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ELSE)) {
                        asm_file << "    # -- else --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDIF)) {
                        asm_file << "    # -- endif --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUP)) {
                        asm_file << "    # -- dup --\n"
                                 << "    pop %rax\n"
                                 << "    push %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::TWODUP)) {
                        asm_file << "    # -- twodup --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    push %rbx\n"
                                 << "    push %rax\n"
                                 << "    push %rbx\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::WHILE)) {
                        asm_file << "    # -- while --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DO)) {
                        asm_file << "    # -- do --\n"
                                 << "    pop %rax\n"
                                 << "    cmp $0, %rax\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDWHILE)) {
                        asm_file << "    # -- endwhile --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::MEM)) {
                        asm_file << "    # -- mem --\n"
                                 << "    lea mem(%rip), %rax\n"
                                 << "    push %rax\n";
                        // Pushes the relative address of allocated memory onto the stack
                    }
                    else if (tok.text == GetKeywordStr(Keyword::LOADB)) {
                        asm_file << "    # -- load byte --\n"
                                 << "    pop %rax\n"
                                 << "    xor %rbx, %rbx\n"
                                 << "    mov (%rax), %bl\n"
                                 << "    push %rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::STOREB)) {
                        asm_file << "    # -- store byte --\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    mov %bl, (%rax)\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP)) {
                        asm_file << "    # -- dump --\n"
                                 << "    lea fmt(%rip), %rdi\n"
                                 << "    pop %rsi\n"
                                 << "    xor %rax, %rax\n"
                                 << "    call printf\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_C)) {
                        asm_file << "    # -- dump --\n"
                                 << "    lea fmt_char(%rip), %rdi\n"
                                 << "    pop %rsi\n"
                                 << "    xor %rax, %rax\n"
                                 << "    call printf\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_S)) {
                        asm_file << "    # -- dump --\n"
                                 << "    lea fmt_str(%rip), %rdi\n"
                                 << "    pop %rsi\n"
                                 << "    xor %rax, %rax\n"
                                 << "    call printf\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DROP)) {
                        asm_file << "    # -- drop --\n"
                                 << "    pop %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SWAP)) {
                        asm_file << "    # -- swap --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    push %rax\n"
                                 << "    push %rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OVER)) {
                        asm_file << "    # -- over --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    push %rbx\n"
                                 << "    push %rax\n"
                                 << "    push %rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHL)) {
                        asm_file << "    # -- bitwise-shift left --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shl %cl, %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHR)) {
                        asm_file << "    # -- bitwise-shift right --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shr %cl %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OR)) {
                        asm_file << "    # -- bitwise or --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    or %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::AND)) {
                        asm_file << "    # -- bitwise and --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    and %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
				    else if (tok.text == GetKeywordStr(Keyword::MOD)) {
                        asm_file << "    # -- modulo --\n"
                                 << "    xor %rdx, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    div %rbx\n"
                                 << "    push %rdx\n";
                    }
                }
                instr_ptr++;
            }
            // WRITE ASM FOOTER (GRACEFUL PROGRAM EXIT, CONSTANTS, MEMORY ALLOCATION)
            asm_file << "    mov $0, %rdi\n"
                     << "    call exit\n"
                     << '\n'
                     << "    .data\n"
                     << "    fmt: .string \"%u\\n\"\n"
                     << "    fmt_char: .string \"%c\"\n"
                     << "    fmt_str: .string \"%s\"\n";

            size_t index = 0;
            if (string_literals.size() > 0) { asm_file << "\n    # USER DEFINED STRINGS\n"; }
            for (auto& string : string_literals) {
                asm_file << "str_" << index << ": .string \"" << string << "\"\n";
                index++;
            }
            asm_file << '\n'
                     << "    .bss\n"
                     << "    .comm mem, " << MEM_CAPACITY << '\n';
            
            asm_file.close();
                
            Log("Linux x64 GAS assembly generated at " + asm_file_path);
        }
        else {
            Error("Could not open file for writing. Does directory exist?");
            exit(1);
        }
    }

    void GenerateAssembly_NASM_mac64(Program& prog) {
        std::string asm_file_path = OUTPUT_NAME + ".asm";
        std::fstream asm_file;
        asm_file.open(asm_file_path.c_str(), std::ios::out);
        if (asm_file) {
            Log("Generating NASM mac64 assembly");

            /* MacOS assembly
               I know it sounds scary, but it's really not too bad.
               It supports NASM, and can be linked using gcc which comes with XCode, the built in IDE.
               The calling convention is familiar (System V AMD64 ABI), so that's a win.
               The weird thing is all external symbols must be prefaced by an `_`. Why are you like this, Apple?
            */

            // TODO: WRITE HEADER
            asm_file << "    ;; CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
                     << "    ;; USING `SYSTEM V AMD64 ABI` CALLING CONVENTION (RDI, RSI, RDX, RCX, R8, R9, -> STACK)\n"
                     << "    ;; WHEN LINKING WITH OBJECTIVE-C, ALL EXTERNAL SYMBOLS MUST BE PREFIXED WITH '_'\n";

            // TODO: WRITE TOKENS

            // WRITE FOOTER (GRACEFUL EXIT AND CONSTANTS)
            asm_file << "    ;; -- TODO: graceful exit --\n"
                     << "\n\n"
                     << "    SECTION .data\n"
                     << "    fmt db '%u', 10, 0\n"
                     << "    fmt_char db '%c', 0\n"
                     << "    fmt_str db '%s', 0\n";
        }
        else {
            Error("Could not open file for writing. Does directory exist?");
            exit(1);
        }
    }

    void GenerateAssembly_NASM_win64(Program& prog) {
        // Loop through a lexed program and then generate assembly file from it.
        std::string asm_file_path = OUTPUT_NAME + ".asm";
        std::fstream asm_file;
        asm_file.open(asm_file_path.c_str(), std::ios::out);
        if (asm_file) {
            Log("Generating NASM win64 assembly");

            // String constants
            std::vector<std::string> string_literals;
            
            // WRITE HEADER TO ASM FILE
            asm_file << "    ;; CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
                     << "    ;; USING `WINDOWS x64` CALLING CONVENTION (RCX, RDX, R8, R9, ETC)\n"
                     << "    SECTION .text\n"
                     << "    ;; DEFINE EXTERNAL C RUNTIME SYMBOLS (LINK AGAINST MSVCRT.DLL)\n"
                     << "    extern printf\n"
                     << "    extern exit\n"
                     << "\n"
                     << "    global main\n"
                     << "main:\n";

            // WRITE TOKENS TO ASM FILE MAIN LABEL
            static_assert(static_cast<int>(TokenType::COUNT) == 5, "Exhaustive handling of token types in GenerateAssembly_NASM_win64");
            size_t instr_ptr = 0;
            size_t instr_ptr_max = prog.tokens.size();
            while (instr_ptr < instr_ptr_max) {
                Token& tok = prog.tokens[instr_ptr];
                if (tok.type == TokenType::INT){
                    asm_file << "    ;; -- push INT --\n"
                             << "    mov rax, " << tok.text << "\n"
                             << "    push rax\n";
                }
                else if (tok.type == TokenType::STRING) {
                    asm_file << "    ;; -- push STRING --\n"
                             << "    mov rax, str_" << string_literals.size() << '\n'
                             << "    push rax\n";
                    string_literals.push_back(tok.text);
                }
                else if (tok.type == TokenType::OP) {
                    static_assert(OP_COUNT == 15, "Exhaustive handling of operators in GenerateAssembly_NASM_win64");
                    if (tok.text == "+") {
                        asm_file << "    ;; -- add --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    add rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "-") {
                        asm_file << "    ;; -- subtract --\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    sub rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "*") {
                        asm_file << "    ;; -- multiply --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    mul rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "/") {
                        asm_file << "    ;; -- divide --\n"
                                 << "    xor rdx, rdx\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    div rbx\n"
                                 << "    push rax\n";
                    }
					else if (tok.text == "%") {
                        asm_file << "    ;; -- modulo --\n"
                                 << "    xor rdx, rdx\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    div rbx\n"
                                 << "    push rdx\n";
                    }
                    else if (tok.text == "=") {
                        asm_file << "    ;; -- equality condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmove rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == "<") {
                        asm_file << "    ;; -- less than condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovl rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == ">") {
                        asm_file << "    ;; -- greater than condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovg rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == "<=") {
                        asm_file << "    ;; -- less than or equal condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovle rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == ">=") {
                        asm_file << "    ;; -- greater than or equal condition --\n"
                                 << "    mov rcx, 0\n"
                                 << "    mov rdx, 1\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, rbx\n"
                                 << "    cmovge rcx, rdx\n"
                                 << "    push rcx\n";
                    }
                    else if (tok.text == "<<") {
                        asm_file << "    ;; -- bitwise-shift left --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shl rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == ">>") {
                        asm_file << "    ;; -- bitwise-shift right --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shr rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == "||") {
                        asm_file << "    ;; -- bitwise or --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    or rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "&&") {
                        asm_file << "    ;; -- bitwise and --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    and rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == "#") {
                        // A call in Windows x64 requires shadow space on the stack
                        // This is also called spill space or home space
                        // It's basically space on the stack the called function
                        //   will assume to be usable and over-writable
                        // In Corth, a stack-based language, this obviously causes issues
                        //   if I don't handle it correctly.
                        asm_file << "    ;; -- dump --\n"
                                 << "    lea rcx, [rel fmt]\n"
                                 << "    pop rdx\n"
                                 << "    xor rax, rax\n"
                                 << "    sub rsp, 32\n"
                                 << "    call printf\n"
                                 << "    add rsp, 32\n";
                    }
                }
                else if (tok.type == TokenType::KEYWORD) {
                    static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in GenerateAssembly_NASM_win64");
                    if (tok.text == GetKeywordStr(Keyword::IF)) {
                        asm_file << "    ;; -- if --\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, 0\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ELSE)) {
                        asm_file << "    ;; -- else --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDIF)) {
                        asm_file << "    ;; -- endif --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUP)) {
                        asm_file << "    ;; -- dup --\n"
                                 << "    pop rax\n"
                                 << "    push rax\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::TWODUP)) {
                        asm_file << "    ;; -- twodup --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    push rbx\n"
                                 << "    push rax\n"
                                 << "    push rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::WHILE)) {
                        asm_file << "    ;; -- while --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DO)) {
                        asm_file << "    ;; -- do --\n"
                                 << "    pop rax\n"
                                 << "    cmp rax, 0\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDWHILE)) {
                        asm_file << "    ;; -- endwhile --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::MEM)) {
                        // Pushes the relative address of allocated memory onto the stack
                        asm_file << "    ;; -- mem --\n"
                                 << "    push mem\n";
                                
                    }
                    else if (tok.text == GetKeywordStr(Keyword::LOADB)) {
                        asm_file << "    ;; -- load byte --\n"
                                 << "    pop rax\n"
                                 << "    xor rbx, rbx\n"
                                 << "    mov bl, [rax]\n"
                                 << "    push rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::STOREB)) {
                        asm_file << "    ;; -- store byte --\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    mov [rax], bl\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP)) {
                        asm_file << "    ;; -- dump --\n"
                                 << "    lea rcx, [rel fmt]\n"
                                 << "    pop rdx\n"
                                 << "    xor rax, rax\n"
                                 << "    sub rsp, 32\n"
                                 << "    call printf\n"
                                 << "    add rsp, 32\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_C)) {
                        asm_file << "    ;; -- dump --\n"
                                 << "    lea rcx, [rel fmt_char]\n"
                                 << "    pop rdx\n"
                                 << "    xor rax, rax\n"
                                 << "    sub rsp, 32\n"
                                 << "    call printf\n"
                                 << "    add rsp, 32\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_S)) {
                        asm_file << "    ;; -- dump --\n"
                                 << "    lea rcx, [rel fmt_str]\n"
                                 << "    pop rdx\n"
                                 << "    xor rax, rax\n"
                                 << "    sub rsp, 32\n"
                                 << "    call printf\n"
                                 << "    add rsp, 32\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DROP)) {
                        asm_file << "    ;; -- drop --\n"
                                 << "    pop rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SWAP)) {
                        asm_file << "    ;; -- swap --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    push rax\n"
                                 << "    push rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OVER)) {
                        asm_file << "    ;; -- over --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    push rbx\n"
                                 << "    push rax\n"
                                 << "    push rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHL)) {
                        asm_file << "    ;; -- bitwise-shift left --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shl rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHR)) {
                        asm_file << "    ;; -- bitwise-shift right --\n"
                                 << "    pop rcx\n"
                                 << "    pop rbx\n"
                                 << "    shr rbx, cl\n"
                                 << "    push rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OR)) {
                        asm_file << "    ;; -- bitwise or --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    or rax, rbx\n"
                                 << "    push rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::AND)) {
                        asm_file << "    ;; -- bitwise and --\n"
                                 << "    pop rax\n"
                                 << "    pop rbx\n"
                                 << "    and rax, rbx\n"
                                 << "    push rax\n";
                    }
					else if (tok.text == GetKeywordStr(Keyword::MOD)) {
                        asm_file << "    ;; -- modulo --\n"
                                 << "    xor rdx, rdx\n"
                                 << "    pop rbx\n"
                                 << "    pop rax\n"
                                 << "    div rbx\n"
                                 << "    push rdx\n";
                    }
                }
                instr_ptr++;
            }
            // EXIT GRACEFUL
            asm_file << "    mov rcx, 0\n"
                     << "    call exit\n\n";

            // DECLARE CORTH CONSTANTS
            asm_file << "    SECTION .data\n"
                     << "    fmt db '%u', 10, 0\n"
                     << "    fmt_char db '%c', 0\n"
                     << "    fmt_str db '%s', 0\n";

            // DECLARE USER-DEFINED STRING CONSTANTS HERE
            size_t index = 0;
            for (auto& string : string_literals) {
                std::vector<std::string> hex_chars = string_to_hex(string);
                asm_file << "str_" << index << " db ";
                for (auto& c : hex_chars) {
                    asm_file << c << ',';
                }
                // Null-terminator
                asm_file << "0\n";
                index++;
            }

            // ALLOCATE MEMORY
            asm_file << "\n"
                     << "    SECTION .bss\n"
                     << "    mem resb " << MEM_CAPACITY << "\n";

            asm_file.close();
            Log("NASM win64 assembly generated at " + asm_file_path);
        }
        else {
            Error("Could not open file for writing. Does directory exist?");
            exit(1);
        }
    }

    void GenerateAssembly_GAS_win64(Program& prog) {
        std::string asm_file_path = OUTPUT_NAME + ".s";
        std::fstream asm_file;
        asm_file.open(asm_file_path.c_str(), std::ios::out);
        if (asm_file) {
            Log("Generating WIN64 GAS assembly");

            // Save list of defined strings in file to write at the end of the assembly in the `.data` section.
            std::vector<std::string> string_literals;

            // WRITE HEADER TO ASM FILE
            asm_file << "    # CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
                     << "    # USING `GAS` SYNTAX\n"
                     << "    # USING `WINDOWS X64` CALLING CONVENTION (RCX, RDX, R8, R9, -> STACK)\n"
                     << "    .text\n"
                     << "    .globl main\n"
                     << "main:\n";

            // WRITE TOKENS TO ASM FILE MAIN LABEL
            static_assert(static_cast<int>(TokenType::COUNT) == 5, "Exhaustive handling of token types in GenerateAssembly_GAS_win64");
            size_t instr_ptr = 0;
            size_t instr_ptr_max = prog.tokens.size();
            while (instr_ptr < instr_ptr_max) {
                Token& tok = prog.tokens[instr_ptr];
                // Write assembly to opened file based on token type and value
                if (tok.type == TokenType::INT) {
                    asm_file << "    # -- push INT --\n"
                             << "    mov $"  << tok.text << ", %rax" << "\n"
                             << "    push %rax\n";
                }
                else if (tok.type == TokenType::STRING) {
                    asm_file << "    # -- push STRING --\n"
                             << "    lea str_" << string_literals.size() << "(%rip), %rax\n"
                             << "    push %rax\n";
                    // String value is stored in tok.text
                    string_literals.push_back(tok.text);
                }
                else if (tok.type == TokenType::OP) {
                    static_assert(OP_COUNT == 15, "Exhaustive handling of operators in GenerateAssembly_GAS_win64");
                    if (tok.text == "+") {
                        asm_file << "    # -- add --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    add %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "-") {
                        asm_file << "    # -- subtract --\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    sub %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "*") {
                        asm_file << "    # -- multiply --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    mul %rbx\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "/") {
                        asm_file << "    # -- divide --\n"
                                 << "    xor %rdx, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    div %rbx\n"
                                 << "    push %rax\n";
                    }
					else if (tok.text == "%") {
						asm_file << "    # -- modulo --\n"
                                 << "    xor %rdx, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    div %rbx\n"
                                 << "    push %rdx\n";
					}
                    else if (tok.text == "=") {
                        asm_file << "    # -- equality condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmove %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == "<") {
                        asm_file << "    # -- less than condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovl %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == ">") {
                        asm_file << "    # -- greater than condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovg %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == "<=") {
                        asm_file << "    # -- less than or equal condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovle %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == ">=") {
                        asm_file << "    # -- greater than or equal condition --\n"
                                 << "    mov $0, %rcx\n"
                                 << "    mov $1, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    cmp %rbx, %rax\n"
                                 << "    cmovge %rdx, %rcx\n"
                                 << "    push %rcx\n";
                    }
                    else if (tok.text == "<<") {
                        asm_file << "    # -- bitwise-shift left --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shl %cl, %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == ">>") {
                        asm_file << "    # -- bitwise-shift right --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shr %cl, %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == "||") {
                        asm_file << "    # -- bitwise or --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    or %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "&&") {
                        asm_file << "    # -- bitwise and --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    and %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == "#") {
                        asm_file << "    # -- dump --\n"
                                 << "    lea fmt(%rip), %rcx\n"
                                 << "    pop %rdx\n"
                                 << "    xor %rax, %rax\n"
                                 << "    sub $32, %rsp\n"
                                 << "    call printf\n"
                                 << "    add $32, %rsp\n";
                    }
                }
                else if (tok.type == TokenType::KEYWORD) {
                    static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of token types in GenerateAssembly_GAS_win64");
                    if (tok.text == GetKeywordStr(Keyword::IF)) {
                        asm_file << "    # -- if --\n"
                                 << "    pop %rax\n"
                                 << "    cmp $0, %rax\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ELSE)) {
                        asm_file << "    # -- else --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDIF)) {
                        asm_file << "    # -- endif --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUP)) {
                        asm_file << "    # -- dup --\n"
                                 << "    pop %rax\n"
                                 << "    push %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::TWODUP)) {
                        asm_file << "    # -- twodup --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    push %rbx\n"
                                 << "    push %rax\n"
                                 << "    push %rbx\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::WHILE)) {
                        asm_file << "    # -- while --\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DO)) {
                        asm_file << "    # -- do --\n"
                                 << "    pop %rax\n"
                                 << "    cmp $0, %rax\n"
                                 << "    je addr_" << tok.data << "\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::ENDWHILE)) {
                        asm_file << "    # -- endwhile --\n"
                                 << "    jmp addr_" << tok.data << "\n"
                                 << "addr_" << instr_ptr << ":\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::MEM)) {
                        asm_file << "    # -- mem --\n"
                                 << "    lea mem(%rip), %rax\n"
                                 << "    push %rax\n";
                        // Pushes the relative address of allocated memory onto the stack
                    }
                    else if (tok.text == GetKeywordStr(Keyword::LOADB)) {
                        asm_file << "    # -- load byte --\n"
                                 << "    pop %rax\n"
                                 << "    xor %rbx, %rbx\n"
                                 << "    mov (%rax), %bl\n"
                                 << "    push %rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::STOREB)) {
                        asm_file << "    # -- store byte --\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    mov %bl, (%rax)\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP)) {
                        asm_file << "    # -- dump --\n"
                                 << "    lea fmt(%rip), %rcx\n"
                                 << "    pop %rdx\n"
                                 << "    xor %rax, %rax\n"
                                 << "    sub $32, %rsp\n"
                                 << "    call printf\n"
                                 << "    add $32, %rsp\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_C)) {
                        asm_file << "    # -- dump character --\n"
                                 << "    lea fmt_char(%rip), %rcx\n"
                                 << "    pop %rdx\n"
                                 << "    xor %rax, %rax\n"
                                 << "    sub $32, %rsp\n"
                                 << "    call printf\n"
                                 << "    add $32, %rsp\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DUMP_S)) {
                        asm_file << "    # -- dump string --\n"
                                 << "    lea fmt_str(%rip), %rcx\n"
                                 << "    pop %rdx\n"
                                 << "    xor %rax, %rax\n"
                                 << "    sub $32, %rsp\n"
                                 << "    call printf\n"
                                 << "    add $32, %rsp\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::DROP)) {
                        asm_file << "    # -- drop --\n"
                                 << "    pop %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SWAP)) {
                        asm_file << "    # -- swap --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    push %rax\n"
                                 << "    push %rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OVER)) {
                        asm_file << "    # -- over --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    push %rbx\n"
                                 << "    push %rax\n"
                                 << "    push %rbx\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHL)) {
                        asm_file << "    # -- bitwise-shift left --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shl %cl, %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::SHR)) {
                        asm_file << "    # -- bitwise-shift right --\n"
                                 << "    pop %rcx\n"
                                 << "    pop %rbx\n"
                                 << "    shr %cl %rbx\n"
                                 << "    push %rbx";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::OR)) {
                        asm_file << "    # -- bitwise or --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    or %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
                    else if (tok.text == GetKeywordStr(Keyword::AND)) {
                        asm_file << "    # -- bitwise and --\n"
                                 << "    pop %rax\n"
                                 << "    pop %rbx\n"
                                 << "    and %rbx, %rax\n"
                                 << "    push %rax\n";
                    }
					else if (tok.text == GetKeywordStr(Keyword::MOD)) {
						asm_file << "    # -- modulo --\n"
                                 << "    xor %rdx, %rdx\n"
                                 << "    pop %rbx\n"
                                 << "    pop %rax\n"
                                 << "    div %rbx\n"
                                 << "    push %rdx\n";
					}
                }
                instr_ptr++;
            }
            // WRITE ASM FOOTER (GRACEFUL PROGRAM EXIT, CONSTANTS, MEMORY ALLOCATION)
            asm_file << "    mov $0, %rcx\n"
                     << "    call exit\n"
                     << '\n'
                     << "    .data\n"
                     << "    fmt: .string \"%u\\n\"\n"
                     << "    fmt_char: .string \"%c\"\n"
                     << "    fmt_str: .string \"%s\"\n";

            size_t index = 0;
            if (string_literals.size() > 0) { asm_file << "\n    # USER DEFINED STRINGS\n"; }
            for (auto& string : string_literals) {
                asm_file << "str_" << index << ": .string \"" << string << "\"\n";
                index++;
            }
            asm_file << '\n'
                     << "    .bss\n"
                     << "    .comm mem, " << MEM_CAPACITY << '\n';
            
            asm_file.close();
                
            Log("WIN64 GAS assembly generated at " + asm_file_path);
        }
        else {
            Error("Could not open file for writing. Does directory exist?");
            exit(1);
        }
    }

    bool HandleCMDLineArgs(int argc, char** argv) {
        // Return value:
        // False = Execution will halt in main function
        // True = Execution will continue in main function
        static_assert(static_cast<int>(MODE::COUNT) == 2, "Exhaustive handling of supported modes in HandleCMDLineArgs");
        static_assert(static_cast<int>(PLATFORM::COUNT) == 2, "Exhaustive handling of supported platforms in HandleCMDLineArgs");
        static_assert(static_cast<int>(ASM_SYNTAX::COUNT) == 2, "Exhaustive handling of supported assembly syntaxes in HandleCMDLineArgs");

        if (argc == 1) {
            // No command line args entered, print usage
            PrintUsage();
            return false;
        }
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                PrintUsage();
                return false;
            }
            else if (arg == "-v" || arg == "--verbose") {
                Log("Verbose logging enabled");
                verbose_logging = true;
            }
            else if (arg == "-o" || arg == "--output-name") {
                if (i + 1 < argc) {
                    i++;
                    OUTPUT_NAME = argv[i];
                }
                else {
                    Error("Expected name of output file to be specified after `-o`!");
                    return false;
                }
            }
            else if (arg == "-a" || arg == "--assembler-path") {
                if (i + 1 < argc) {
                    i++;
                    ASMB_PATH = argv[i];
                }
                else {
                    Error("Expected path to assembler to be specified after `-a`!");
                    return false;
                }
            }
            else if (arg == "-ao" || arg == "--assembler-options") {
                if (i + 1 < argc) {
                    i++;
                    ASMB_OPTS = argv[i];
                }
                else {
                    Error("Expected assembler options to be specified afer `-ao`!");
                    return false;
                }
            }
            else if (arg == "-l" || arg == "--linker-path") {
                if (i + 1 < argc) {
                    i++;
                    LINK_PATH = argv[i];
                }
                else {
                    Error("Expected path to linker to be specified after `-l`!");
                    return false;
                }
            }
            else if (arg == "-lo" || arg == "--linker-options") {
                if (i + 1 < argc) {
                    i++;
                    LINK_OPTS = argv[i];
                }
                else {
                    Error("Expected linker options to be specified after `-lo`!");
                    return false;
                }
            }
            else if (arg == "-add-ao" || arg == "--add-asm-opt") {
                if (i + 1 < argc) {
                    i++;
                    ASMB_OPTS.append(1, ' ');
                    ASMB_OPTS += argv[i];
                }
                else {
                    Error("Expected an assembler command line option to be specified after `-add-ao`!");
                    return false;
                }
            }
            else if (arg == "-add-lo" || arg == "--add-link-opt") {
                if (i + 1 < argc) {
                    i++;
                    LINK_OPTS.append(1, ' ');
                    LINK_OPTS += argv[i];                   
                }
                else {
                    Error("Expected a linker command line option to be specified after `-add-lo`!");
                    return false;
                }
            }
            else if (arg == "-win" || arg == "-win64" ) {
                RUN_PLATFORM = PLATFORM::WIN64;
            }
            else if (arg == "-linux" || arg == "-linux64") {
                RUN_PLATFORM = PLATFORM::LINUX64;
            }
            else if (arg == "-win32" || arg == "-m32" || arg == "-linux32" || arg == "-Wa,--32") {
                Error("32-bit mode is not supported!");
                return false;
            }
            else if (arg == "-com" || arg == "--compile") {
                RUN_MODE = MODE::COMPILE;
            }
            else if (arg == "-gen" || arg == "--generate") {
                RUN_MODE = MODE::GENERATE;
            }
            else if (arg == "-NASM") {
                ASSEMBLY_SYNTAX = ASM_SYNTAX::NASM;
                // PLATFORM SPECIFIC DEFAULTS
                #ifdef _WIN64
                // Defaults assume tools were installed on the same drive as Corth as well as in the root directory of the drive.
                Corth::ASMB_PATH = "\\NASM\\nasm.exe";
                Corth::ASMB_OPTS = "-f win64";
                Corth::LINK_PATH = "\\Golink\\golink.exe";
                Corth::LINK_OPTS = "/console /ENTRY:main msvcrt.dll";
                #endif

                #ifdef __linux__
                Corth::ASMB_PATH = "nasm";
                Corth::ASMB_OPTS = "-f elf64";
                Corth::LINK_PATH = "ld";
                Corth::LINK_OPTS = "-dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -m elf_x86_64";
                #endif  
            }
            else if (arg == "-GAS") {
                ASSEMBLY_SYNTAX = ASM_SYNTAX::GAS;
                #ifdef _WIN64
                // Defaults assume tools were installed on the same drive as Corth as well as in the root directory of the drive.
                Corth::ASMB_PATH = "\\TDM-GCC-64\\bin\\gcc.exe";
                Corth::ASMB_OPTS = "-e main";
                Corth::LINK_PATH = "";
                Corth::LINK_OPTS = "";
                #endif

                #ifdef __linux__
                Corth::ASMB_PATH = "gcc";
                Corth::ASMB_OPTS = "-e main";
                Corth::LINK_PATH = "";
                Corth::LINK_OPTS = "";
                #endif  
            }
            else {
                SOURCE_PATH = argv[i];
            }
        }

        if (SOURCE_PATH.empty()) {
            Error("Expected source file path in command line arguments!");
            return false;
        }
    
        return true;
    }

    bool iswhitespace(char& c) {
        return c == ' '     // spaces
            || c == '\t'    // tab
            || c == '\r'    // crlf
            || c == '\n';   // newline/linefeed
    }

    void PushToken(std::vector<Token>& tokList, Token& tok) {
        // Add token to program if it is not whitespace
        if (tok.type != TokenType::WHITESPACE) {
            tokList.push_back(tok);
        }
        // Reset token
        tok.type = TokenType::WHITESPACE;
        tok.text.erase();
    }

    // Convert program source into tokens
    bool Lex(Program& prog) {
        std::string src = prog.source;
        size_t src_end = src.size();

        static_assert(static_cast<int>(TokenType::COUNT) == 5, "Exhaustive handling of token types in Lex method");
        
        std::vector<Token>& toks = prog.tokens;
        Token tok;
        char current = src[0];
        
        for(int i = 0; i < src_end; i++) {
            current = src[i];
            
            tok.col_number++;

            // Skip whitespace
            if (iswhitespace(current)) {
                if (current == '\n') {
                    tok.line_number++;
                    tok.col_number = 1;
                }
            }
            else if (isoperator(current)) {
                static_assert(OP_COUNT == 15, "Exhaustive handling of operators in Lex method");
                tok.type = TokenType::OP;
                tok.text.append(1, current);
                // Look-ahead to check for multi-character operators
                i++;
                current = src[i];
                if ((tok.text == "=" && current == '=')
                    || (tok.text == "<" && current == '=')
                    || (tok.text == ">" && current == '=')
                    || (tok.text == "<" && current == '<')
                    || (tok.text == ">" && current == '>'))
                {
                    tok.text.append(1, current);
                }
                else if (tok.text == "|") {
                    if (current == '|') {
                        tok.text.append(1, current);
                    }
                    else {
                        Warning("Expected '|' following '|'", tok.line_number, tok.col_number);
                        tok.text.append(1, '|'); // Create missing text so it will still work
                    }
                }
                else if (tok.text == "&") {
                    if (current == '&') {
                        tok.text.append(1, current);
                    }
                    else {
                        Warning("Expected '&' following '&'", tok.line_number, tok.col_number);
                        tok.text.append(1, '&'); // Create missing text so it will still work
                    }
                }
                else if (tok.text == "/" && current == '/') {
                    // This is a comment until new-line or end of file
                    tok.type = TokenType::WHITESPACE;
                    while (i < src_end) {
                        i++;
                        current = src[i];
                        tok.col_number++;
                        if (current == '\n') {
                            tok.line_number++;
                            tok.col_number = 1;
                            break;
                        }
                    }
                }
                PushToken(toks, tok);
            }
            else if (isdigit(current)) {
                tok.type = TokenType::INT;
                tok.text.append(1, current);
                // Handle multi-digit numbers
                while (i < src_end) {
                    // Look ahead for digit
                    i++;
                    current = src[i];
                    tok.col_number++;
                    if (isdigit(current)) { tok.text.append(1, current); }
                    else { break; }
                }
                i--; // Undo lookahead.
                PushToken(toks, tok);
            }
            else if (isalpha(current)) {
                tok.text.append(1, current);
                // Handle multiple-alpha keywords
                while (i < src_end) {
                    // Look ahead for alpha
                    i++;
                    current = src[i];
                    tok.col_number++;
                    if (isalpha(current)
						|| current == '_')
					{
						tok.text.append(1, current);
					}
                    else { break; }
                }

                // If the token is not a keyword, output a warning and
                // keep this token's type as whitespace
                // (effectively ensuring it will be removed)
                if (iskeyword(tok.text)) {
                    tok.type = TokenType::KEYWORD;
                }
                else {
                    Error("Unidentified keyword: " + tok.text,
                            tok.line_number, tok.col_number);
					return false;
                }
                
                i--; // Undo lookahead.
                PushToken(toks, tok);
            }
            else if (current == '"') {
                tok.type = TokenType::STRING;
                // Eat quotes
                i++;
                current = src[i];
                tok.col_number++;
                // Find closing quotes
                while (i < src_end) {
                    if (current == '"') {
                        // Eat closing quotes
                        i++;
                        current = src[i];
                        tok.col_number++;
                        break;
                    }
                    else { tok.text.append(1, current);}
                    // Increment current character
                    i++;
                    current = src[i];
                    tok.col_number++;
                }
                
                if (i >= src_end) {
                    Error("Expected closing quotes following opening quotes",
                          tok.line_number, tok.col_number);
					return false;
                }
                i--;
                PushToken(toks, tok);
            }
        }
		return true;
    }

    void PrintToken(Token& t) {
        if (t.data.empty()) {
            printf("TOKEN(%s, %s)\n", TokenTypeStr(t.type).c_str(), t.text.c_str());
        }
        else {
            printf("TOKEN(%s, %s, %s)\n", TokenTypeStr(t.type).c_str(), t.text.c_str(), t.data.c_str());
        }           
    }

    void PrintTokens(Program& p) {
        printf("%s\n", "TOKENS:");
        size_t instr_ptr = 0;
        size_t instr_ptr_max = p.tokens.size();
        for (size_t instr_ptr = 0; instr_ptr < instr_ptr_max; instr_ptr++) {
            printf("    %zu: ", instr_ptr);
            PrintToken(p.tokens[instr_ptr]);
        }
    }

    bool RemovableToken(Token& tok) {
        if (tok.type == TokenType::WHITESPACE) { return false; }
        return true;
    }

    void TokenStackError(Token& tok) {
        // This token could cause serious memory issues (by popping a value off the stack that doesn't exist)
        // It is marked for removal by setting it's type to whitespace.
        tok.type = TokenType::WHITESPACE;
        StackError(tok.line_number, tok.col_number);
    }

	// This function simulates the program running on its most basic level: adding and removing from the stack.
	// A read-access violation can occur if trying to pop off the stack without pushing something on first.
	// To prevent this violation during runtime of Corth, this function catches over-popping before it can happen.
	void ValidateTokens_Stack(Program& prog) {
		std::vector<Token>& toks = prog.tokens;
		// Amount of things on virtual stack
		size_t stackSize = 0;
		static_assert(static_cast<int>(TokenType::COUNT) == 5, "Exhaustive handling of token types in ValidateTokens_Stack");
		for (auto& tok : toks) {
			if (tok.type == TokenType::WHITESPACE) {
				Warning("Validator: Whitespace tokens should not appear in final program. Problem with the Lexing?", tok.line_number, tok.col_number);
			}
			else if (tok.type == TokenType::INT
					 || tok.type == TokenType::STRING)
			{
				stackSize++;
			}
			else if (tok.type == TokenType::OP) {
				static_assert(OP_COUNT == 15, "Exhaustive handling of operators in ValidateTokens_Stack.");
				// Operators that pop two values off the stack and return one to it
				if (tok.text == "+"
					|| tok.text == "-"
					|| tok.text == "*"
					|| tok.text == "/"
					|| tok.text == "%"
					// conditionals
					|| tok.text == "="
					|| tok.text == "<"
					|| tok.text == ">"
					|| tok.text == "<="
					|| tok.text == ">="
					// bitwise
					|| tok.text == "<<"
					|| tok.text == ">>"
					|| tok.text == "||"
					|| tok.text == "&&")
				{
					if (stackSize > 1) {
						stackSize--; // Two values removed, result added, net loss of one
					}
					else {
						TokenStackError(tok);
					}
				}
				// Operators that pop one value off the stack and return zero to it
				else if (tok.text == "#") {
					if (stackSize > 0) {
						stackSize--;
					}
					else {
						TokenStackError(tok);
					}
				}
			}
			else if (tok.type == TokenType::KEYWORD) {
				static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in ValidateTokens_Stack. Keep in mind not all keywords do stack operations");
				// Skip skippable tokens first for speed
				if (tok.text == GetKeywordStr(Keyword::ELSE)
					|| tok.text == GetKeywordStr(Keyword::ENDIF)
					|| tok.text == GetKeywordStr(Keyword::WHILE)
					|| tok.text == GetKeywordStr(Keyword::ENDWHILE))
				{
					continue;
				}
				else if (tok.text == GetKeywordStr(Keyword::IF)
						 || tok.text == GetKeywordStr(Keyword::DO))
				{
					// both `if` and `do` will pop from the stack to check the condition to see if it needs to jump or not
					// {<condition>} -> { }
					if (stackSize > 0) {
						stackSize--;
					}
					else {
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::DUP)) {
					// dup will pop from the stack then push that value back twice
					if (stackSize > 0) {
						stackSize++;
					}
					else {
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::TWODUP)) {
					// dup will pop from the stack then push that value back twice
					if (stackSize > 1) {
						stackSize += 2;
					}
					else {
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::MEM)) {
					// `mem` will push the address of usable memory onto the stack
					// { } -> {<memory address>}
					stackSize++;
				}
				else if (tok.text == GetKeywordStr(Keyword::LOADB)) {
					// `loadb` will pop an address from the stack then push the value at that address
					// {<address>} -> {<value>}
					if (stackSize > 0) {
						continue;
					}
					else {
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::STOREB)) {
					// `storeb` will pop a value and an address from the stack
					// {<address>, <value>} -> { }
					if (stackSize > 1) {
						stackSize -= 2;
					}
					else {
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::DUMP)
						 || tok.text == GetKeywordStr(Keyword::DUMP_C)
						 || tok.text == GetKeywordStr(Keyword::DUMP_S)
						 || tok.text == GetKeywordStr(Keyword::DROP))
				{
					// both `dump`, `dump_c`, `dump_s`, and `drop` will
					//   take an item off the stack without returning anything
					// {a} -> { }
					if (stackSize > 0) {
						stackSize--;
					}
					else {
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::SWAP)) {
					// `swap` will pop two values but also push two values, net zero.
					if (stackSize > 1) {
						continue;
					}
					else
					{
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::OVER)) {
					// Over will pop two values but push three values, net one
					// {a, b} -> {a, b, a}
					if (stackSize > 1) {
						stackSize++;
					}
					else
					{
						TokenStackError(tok);
					}
				}
				else if (tok.text == GetKeywordStr(Keyword::SHL)
						 || tok.text == GetKeywordStr(Keyword::SHR)
						 || tok.text == GetKeywordStr(Keyword::OR)
						 || tok.text == GetKeywordStr(Keyword::AND)
						 || tok.text == GetKeywordStr(Keyword::MOD))
				{
					// Bitwise-shift left and right, bitwise-or and bitwise-and, as well as modulo will pop two
					//   values off the stack and add one, net negative one
					// {a, b} -> {c}
					if (stackSize > 1) {
						stackSize--;
					}
					else
					{
						TokenStackError(tok);
					}
				}
			}
		}
		
		if (stackSize != 0) {
			Warning("Validator: Best practices indicate stack should be empty at end of program.\nStack Size at End of Program: " + std::to_string(stackSize));
		}
	}

	bool ValidateBlock(Program& prog, size_t& instr_ptr, size_t instr_ptr_max) {
	    // Assume that current token at instruction pointer is an `if`, `else`, `do`, or `while`
		size_t block_instr_ptr = instr_ptr;

		static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in ValidateBlock. Keep in mind not all keywords form blocks.");
		
		// Handle while block
		if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::WHILE)) {
			// Find `do`, error if you can't. Set `do` data field to WHILE instr_ptr temporarily for endwhile to use
			while (instr_ptr < instr_ptr_max) {
				if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::DO)) {
					prog.tokens[instr_ptr].data = std::to_string(block_instr_ptr);
					return ValidateBlock(prog, instr_ptr, instr_ptr_max);
					// Undo look-ahead
					instr_ptr--;
				}
				else if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::IF)
						 || prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::ELSE)
						 || prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::ENDIF)
						 || instr_ptr + 1 == instr_ptr_max)
				{
					Error("Expected `" + GetKeywordStr(Keyword::DO)
						  + "` following `" + GetKeywordStr(Keyword::WHILE) + "`",
						  prog.tokens[instr_ptr].line_number,
						  prog.tokens[instr_ptr].col_number);
					return false;
				}
				instr_ptr++;
			}	
		}
		
		while (instr_ptr < instr_ptr_max) {
			instr_ptr++;
				
			if (prog.tokens[instr_ptr].type == TokenType::KEYWORD) {
				if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::IF)) {
					// Recursively validate nested if
					ValidateBlock(prog, instr_ptr, instr_ptr_max);
				}
				else if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::ELSE)) {
					if (prog.tokens[block_instr_ptr].text == GetKeywordStr(Keyword::IF)) {
						// Upon an `if` reaching an `else`, the `if` data field
						//   should be updated to the `else` instr_ptr
						prog.tokens[block_instr_ptr].data = std::to_string(instr_ptr);
						// Recursively validate else block
						return ValidateBlock(prog, instr_ptr, instr_ptr_max);
					}
					else {
						Error("`" + GetKeywordStr(Keyword::ELSE)
							  + "` keyword can only be used within `"
							  + GetKeywordStr(Keyword::IF) + "` blocks",
							  prog.tokens[instr_ptr].line_number,
							  prog.tokens[instr_ptr].col_number);
						return false;
					}
				}
				else if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::WHILE)) {
					// Recursively validate nested while
					ValidateBlock(prog, instr_ptr, instr_ptr_max);
				}
				else if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::ENDWHILE)) {
					if (prog.tokens[block_instr_ptr].text == GetKeywordStr(Keyword::DO)) {
						// An endwhile must set the `do` data field to
						//    it's instruction pointer (to jump to upon condition fail)
						// It must first set it's own data field to it's jump point,
						//   which is the `do`s data field before we change it
						prog.tokens[instr_ptr].data = prog.tokens[block_instr_ptr].data;
						prog.tokens[block_instr_ptr].data = std::to_string(instr_ptr);
						return true;
					}
					Error("`" + GetKeywordStr(Keyword::ENDWHILE)
						  + "` keyword can only be used within `"
						  + GetKeywordStr(Keyword::DO) + "` blocks",
						  prog.tokens[instr_ptr].line_number,
						  prog.tokens[instr_ptr].col_number);
					return false;
				}
				else if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::ENDIF)) {
					if (prog.tokens[block_instr_ptr].text == GetKeywordStr(Keyword::IF)
						|| prog.tokens[block_instr_ptr].text == GetKeywordStr(Keyword::ELSE))
					{
						prog.tokens[block_instr_ptr].data = std::to_string(instr_ptr);
						return true;
					}
					Error("`" + GetKeywordStr(Keyword::ENDIF)
						  + "` keyword can only be used within `"
						  + GetKeywordStr(Keyword::IF) + "` blocks",
						  prog.tokens[instr_ptr].line_number,
						  prog.tokens[instr_ptr].col_number);
					return false;
				}
			} 
		}	
		
		return false;
	}

	// This function ensures any tokens that start or stop blocks are correctly referenced
	// For example, an `if` statement needs to know where to jump to if it is false.
	// Another example: `endwhile` statement needs to know where to jump back to.
	void ValidateTokens_Blocks(Program& prog) {
		static_assert(static_cast<int>(Keyword::COUNT) == 22, "Exhaustive handling of keywords in ValidateTokens_Blocks. Keep in mind not all tokens form blocks");
		size_t instr_ptr = 0;
		size_t instr_ptr_max = prog.tokens.size();
		while (instr_ptr < instr_ptr_max) {
			if (prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::IF)
				|| prog.tokens[instr_ptr].text == GetKeywordStr(Keyword::WHILE))
			{
				ValidateBlock(prog, instr_ptr, instr_ptr_max);
				// Undo look-ahead
				instr_ptr--;
			}
			instr_ptr++;
		}
	}
	
	void ValidateTokens(Program& prog) {
		// Stack protection
		ValidateTokens_Stack(prog);

		// Cross-reference blocks (give `if` tokens a reference to it's `endif` counterpart
		ValidateTokens_Blocks(prog);

		// Remove all un-neccessary tokens
	    static_cast<void>(std::remove_if(prog.tokens.begin(), prog.tokens.end(), RemovableToken));

		if (verbose_logging) { Log("Tokens validated"); }
	}
}

// This function is my Windows version of the `where` cmd
bool FileExists(std::string filePath) {
	// Check path relative Corth.exe
	std::ifstream file(filePath);
	if (file.is_open()) { file.close(); return true; }
	
    #ifdef _WIN64
	// Get PATH system variable on Windows
	std::vector<std::string> path_var;
	size_t buf_sz = 2048;
	char* buf[2048];
	if (_dupenv_s(buf, &buf_sz, "PATH")) {
		Corth::Error("Could not access PATH system variable");
	}
	// Split path by semi-colon, and ensure there is a slash at the end of every split.
	std::string tmp;
	std::string path_var_str(buf[0], buf_sz);
	std::stringstream path_var_ss(path_var_str);
	while (std::getline(path_var_ss, tmp, ';')) {
		if (tmp.back() != '\\' || tmp.back() != '/') {
			tmp.append(1, '/');
		}
	   	path_var.push_back(tmp);
    }

    // Check each path in Windows PATH variable if file exists
	for (auto& path : path_var) {
		std::string test(path + filePath);
		if (Corth::verbose_logging) { Corth::Log("Testing " + test); }
		std::ifstream f(test);
		if (f.is_open()) { f.close(); return true; }
	}
	#else
	#endif

	return false;
}

// Load a file into a string from a path.
std::string loadFromFile(std::string filePath) {
    std::ifstream inFileStream(filePath);
    if (!inFileStream) {
		throw std::runtime_error(("File not found at " + filePath).c_str());
	}
    return std::string(std::istreambuf_iterator<char>(inFileStream), std::istreambuf_iterator<char>());
}

void printCharactersFromFile(std::string filePath, std::string logPrefix = "[LOG]") {
	FILE* file_ptr {nullptr};
	char c;
	file_ptr = fopen(filePath.c_str(), "r");
	if (file_ptr != nullptr) {
		// Don't print if log-file is empty
		fseek(file_ptr, 0, SEEK_END);
		if (ftell(file_ptr) != 0) {
			printf("%c", '\n');
			// Navigate back to beginning of file
			fseek(file_ptr, 0, SEEK_SET);
			printf("%s (%s):\n", logPrefix.c_str(), filePath.c_str());
			c = fgetc(file_ptr);
			while (c != EOF) {
				printf("%c", c);
				c = fgetc(file_ptr);
			}
		}
		fclose(file_ptr);
	}
}

int main(int argc, char** argv) {
    // PLATFORM SPECIFIC DEFAULTS
	#ifdef _WIN64
	// Defaults assume tools were installed on the same drive as Corth as well as in the root directory of the drive.
	Corth::ASMB_PATH = "\\NASM\\nasm.exe";
	Corth::ASMB_OPTS = "-f win64";
	Corth::LINK_PATH = "\\Golink\\golink.exe";
	Corth::LINK_OPTS = "/console /ENTRY:main msvcrt.dll";
    #endif

	#ifdef __linux__
	Corth::ASMB_PATH = "nasm";
	Corth::ASMB_OPTS = "-f elf64";
	Corth::LINK_PATH = "ld";
	Corth::LINK_OPTS = "-dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -m elf_x86_64";
	#endif
	
	if (!Corth::HandleCMDLineArgs(argc, argv)){
		// Non-graceful handling of command line arguments, abort execution.
		return -1;
	}
	
	Corth::Program prog;

	static_assert(static_cast<int>(Corth::MODE::COUNT) == 2, "Exhaustive handling of modes in main method");
	static_assert(static_cast<int>(Corth::PLATFORM::COUNT) == 2, "Exhaustive handling of platforms in main method");
	static_assert(static_cast<int>(Corth::ASM_SYNTAX::COUNT) == 2, "Exhaustive handling of assembly syntaxes in main method");

	// Try to load program source from a file
    try {
        prog.source = loadFromFile(Corth::SOURCE_PATH);
		if (Corth::verbose_logging) { Corth::Log("Load file: successful"); }
    }
    catch (std::runtime_error e) {
		Corth::Error("Could not load source file!", e);
        return -1;
    }
    catch (...) {
		Corth::Error(("Could not load source file at " + Corth::SOURCE_PATH));
        return -1;
    }

	// Lex program source into tokens
	bool lexSuccessful = Corth::Lex(prog);
	if (lexSuccessful) {
		Corth::ValidateTokens(prog);
		if (Corth::verbose_logging) {
			Corth::Log("Lexed file into tokens: successful");
			Corth::PrintTokens(prog);
		}
	}
	else {
		Corth::Error("Failure when lexing file into tokens");
	    return -1;
	}

	if (Corth::RUN_MODE == Corth::MODE::GENERATE) {
		if (Corth::RUN_PLATFORM == Corth::PLATFORM::WIN64) {
            #ifdef _WIN64
			if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::NASM) {
				Corth::GenerateAssembly_NASM_win64(prog);
			}
			else if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::GAS) {
				Corth::GenerateAssembly_GAS_win64(prog);
			}
            #else
			Corth::Error("_WIN64 is undefined; specify the correct platform with a cmd-line flag");
			return -1;
            #endif
		}
		else if (Corth::RUN_PLATFORM == Corth::PLATFORM::LINUX64) {
            #ifdef __linux__
		    if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::NASM) {
				Corth::GenerateAssembly_NASM_linux64(prog);
			}
			else if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::GAS) {
				Corth::GenerateAssembly_GAS_linux64(prog);
			}
            #else
			Corth::Error("__linux__ is undefined. Incorrect platform selected using cmd-line flags?");
			return -1;
            #endif
		}
	}
	else if (Corth::RUN_MODE == Corth::MODE::COMPILE) {
		if (Corth::RUN_PLATFORM == Corth::PLATFORM::WIN64) {
            #ifdef _WIN64
			if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::GAS) {
				Corth::GenerateAssembly_GAS_win64(prog);
				if (FileExists(Corth::ASMB_PATH)) {
					/* Construct Commands
					   Assembly is generated at `Corth::OUTPUT_NAME.s` */
						
					std::string cmd_asmb = Corth::ASMB_PATH + " "
						+ Corth::ASMB_OPTS + " "
						+ Corth::OUTPUT_NAME + ".s "
						+ ">assembler-log.txt 2>&1";
		
					printf("[CMD]: `%s`\n", cmd_asmb.c_str());
					if (system(cmd_asmb.c_str()) == 0) {
						Corth::Log("Assembler successful!");
					}
					else {
						Corth::Log("Assembler returned non-zero exit code, indicating a failure. Check `assembler-log.txt` for the output of the assembler, or enable verbose logging (`-v` flag) to print output to the console.");
					}

					if (Corth::verbose_logging) {
						printCharactersFromFile("assembler-log.txt", "Assembler Log");
					}
				}
				else {
					Corth::Error("Assembler not found at " + Corth::ASMB_PATH + "\n");
					return -1;
				}
			}
			else if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::NASM) {
				Corth::GenerateAssembly_NASM_win64(prog);
				if (FileExists(Corth::ASMB_PATH)) {
					if (FileExists(Corth::LINK_PATH)) {
						/* Construct Commands
						   Assembly is generated at `Corth::OUTPUT_NAME.asm`
						   By default on win64, NASM generates an output `.obj` file of the same name as the input file.
						   This means the linker needs to link to `Corth::OUTPUT_NAME.obj` */
						
						std::string cmd_asmb = Corth::ASMB_PATH + " "
							+ Corth::ASMB_OPTS + " "
							+ Corth::OUTPUT_NAME + ".asm "
							+ ">assembler-log.txt 2>&1";
						
						std::string cmd_link = Corth::LINK_PATH + " "
							+ Corth::LINK_OPTS + " "
							+ Corth::OUTPUT_NAME + ".obj "
							+ ">linker-log.txt 2>&1";
		
						printf("[CMD]: `%s`\n", cmd_asmb.c_str());
						if (system(cmd_asmb.c_str()) == 0) {
							Corth::Log("Assembler successful!");
						}
						else {
							Corth::Log("Assembler returned non-zero exit code, indicating a failure. Check `assembler-log.txt` for the output of the assembler, or enable verbose logging (`-v` flag) to print output to the console.");
						}

						printf("[CMD]: `%s`\n", cmd_link.c_str());
						if (system(cmd_link.c_str()) == 0) {
							Corth::Log("Linker successful!");
						}
						else {
							Corth::Log("Linker returned non-zero exit code, indicating a failure. Check `linker-log.txt` for the output of the linker, or enable verbose logging (`-v` flag) to print output to the console.");
						}

						if (Corth::verbose_logging) {
							printCharactersFromFile("assembler-log.txt", "Assembler Log");
							printCharactersFromFile("linker-log.txt", "Linker Log");
						}
					}
					else {
						Corth::Error("Linker not found at " + Corth::LINK_PATH + "\n");
						return -1;
					}
				}
				else {
					Corth::Error("Assembler not found at " + Corth::ASMB_PATH + "\n");
					return -1;
				}
			}
            #else
			Corth::Error("_WIN64 is undefined; specify the correct platform with a cmd-line option");
			return -1;
            #endif
		}
		else if (Corth::RUN_PLATFORM == Corth::PLATFORM::LINUX64) {
			#ifdef __linux__
			if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::GAS) {
				Corth::GenerateAssembly_GAS_linux64(prog);
				if (!system(("which " + Corth::ASMB_PATH).c_str())) {
					/* Construct Commands
					   Assembly is generated at `<Corth::OUTPUT_NAME>.s` */
					std::string cmd_asmb = Corth::ASMB_PATH + " "
						+ Corth::ASMB_OPTS + " "
						+ Corth::OUTPUT_NAME + ".s "
						+ ">assembler-log.txt 2>&1";
						
					// TODO: Look into exec() family of functions
					printf("[CMD]: `%s`\n", cmd_asmb.c_str());
					if (system(cmd_asmb.c_str()) == 0) {
						Corth::Log("Assembler successful!");
					}
					else {
						Corth::Log("Assembler returned non-zero exit code, indicating a failure. Check `assembler-log.txt` for the output of the assembler, or enable verbose logging (`-v` flag) to print output to the console.");
					}

					if (Corth::verbose_logging) {
						printCharactersFromFile("assembler-log.txt", "Assembler Log");
					}
				}
				else {
					Corth::Error("Assembler not found at " + Corth::ASMB_PATH + "\n");
					return -1;
				}
			}
			else if (Corth::ASSEMBLY_SYNTAX == Corth::ASM_SYNTAX::NASM) {
				Corth::GenerateAssembly_NASM_linux64(prog);
				if (!system(("which " + Corth::ASMB_PATH).c_str())) {
					if (!system(("which " + Corth::LINK_PATH).c_str())) {
						/* Construct Commands
						   Assembly is generated at `<Corth::OUTPUT_NAME>.asm`
						   By default on linux, NASM generates an output `.o` file of the same name as the input file
						   This means the linker needs to link to `Corth::OUTPUT_NAME.o`
						*/
						std::string cmd_asmb = Corth::ASMB_PATH + " "
							+ Corth::ASMB_OPTS + " "
							+ Corth::OUTPUT_NAME + ".asm "
							+ ">assembler-log.txt 2>&1";
						
						std::string cmd_link = Corth::LINK_PATH + " "
							+ Corth::LINK_OPTS + " "
							+ Corth::OUTPUT_NAME + ".o "
							+ ">linker-log.txt 2>&1";

						// TODO: Look into exec() family of functions
						printf("[CMD]: `%s`\n", cmd_asmb.c_str());
						if (system(cmd_asmb.c_str()) == 0) {
							Corth::Log("Assembler successful!");
						}
						else {
							Corth::Log("Assembler returned non-zero exit code, indicating a failure. Check `assembler-log.txt` for the output of the assembler, or enable verbose logging (`-v` flag) to print output to the console.");
						}

						printf("[CMD]: `%s`\n", cmd_link.c_str());
						if (system(cmd_link.c_str()) == 0) {
							Corth::Log("Linker successful!");
						}
						else {
							Corth::Log("Linker returned non-zero exit code, indicating a failure. Check `linker-log.txt` for the output of the linker, or enable verbose logging (`-v` flag) to print output to the console.");
						}

						if (Corth::verbose_logging) {
							printCharactersFromFile("assembler-log.txt", "Assembler Log");
							printCharactersFromFile("linker-log.txt", "Linker Log");
						}
					}
					else {
						Corth::Error("Linker not found at " + Corth::LINK_PATH + "\n");
						return -1;
					}
				}
				else {
					Corth::Error("Assembler not found at " + Corth::ASMB_PATH + "\n");
					return -1;
				}
			}
		    #else
			Corth::Error("__linux__ is undefined. Incorrect platform selected using cmd-line flags?");
			return -1;
            #endif
		}
	}
	return 0;
}
