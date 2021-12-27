#include <iostream>    // printf
#include <stdio.h>     // cout, cin
#include <fstream>     // ofstream, ifstream (reading/writing files)
#include <algorithm>   // remove_if
#include <stdlib.h>
#include <assert.h>    // I use assert to exit the program in the case it encounters an error.
#include <string>
#include <sstream>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#else
#endif

/* TODO: Convert all auto-ranged for loops over program Tokens into iterator based loops (to be able to skip around for branching and loops within Corth). */

namespace Corth {
	// This needs to be changed if operators are added or removed from Corth internally.
	size_t OP_COUNT = 6;
	bool isoperator(char& c){
		return c == '+'    // addition
			|| c == '-'    // subtraction
			|| c == '*'    // multiplication
			|| c == '/'    // division
			|| c == '='    // boolean comparison (equal)
			|| c == '#';   // dump (pop + print)
	}

	// This needs to be changed if keywords are added or removed from Corth internally.
	size_t KEYWORD_COUNT = 2;
	std::vector<std::string> keywords {
		"if",
		"endif"
	};
	
	// Corth state (it may be a simple state machine, but it still is one :D)
	// TODO: Add `-o` flag to allow output name to be changed.
	std::string SOURCE_PATH = "main.corth";
	std::string OUTPUT_NAME = "corth_program";
	std::string ASMB_PATH = "";
	std::string LINK_PATH = "";
	std::string ASMB_OPTS = "";
	std::string LINK_OPTS = "";

	enum class MODE {
		COMPILE,
		SIMULATE,
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
	bool verbose_logging = false;

    enum class TokenType {
        WHITESPACE,
        INT,
        OP,
		KEYWORD,
		COUNT
    };

	std::string TokenTypeStr(TokenType& t) {
		assert(static_cast<int>(TokenType::COUNT) == 4);
		if (t == TokenType::WHITESPACE)   { return "WHITESPACE"; }
		else if (t == TokenType::INT)     { return "INTEGER"; }
		else if (t == TokenType::OP)      { return "OPERATOR"; }
		else if (t == TokenType::KEYWORD) { return "KEYWORD"; }
		return "ERROR: UNREACHABLE";
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

	void PrintUsage(){
		printf("\n%s\n", "Usage: `Corth.exe <flags> <options> Path/To/File.corth`");
		printf("    %s\n", "Flags:");
        printf("        %s\n", "-win, -win64             | Generate assembly for Windows 64-bit. If no platform is specified, this is the default.");
		printf("        %s\n", "-linux, -linux64         | Generate assembly for Linux 64-bit.");
		//printf("        %s\n", "-mac, -apple             | Generate assembly for MacOS 64-bit.");
        printf("        %s\n", "-com, --compile          | Compile program from source into executable");
		printf("        %s\n", "-sim, --simulate         | Simulate the program in a virtual machine");
		printf("        %s\n", "-gen, --generate         | Generate assembly, but don't create an executable from it.");
		printf("    %s\n", "Options (latest over-rides):");
		printf("        %s\n", "Usage: <option> <input>");
		printf("        %s\n", "If the <input> contains spaces, be sure to surround it by double quotes.");
		printf("        %s\n", "-a, --assembler-path     | Specify path to assembler (include .exe)");
        printf("        %s\n", "-l, --linker-path        | Specify path to linker (include .exe)");
		printf("        %s\n", "-ao, --assembler-options | Command line arguments called with assembler");
        printf("        %s\n", "-lo, --linker-options    | Command line arguments called with linker");
		printf("        %s\n", "-v, --verbose            | Enable verbose logging within Corth");
	}

	void DoLog(std::string msg, std::string prefix = "[LOG]", std::string suffix = "\n") {
		printf((prefix + ": %s" + suffix).c_str(), msg.c_str());
	}

	void DoLog(std::string msg, size_t line_num, std::string prefix = "[LOG]", std::string suffix = "\n") {
		printf((prefix + " LINE %zu: %s" + suffix).c_str(), line_num, msg.c_str());
	}

	void DoLog(std::string msg, size_t line_num, size_t column_num, std::string prefix = "[LOG]", std::string suffix = "\n") {
		printf((prefix + " LINE %zu, COL %zu: %s" + suffix).c_str(), line_num, column_num, msg.c_str());
	}

	void Error(std::string msg) {
		DoLog(msg, "\n[ERR]");
	}

	void Error(std::string msg, size_t line_num) {
		DoLog(msg, line_num, "\n[ERR]");
	}

	void Error(std::string msg, size_t line_num, size_t column_num) {
		DoLog(msg, line_num, column_num, "\n[ERR]");
	}

	void Error(std::string msg, std::exception e) {
		DoLog(msg + " (" + e.what() + ")", "\n[ERR]");
	}

	void StackError(){
		Error("Stack protection invoked! (Did you forget to put the operator after the operands (i.e. `5 5 +` not `5 + 5`))?");
	}

    void StackError(size_t line_num) {
        Error("Stack protection invoked! (Did you forget to put the operator after the operands (i.e. `5 5 +` not `5 + 5`))?", line_num);
    }

    void StackError(size_t line_num, size_t column_num) {
        Error("Stack protection invoked! (Did you forget to put the operator after the operands (i.e. `5 5 +` not `5 + 5`))?", line_num, column_num);
    }

    void Warning(std::string msg) {
		DoLog(msg, "[WRN]");
    }

    void Warning(std::string msg, size_t line_num) {
		DoLog(msg, line_num, "[WRN]");
    }

    void Warning(std::string msg, size_t line_num, size_t column_num) {
		DoLog(msg, line_num, column_num, "[WRN]");
    }

    // TODO: Convert a bunch of `printf`s into a bunch of `Log`s

    void DbgLog(std::string msg) {
		DoLog(msg, "[DBG]");
    }

    void DbgLog(std::string msg, size_t line_num) {
		DoLog(msg, line_num, "[DBG]");
    }

    void DbgLog(std::string msg, size_t line_num, size_t column_num) {
		DoLog(msg, line_num, column_num, "[DBG]");
    }

    void Log(std::string msg) {
        DoLog(msg);
    }

	void Log(std::string msg, size_t line_num) {
        DoLog(msg, line_num);
    }

	void Log(std::string msg, size_t line_num, size_t column_num) {
		DoLog(msg, line_num, column_num);
    }

    // TODO: Make it so a line number could be specified and the execution will halt at that line with a printout of the stack
    void SimulateProgram(Program& prog) {
		DoLog("Begin program simulation", "\n[SIM]", "\n\n");
        std::vector<std::string> stack;

        size_t instr_ptr = 0;
        size_t instr_ptr_max = prog.tokens.size();
        while (instr_ptr < instr_ptr_max) {
            Token& tok = prog.tokens[instr_ptr];
            if (tok.type == TokenType::INT) {
                stack.push_back(tok.text);
            }
            else if (tok.type == TokenType::OP) {
                if (OP_COUNT == 6) {
                    if (tok.text == "+") {
                        if (stack.size() > 1) {
                            int a = std::stoi(stack.back());
                            stack.pop_back();
                            int b = std::stoi(stack.back());
                            stack.pop_back();
                            stack.push_back(std::to_string(a + b));
                        }
                        else {
                            StackError(tok.line_number, tok.col_number);
                            assert(stack.size() > 1);
                        }
                    }
                    else if (tok.text == "-") {
                        if(stack.size() > 1) {
                            int b = std::stoi(stack.back());
                            stack.pop_back();                   
                            int a = std::stoi(stack.back());
                            stack.pop_back();
                            stack.push_back(std::to_string(a - b));
                        }
                        else {
                            StackError(tok.line_number, tok.col_number);
                            assert(stack.size() > 1);
                        }
                    }
                    else if (tok.text == "*") {
                        if (stack.size() > 1) {
                            int a = std::stoi(stack.back());
                            stack.pop_back();
                            int b = std::stoi(stack.back());
                            stack.pop_back();
                            stack.push_back(std::to_string(a * b));
                        }
                        else {
                            StackError(tok.line_number, tok.col_number);
                            assert(stack.size() > 1);
                        }
                    }
                    else if (tok.text == "/") {
                        if (stack.size() > 1) {
                            int b = std::stoi(stack.back());
                            stack.pop_back();
                            int a = std::stoi(stack.back());
                            stack.pop_back();
                            stack.push_back(std::to_string(a / b));
                        }
                        else {
                            StackError(tok.line_number, tok.col_number);
                            assert(stack.size() > 1);
                        }
                    }
                    else if (tok.text == "=") {
                        if (stack.size() > 1) {
                            int a = std::stoi(stack.back());
                            stack.pop_back();
                            int b = std::stoi(stack.back());
                            stack.pop_back();
                            stack.push_back(std::to_string(a == b));
                        }
                    }
                    else if (tok.text == "#") {
                        if (stack.size() > 0) {
                            printf("%s\n", stack.back().c_str());
                            stack.pop_back();
                        }
                        else {
                            StackError(tok.line_number, tok.col_number);
                            assert(stack.size() > 0);
                        }
                    }
                }
                else {
                    // Exhaustive handling of operator count
                    Error("Exhaustive handling of operator count in SimulateProgram()", tok.line_number);
                    assert(OP_COUNT == 6);
                }
            }
            instr_ptr++;
        }

        DoLog("End program simulation", "\n[SIM]", "\n\n");
	}

	// TODO: Convert GenerateAssembly return type from void to bool
	void GenerateAssembly_NASM_linux64(Program& prog){
		std::string asm_file_path = OUTPUT_NAME + ".asm";
		std::fstream asm_file;
		asm_file.open(asm_file_path.c_str(), std::ios::out);
		if (asm_file) {
			printf("%s\n", "Generating NASM elf64 assembly");

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
			if (static_cast<int>(TokenType::COUNT) == 4) {
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
					else if (tok.type == TokenType::OP) {
						if (OP_COUNT == 6) {
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
							else if (tok.text == "#") {
								asm_file << "    ;; -- dump --\n"
										 << "    lea rdi, [rel fmt]\n"
										 << "    pop rsi\n"
										 << "    mov rax, 0\n"
										 << "    call printf\n";
							}
						}
						else {
							Error("Exhaustive handling of operator count in GenerateAssembly_NASM_linux64()", tok.line_number, tok.col_number);
							assert(OP_COUNT == 6);
						}
					}
					else if (tok.type == TokenType::KEYWORD) {
						if (KEYWORD_COUNT == 2) {
							if (tok.text == "if") {
							    asm_file << "    ;; -- if --\n"
										 << "    pop rax\n"
										 << "    cmp rax, 0\n"
										 << "    je addr_" << tok.data << "\n";
							}
							else if (tok.text == "endif") {
								asm_file << "    ;; -- endif --\n"
										 << "addr_" << instr_ptr << "\n";
							}
						}
						else {
							Error("Exhaustive handling of keyword count in GenerateAssembly_NASM_linux64()", tok.line_number, tok.col_number);
						}
					}
					instr_ptr++;
				}
				// WRITE ASM FOOTER (GRACEFUL PROGRAM EXIT, CONSTANTS)
				asm_file << "    mov rdi, 0\n"
						 << "    call exit\n"
						 << "\n\n"
						 << "    SECTION .data\n"
						 << "    fmt db '%u', 0x0a, 0\n";
			
				asm_file.close();
			}
			else {
				Error("Exhaustive handling of TokenType count in GenerateAssembly_NASM_linux64()");
				assert(static_cast<int>(TokenType::COUNT) == 4); 
			}
		}
		else {
			Error("Could not open file for writing. Missing permissions?");
		}
	}

	void GenerateAssembly_NASM_mac64(Program& prog) {
		std::string asm_file_path = OUTPUT_NAME + ".asm";
		std::fstream asm_file;
		asm_file.open(asm_file_path.c_str(), std::ios::out);
		if (asm_file) {
			printf("%s\n", "Generating NASM mac64 assembly");

			/* MacOS assembly
			    I know it sounds scary, but it's really not too bad.
				It supports NASM, and can be linked using gcc which comes with XCode, the built in IDE.
			    The calling convention is familiar (System V AMD64 ABI), so that's a win.
				The weird thing is all external symbols must be prefaced by an `_`. Why are you like this, Apple?
			 */

			// WRITE HEADER
			asm_file << "    ;; CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
					 << "    ;; USING `SYSTEM V AMD64 ABI` CALLING CONVENTION (RDI, RSI, RDX, RCX, R8, R9, -> STACK)\n";



			// WRITE FOOTER (GRACEFUL EXIT AND CONSTANTS)
			asm_file << "    ;; -- TODO: graceful exit --\n"
					 << "\n\n"
					 << "    SECTION .data\n"
					 << "    fmt db '%u', 0x0a, 0\n";
		}
        else {
			Error("Could not open file for writing. Missing permissions?");
		}
	}

	void GenerateAssembly_NASM_win64(Program& prog) {
		// Loop through a lexed program and then generate assembly file from it.
		std::string asm_file_path = OUTPUT_NAME + ".asm";
		std::fstream asm_file;
		asm_file.open(asm_file_path.c_str(), std::ios::out);
		if (asm_file) {
    		printf("%s\n", "Generating NASM win64 assembly");
			
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
			if (static_cast<int>(TokenType::COUNT) == 4) {
				size_t instr_ptr = 0;
				size_t instr_ptr_max = prog.tokens.size();
				while (instr_ptr < instr_ptr_max) {
					Token& tok = prog.tokens[instr_ptr];
					if (tok.type == TokenType::INT){
						asm_file << "    ;; -- push INT --\n"
								 << "    mov rax, " << tok.text << "\n"
								 << "    push rax\n";
					}
					else if (tok.type == TokenType::OP) {
						if (OP_COUNT == 6) {
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
							else if (tok.text == "#") {
								asm_file << "    ;; -- dump --\n"
										 << "    lea rcx, [rel fmt]\n"
										 << "    pop rdx\n"
										 << "    mov rax, 0\n"
										 << "    call printf\n";
							}
						}
						else {
							Error("Exhaustive handling of operator count in GenerateAssembly_NASM_win64()");
							assert(OP_COUNT == 6);
						}
					}
					else if (tok.type == TokenType::KEYWORD) {
						if (KEYWORD_COUNT == 2) {
							if (tok.text == "if") {
								// Need to pop from stack, compare, and jump to end if it is zero
								// This means end needs to generate a UNIQUE label to jump to
								// Get conditional value from stack, if false jump to end label
								asm_file << "    ;; -- if --\n"
										 << "    pop rax\n"
										 << "    cmp rax, 0\n"
										 << "    je addr_" << tok.data << "\n";
									
							}
							else if (tok.text == "endif") {
								asm_file << "    ;; -- endif --\n"
										 << "addr_" << instr_ptr << ":\n";
							}
						}
						else {
							Error("Exhaustive handling of keyword count in GenerateAssembly_NASM_win64()");
							assert(KEYWORD_COUNT == 2);
						}
					}
					instr_ptr++;
				}
				// EXIT GRACEFUL
				asm_file << "    mov rcx, 0\n"
						 << "    call exit\n"
						 << "\n\n"
						 << "    SECTION .data\n"
						 << "    fmt db '%u', 0x0a, 0\n";

				asm_file.close();
				printf("NASM win64 assembly generated at %s\n", asm_file_path.c_str());
			}
			else {
				Error("Exhaustive handling of TokenType count in GenerateAssembly_NASM_win64()");
				assert(static_cast<int>(TokenType::COUNT) == 4); // Exhaustive handling of implementation of token types
			}
		}
		else {
			Error("Could not open file for writing. Missing permissions?");
		}
	}

	bool HandleCMDLineArgs(int argc, char** argv) {
		// Return value = whether execution will halt or not in main function
		assert(static_cast<int>(MODE::COUNT) == 3);
		assert(static_cast<int>(PLATFORM::COUNT) == 2);
		for (int i = 1; i < argc; i++) {
			std::string arg = argv[i];
			if (arg == "-h" || arg == "--help") {
				return false;
			}
			else if (arg == "-v" || arg == "--verbose"){
				printf("%s\n", "Verbose logging enabled");
				verbose_logging = true;
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
			else if (arg == "-ao" || arg == "--assembler-options"){
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
			else if (arg == "-lo" || arg == "--linker-options"){
				if (i + 1 < argc) {
					i++;
					LINK_OPTS = argv[i];
				}
				else {
					Error("Expected linker options to be specified after `-lo`!");
					return false;
				}
			}
			else if (arg == "-win" || arg == "-win64" ) {
				RUN_PLATFORM = PLATFORM::WIN64;
			}
			else if (arg == "-linux" || arg == "-linux64") {
				RUN_PLATFORM = PLATFORM::LINUX64;
			}
			else if (arg == "-win32" || arg == "-m32" || arg == "-linux32") {
				Error("32-bit mode is not supported!");
				return false;
			}
			else if (arg == "-com" || arg == "--compile") {
				RUN_MODE = MODE::COMPILE;
			}
			else if (arg == "-sim" || arg == "--simulate") {
				RUN_MODE = MODE::SIMULATE;
			}
			else if (arg == "-gen" || arg == "--generate") {
				RUN_MODE = MODE::GENERATE;
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

	bool iswhitespace(char& c){
		return c == ' '     // spaces
			|| c == '\t'    // tab
			|| c == '\r'    // crlf
			|| c == '\n';   // newline/linefeed
	}

	void PushToken(std::vector<Token>& tokList, Token& tok){
		// Add token to program if it is not whitespace
		if (tok.type != TokenType::WHITESPACE) {
			tokList.push_back(tok);
		}
		// Reset token
		tok.type = TokenType::WHITESPACE;
		tok.text.erase();
	}

	// Convert program source into tokens
    void Lex(Program& prog){
		std::string src = prog.source;
		size_t src_end = src.size();

		std::vector<Token>& toks = prog.tokens;
		Token tok;
		char current = src[0];

		for(int i = 0; i < src_end; i++){
			current = src[i];
			
			tok.col_number++;

			// Skip whitespace
			if (iswhitespace(current)) {
				if (current == '\n') {
					tok.line_number++;
					tok.col_number = 1;
				}
			}
			else if (isoperator(current)){
				tok.type = TokenType::OP;
				tok.text.append(1, current);
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
					if (isdigit(current)) { tok.text.append(1, current); }
					else { break; }
				}
				i--; // Undo lookahead.
				PushToken(toks, tok);
            }
			else if (isalpha(current)) {
				tok.type = TokenType::KEYWORD;
			    tok.text.append(1, current);
				// Handle multiple-alpha keywords
				while (i < src_end) {
					// Look ahead for alpha
					i++;
					current = src[i];
					if (isalpha(current)) {tok.text.append(1, current); }
					else { break; }
				}
				i--; // Undo lookahead.
				PushToken(toks, tok);
            }
		}
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
		for (auto& tok : p.tokens){
			std::cout << "    ";
			PrintToken(tok);
		}
	}

    bool RemovableToken(Token& tok) {
		if (tok.type == TokenType::WHITESPACE) { return false; }
		return true;
	}

	// This function simulates the program running on its most basic level: adding and removing from the stack.
	// A read-access violation can occur if trying to pop off the stack without pushing something on first.
	// To prevent this violation during runtime of Corth, this function catches over-popping before it can happen.
	void ValidateTokens_Stack(Program& prog) {
		std::vector<Token>& toks = prog.tokens;
		// Amount of things on virtual stack
		size_t stackSize = 0;
		if (static_cast<int>(TokenType::COUNT) == 4) {
			for (auto& tok : toks) {
				if (tok.type == TokenType::WHITESPACE) {
					Warning("Validator: Whitespace tokens should not appear in final program. Problem with the Lexing?", tok.line_number, tok.col_number);
				}
				else if (tok.type == TokenType::INT) {
					stackSize++;
				}
				else if (tok.type == TokenType::OP) {
					if (OP_COUNT == 6) {
						// Operators that pop two values off the stack and return one to it
						if (tok.text == "+"
							|| tok.text == "-"
							|| tok.text == "*"
							|| tok.text == "/"
							|| tok.text == "=" )
						{
							if (stackSize > 1) {
								stackSize--; // Two values removed, result added, net loss of one
							}
							else {
								tok.type = TokenType::WHITESPACE;
								StackError(tok.line_number, tok.col_number);
							}
						}
						// Operators that pop one value off the stack and return zero to it
						else if (tok.text == "#") {
							if (stackSize > 0) {
								stackSize--;
							}
							else {
								// This token could cause serious memory issues (by popping a value off the stack that doesn't exist)
								// It is marked for removal by setting it's type to whitespace.
								tok.type = TokenType::WHITESPACE;
								StackError(tok.line_number, tok.col_number);
							}
						}
					}
					else {
						Error("Exhaustive handling of operator count in ValidateTokens()");
						assert(OP_COUNT == 6);
					}
				}
				else if (tok.type == TokenType::KEYWORD) {
					if (KEYWORD_COUNT == 2) {
						if (tok.text == "if") {
							// if will pop from the stack to check the condition to see if it needs to jump or not
							if (stackSize > 0) {
								stackSize--;
							}
							else {
								// This token could cause serious memory issues (by popping a value off the stack that doesn't exist)
								// It is marked for removal by setting it's type to whitespace.
								tok.type = TokenType::WHITESPACE;
								StackError(tok.line_number, tok.col_number);
							}
						}
						else if (tok.text == "endif") {
							continue;
						}
					}
				}
			}
		
			if (stackSize != 0) {
				Warning("Validator: Best practices indicate stack should be empty at end of program.\nStack Size at End of Program: " + std::to_string(stackSize));
			}
		}
		else {
			Error("Exhaustive handling of TokenType count in ValidateTokens_Stack()");
			assert(static_cast<int>(TokenType::COUNT) == 4);
		}
	}

	// This function ensures any tokens that start or stop blocks are correctly refernced
	// For example, an `if` statement needs to know where to jump to if it is false.
	// Another example: `endwhile` statement needs to know where to jump back to.
	void ValidateTokens_Blocks(Program& prog) {
		// This seems like too much nesting, but I can't seem to wrap my head around a different way of doing it.
		// If you would, kindly make a pull request fixing this stupidity and I will gladly accept it and merge it if it works :)
		if (KEYWORD_COUNT == 2) {
			size_t instr_ptr = 0;
			size_t instr_ptr_max = prog.tokens.size();
			while (instr_ptr < instr_ptr_max) {
                if (prog.tokens[instr_ptr].text == "if") {
					size_t if_instr_ptr = instr_ptr;
					bool found_endif = false;
					// Loop through the rest of the tokens until an endif is found.
					// If one isn't found, error.
					// If one is, give if token a reference to this instruction pointer in it's `data` field
                    while (instr_ptr < instr_ptr_max) {
						if (prog.tokens[instr_ptr].type == TokenType::KEYWORD && prog.tokens[instr_ptr].text == "endif") {
							// Success! Cross reference `if` block with `endif`
							if (verbose_logging) { Log("Block `if` successfully cross-referenced with `endif`", prog.tokens[if_instr_ptr].line_number, prog.tokens[if_instr_ptr].col_number); }
							prog.tokens[if_instr_ptr].data = std::to_string(instr_ptr);
							found_endif = true;
							break;
						}
						instr_ptr++;
					}

					if (!found_endif) {
						// Did not find endif block before end of tokens
						Error("Validator: `if` keyword missing `endif` block-ending-symbol", prog.tokens[if_instr_ptr].line_number, prog.tokens[if_instr_ptr].col_number);
						assert(found_endif);
					}
                }
				instr_ptr++;
			}
		}
		else {
			Error("Exhaustive handling of keyword count in ValidateTokens_Blocks()");
			assert(KEYWORD_COUNT == 2);
		}
	}
	
	void ValidateTokens(Program& prog) {
		// Stack protection
		ValidateTokens_Stack(prog);

		// Cross-reference blocks (give `if` tokens a reference to it's `endif` counterpart
		ValidateTokens_Blocks(prog);

		// Remove all un-neccessary tokens
		std::remove_if(prog.tokens.begin(), prog.tokens.end(), RemovableToken);

		if (verbose_logging) { Log("Tokens validated"); }
	}
}

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
	std::string tmp;
	std::string path_var_str(buf[0], buf_sz);
	std::stringstream path_var_ss(path_var_str);
	while (std::getline(path_var_ss, tmp, ';')) {
		if (tmp.back() != '\\' || tmp.back() != '/') {
			tmp.append(1, '/');
		}
	   	path_var.push_back(tmp);
    }

    // Check windows PATH system variable
	for (auto& path : path_var) {
		std::string test(path + filePath);
		if (Corth::verbose_logging) { printf("Testing %s\n", test.c_str()); }
		std::ifstream f(test);
		if (f.is_open()) { f.close(); return true; }
	}
	#else
	Corth::Error("Something went very, very wrong...\nIf you are on Linux may god have mercy on your soul.");
	return false;
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
	    Corth::PrintUsage();
		return -1;
	}
	
	Corth::Program prog;
	bool lexSuccessful = false;

    try {
        prog.source = loadFromFile(Corth::SOURCE_PATH);
				
        printf("%s\n", "Successfully loaded file.");
        Corth::Lex(prog);
		Corth::ValidateTokens(prog);
        printf("%s\n", "Lexed file into tokens");
        lexSuccessful = true;
		if (Corth::verbose_logging){
			Corth::PrintTokens(prog);
		}
    }
    catch (std::runtime_error e) {
		Corth::Error("Could not load source file!", e);
		Corth::PrintUsage();
        return -1;
    }
    catch (...) {
		Corth::Error(("Could not load source file at " + Corth::SOURCE_PATH));
		Corth::PrintUsage();
        return -1;
    }

	if (lexSuccessful) {
		assert(static_cast<int>(Corth::MODE::COUNT) == 3);
		switch (Corth::RUN_MODE) {
		case Corth::MODE::SIMULATE:
			SimulateProgram(prog);
			break;
		case Corth::MODE::GENERATE:
			assert(static_cast<int>(Corth::PLATFORM::COUNT) == 2);
			switch (Corth::RUN_PLATFORM) {
			case Corth::PLATFORM::WIN64:
				#ifdef _WIN64
				Corth::GenerateAssembly_NASM_win64(prog);
				#else
				Corth::Error("_WIN64 is undefined; specify the correct platform with a cmd-line option");
				Corth::PrintUsage();
				return -1;
				#endif
				break;
			case Corth::PLATFORM::LINUX64:
				#ifdef __linux__
				Corth::GenerateAssembly_NASM_linux64(prog);
				#else
				Corth::Error("__linux__ is undefined. Incorrect platform selected using cmd-line flags?");
				Corth::PrintUsage();
				return -1;
				#endif
				break;
			}
			break;
        case Corth::MODE::COMPILE:
			assert(static_cast<int>(Corth::PLATFORM::COUNT) == 2);
			switch (Corth::RUN_PLATFORM) {
			case Corth::PLATFORM::WIN64:
				#ifdef _WIN64
				Corth::GenerateAssembly_NASM_win64(prog);
				if (FileExists(Corth::ASMB_PATH)) {
					if (FileExists(Corth::LINK_PATH)) {
						/* Construct Commands
						    Assembly is generated at `Corth::OUTPUT_NAME.asm`
					        By default on win64, NASM generates an output `.obj` file of the same name as the input file.
					        This means the linker needs to link to `Corth::OUTPUT_NAME.obj` */
						std::string cmd_asmb = Corth::ASMB_PATH + " " + Corth::ASMB_OPTS + " " + Corth::OUTPUT_NAME + ".asm";
						std::string cmd_link = Corth::LINK_PATH + " " + Corth::LINK_OPTS + " " + Corth::OUTPUT_NAME + ".obj";
		
						printf("[CMD]: `%s`\n", cmd_asmb.c_str());
						system(cmd_asmb.c_str());
		
						printf("[CMD]: `%s`\n", cmd_link.c_str());
						system(cmd_link.c_str());
		
						printf("%s\n", "Executable built successfully!");
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
				#else
				Corth::Error("_WIN64 is undefined; specify the correct platform with a cmd-line option");
				Corth::PrintUsage();
				return -1;
				#endif
				break;
			case Corth::PLATFORM::LINUX64:
				#ifdef __linux__
				Corth::GenerateAssembly_NASM_linux64(prog);
				if (!system(("which " + Corth::ASMB_PATH).c_str())) {
					if (!system(("which " + Corth::LINK_PATH).c_str())) {
						/* Construct Commands
						    Assembly is generated at `Corth::OUTPUT_NAME.asm`
							By default on linux, NASM generates an output `.o` file of the same name as the input file
							This means the linker needs to link to `Corth::OUTPUT_NAME.o`
						 */
						std::string cmd_asmb = Corth::ASMB_PATH + " " + Corth::ASMB_OPTS + " " + Corth::OUTPUT_NAME + ".asm";
						std::string cmd_link = Corth::LINK_PATH + " " + Corth::LINK_OPTS + " " + Corth::OUTPUT_NAME + ".o";

						// TODO: Look into exec() family of functions
						printf("[CMD]: `%s`\n", cmd_asmb.c_str());
						system(cmd_asmb.c_str());

						printf("[CMD]: `%s`\n", cmd_link.c_str());
						system(cmd_link.c_str());

						printf("%s\n", "Executable built successfully!");
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
				#else
				Corth::Error("__linux__ is undefined. Incorrect platform selected using cmd-line flags?");
				Corth::PrintUsage();
				return -1;
				#endif
				break;
			}
			
			break;
		}
	}

	return 0;
}
