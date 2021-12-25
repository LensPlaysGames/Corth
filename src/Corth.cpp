#include <iostream>    // printf
#include <stdio.h>     // cout, cin
#include <fstream>     // ofstream, ifstream (reading/writing files)
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <string>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#else
#endif

namespace Corth {
	bool verbose_logging = false;
	
	void PrintUsage(){
		printf("%s\n", "Usage: `Corth.exe <options> Path/To/File.corth`");
		printf("    %s\n", "Options:");
        printf("        %s\n", "-win, -win64             | Generate assembly for Windows 64-bit. If no platform is specified, this is the default.");
        printf("        %s\n", "-linux, -linux64         | Generate assembly for Linux 64-bit. Requires over-riding assembler and linker paths");
		printf("        %s\n", "-com, --compile          | Compile program from source into executable");
		printf("        %s\n", "-sim, --simulate         | Simulate the program in a virtual machine");
		printf("        %s\n", "-a, --assembler-path     | Specify path to assembler (include .exe)");
        printf("        %s\n", "-l, --linker-path        | Specify path to linker (include .exe)");
		printf("        %s\n", "-ao, --assembler-options | Command line arguments called with assembler");
        printf("        %s\n", "-lo, --linker-options    | Command line arguments called with linker");
		printf("        %s\n", "-v, --verbose            | Enable verbose logging within Corth");
	}

    enum class TokenType {
        WHITESPACE,
        INT,
        OP,
		COUNT
    };

	std::string TokenTypeStr(TokenType& t) {
		assert(static_cast<int>(TokenType::COUNT) == 3);
		if (t == TokenType::WHITESPACE) { return "WHITESPACE"; }
		else if (t == TokenType::INT) { return "INTEGER"; }
		else if (t == TokenType::OP) { return "OPERATOR"; }
		return "ERROR: UNREACHABLE";
	}
	
	struct Token {
	public:
		TokenType type;
		std::string text;
		size_t line_number {1};

		Token(){
			type = TokenType::WHITESPACE;
			text = "";
		}
	};
	
	struct Program {
		std::string source;
		std::vector<Token> tokens;
	};

    // TODO: Make it so a line number could be specified and the execution will halt at that line with a printout of the stack
	void SimulateProgram(Program& prog) {
		printf("\n%s\n\n", "Begin program simulation");

		std::vector<std::string> stack;

		for (auto& tok : prog.tokens) {
			if (tok.type == TokenType::INT) {
				stack.push_back(tok.text);
			}
			else if (tok.type == TokenType::OP) {
				if (tok.text == "+") {
					assert(stack.size() > 1);
					int a = std::stoi(stack.back());
					stack.pop_back();
					int b = std::stoi(stack.back());
					stack.pop_back();
					stack.push_back(std::to_string(a + b));
				}
				else if (tok.text == "-") {
					assert(stack.size() > 1);
					int b = std::stoi(stack.back());
					stack.pop_back();					
					int a = std::stoi(stack.back());
					stack.pop_back();
					stack.push_back(std::to_string(a - b));
				}
				else if (tok.text == "*") {
                    assert(stack.size() > 1);
					int a = std::stoi(stack.back());
					stack.pop_back();
					int b = std::stoi(stack.back());
                    stack.pop_back();
					stack.push_back(std::to_string(a * b));
				}
				else if (tok.text == "/") {
					assert(stack.size() > 1);
					int b = std::stoi(stack.back());
					stack.pop_back();
					int a = std::stoi(stack.back());
					stack.pop_back();
					stack.push_back(std::to_string(a / b));
				}
				else if (tok.text == "#") {
                    assert(stack.size() > 0);
					printf("%s\n", stack.back().c_str());
					stack.pop_back();
				}
			}
		}

		printf("\n%s\n", "End program simulation");
	}

	void GenerateAssembly_NASM_linux64(Program& prog){
		std::string asm_file_path = "corth_program.asm";
		std::fstream asm_file;
		asm_file.open(asm_file_path.c_str(), std::ios::out);
		if (asm_file) {
			printf("%s\n", "Generating NASM elf64 assembly");

            // WRITE HEADER TO ASM FILE
			asm_file << "    ;; CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
					 << "    SECTION .data\n"
					 << "    fmt db '%u', 0x0a, 0\n"
					 << "    SECTION .text\n"
                     << "    ;; DEFINE EXTERNAL C RUNTIME SYMBOLS\n"
					 << "    extern exit\n"
					 << "    extern printf\n"
					 << "\n"
					 << "    global _start\n"
					 << "_start:\n";

            // WRITE TOKENS TO ASM FILE MAIN LABEL		
			assert(static_cast<int>(TokenType::COUNT) == 3); 
    		for (auto& tok : prog.tokens){
    			// Write assembly to opened file based on token type and value
				if (tok.type == TokenType::INT){
					asm_file << "    ;; -- push INT --\n"
							 << "    mov rax, " << tok.text << "\n"
							 << "    push rax\n";
				}
				else if (tok.type == TokenType::OP) {
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
					else if (tok.text == "#") {
						asm_file << "    ;; -- dump --\n"
								 << "    lea rdi, [rel fmt]\n"
								 << "    pop rsi\n"
								 << "    mov rax, 0\n"
								 << "    call printf\n";
					}
				}
    		}
			// WRITE ASM FOOTER FOR GRACEFUL PROGRAM EXIT
			asm_file << "    call exit\n";
			
			asm_file.close();
		}
		else {
			printf("Error: %s\n", "Could not open file for writing. Missing permissions?");
		}
	}

	void GenerateAssembly_NASM_win64(Program& prog){
		// Loop through a lexed program and then generate assembly file from it.

		std::string asm_file_path = "corth_program.asm";
		std::fstream asm_file;
		asm_file.open(asm_file_path.c_str(), std::ios::out);
		if (asm_file) {
    		printf("%s\n", "Generating NASM win64 assembly");
			
			// WRITE HEADER TO ASM FILE
			asm_file << "    ;; CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
					 << "    SECTION .data\n"
					 << "    fmt db '%u', 0x0a, 0\n"
					 << "    SECTION .text\n"
                     << "    ;; DEFINE EXTERNAL WINAPI SYMBOLS (LINK AGAINST KERNEL32.DLL AND USER32.DLL)\n"
					 << "    extern printf\n"
					 << "    extern ExitProcess\n"
					 << "\n"
					 << "    global main\n"
					 << "main:\n";

			// WRITE TOKENS TO ASM FILE MAIN LABEL
			assert(static_cast<int>(TokenType::COUNT) == 3); // Exhaustive handling of implementation of token types
    		for (auto& tok : prog.tokens){
    			// Write assembly to opened file based on token type and value
				if (tok.type == TokenType::INT){
					asm_file << "    ;; -- push INT --\n"
							 << "    mov rax, " << tok.text << "\n"
							 << "    push rax\n";
				}
				else if (tok.type == TokenType::OP) {
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
					else if (tok.text == "#") {
						asm_file << "    ;; -- dump --\n"
								 << "    lea rcx, [rel fmt]\n"
								 << "    pop rdx\n"
								 << "    mov rax, 0\n"
								 << "    call printf\n";
					}
				}
    		}
			// WRITE ASM FOOTER (EXIT GRACEFUL)
			asm_file << "    ;; EXIT PROCESS WITH GRACEFUL EXIT CODE USING WINDOWS API\n"
					 << "    mov rax, 0\n"
					 << "    call ExitProcess";

			asm_file.close();
			printf("NASM win64 assembly generated at %s\n", asm_file_path.c_str());
		}
		else {
			printf("Error: %s\n", "Could not open file for writing. Missing permissions?");
		}
	}

	bool iswhitespace(char& c){
		return c == ' '     // spaces
			|| c == '\t'    // tab
			|| c == '\r'    // crlf
			|| c == '\n';   // newline/linefeed
	}

	bool isoperator(char& c){
		return c == '+'    // addition
			|| c == '-'    // subtraction
			|| c == '*'    // multiplication
			|| c == '/'    // division
			|| c == '#';   // dump (pop + print)
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

			// Skip whitespace
			if (iswhitespace(current)) {
				if (tok.text == "\n") { tok.line_number++; }
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
				while (i < src_end){
					// Look ahead for digit.
					i++;
					current = src[i];
					if (isdigit(current)) { tok.text.append(1, current); }
					else { break; }
				}
				
				i--; // Undo lookahead.
				PushToken(toks, tok);
            }
		}
	}

	void PrintToken(Token& t) {
        printf("TOKEN(%s, %s)\n", TokenTypeStr(t.type).c_str(), t.text.c_str());		
	}

	void PrintTokens(Program& p) {
		printf("%s\n", "TOKENS:");
		for (auto& tok : p.tokens){
			std::cout << "    ";
			PrintToken(tok);
		}
	}
}

bool FileExists(std::string filePath) {
	std::ifstream file(filePath);
	if (file.is_open()) { file.close(); return true; }
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

std::string SOURCE_PATH = "main.corth";
std::string ASMB_PATH = "nasm";
std::string LINK_PATH = "golink";

std::string ASMB_OPTS = " -f win64 corth_program.asm";
std::string LINK_OPTS = " /console /ENTRY:main msvcrt.dll corth_program.obj";

enum class MODE {
	COMPILE,
	SIMULATE,
	COUNT
};
MODE RUN_MODE = MODE::COMPILE;

enum class PLATFORM {
	WIN64,
	LINUX64,
	COUNT
};
PLATFORM RUN_PLATFORM = PLATFORM::WIN64;

int main(int argc, char** argv){
	std::string sourceFilePath;
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			Corth::PrintUsage();
			return 0;
		}
		else if (arg == "-v" || arg == "--verbose"){
			printf("%s\n", "Verbose logging enabled");
			Corth::verbose_logging = true;
		}
		else if (arg == "-a" || arg == "--assembler-path") {
			if (i + 1 < argc) {
				i++;
				ASMB_PATH = argv[i];
			}
			else {
				printf("Error: %s\n", "Expected path to assembler to be specified after `-a`!");
				return -1;
			}
		}
		else if (arg == "-ao" || arg == "--assembler-options"){
			if (i + 1 < argc) {
				i++;
				ASMB_OPTS = argv[i];
			}
			else {
				printf("Error: %s\n", "Expected assembler options to be specified afer `-ao`!");
				return -1;
			}
		}
		else if (arg == "-l" || arg == "--linker-path") {
			if (i + 1 < argc) {
				i++;
				LINK_PATH = argv[i];
			}
			else {
				printf("Error: %s\n", "Expected path to linker to be specified after `-l`!");
				return -1;
			}
		}
		else if (arg == "-lo" || arg == "--linker-options"){
			if (i + 1 < argc) {
				i++;
				LINK_OPTS = argv[i];
			}
			else {
				printf("Error: %s\n", "Expected linker options to be specified after `-lo`!");
				return -1;
			}
		}
		else if (arg == "-win" || arg == "-win64" ) {
			RUN_PLATFORM = PLATFORM::WIN64;
		}
		else if (arg == "-linux" || arg == "-linux64") {
			RUN_PLATFORM = PLATFORM::LINUX64;
		}
		else if (arg == "-win32" || arg == "-m32") {
			printf("Error: %s\n", "32-bit mode is not supported!");
		}
		else if (arg == "-com" || arg == "--compile") {
			RUN_MODE = MODE::COMPILE;
		}
		else if (arg == "-sim" || arg == "--simulate") {
			RUN_MODE = MODE::SIMULATE;
		}
		else {
			sourceFilePath = argv[i];
		}
	}

	if (sourceFilePath.empty()) {
		printf("Error: %s\n", "Expected source file path in command line arguments!");
		Corth::PrintUsage();
		return -1;
	}

	Corth::Program prog;
	bool lexSuccessful = false;

    try {
        prog.source = loadFromFile(sourceFilePath);
        printf("%s\n", "Successfully loaded file.");
        Corth::Lex(prog);
        printf("%s\n", "Lexed file into tokens");
        lexSuccessful = true;
		if (Corth::verbose_logging){
			PrintTokens(prog);
		}
    }
    catch (std::runtime_error e) {
        printf("Error during loading file: %s\n", e.what());
        return -1;
    }
    catch (...) {
        std::cout << "Error during loading file!";
        return -1;
    }

	if (lexSuccessful) {
		assert(static_cast<int>(MODE::COUNT) == 2);
		switch (RUN_MODE) {
		case MODE::SIMULATE:
			SimulateProgram(prog);
			break;
        case MODE::COMPILE:
			assert(static_cast<int>(PLATFORM::COUNT) == 2);
			switch (RUN_PLATFORM) {
			case PLATFORM::WIN64:
				#ifdef _WIN64
				Corth::GenerateAssembly_NASM_win64(prog);
				if (FileExists(ASMB_PATH)) {
					if (FileExists(LINK_PATH)) {
						std::string cmd_asmb = ASMB_PATH + ASMB_OPTS;
						std::string cmd_link = LINK_PATH + LINK_OPTS;
		
						printf("Running: `%s`\n", cmd_asmb.c_str());
						system(cmd_asmb.c_str());
		
						printf("Running: `%s`\n", cmd_link.c_str());
						system(cmd_link.c_str());
		
						printf("%s\n", "Executable built successfully!");
					}
					else {
					    printf("Error: %s\n", ("Linker not found at " + LINK_PATH + "\n").c_str());
						return -1;
					}
				}
				else {
				    printf("Error: %s\n", ("Assembler not found at " + ASMB_PATH + "\n").c_str());
					return -1;
				}
				#endif
				break;
			case PLATFORM::LINUX64:
				#ifdef __linux__
				Corth::GenerateAssembly_NASM_linux64(prog);
				if (!system(("which " + ASMB_PATH).c_str())) {
					if (!system(("which " + LINK_PATH).c_str())) {
						std::string cmd_asmb = ASMB_PATH + ASMB_OPTS;
						std::string cmd_link = LINK_PATH + LINK_OPTS;

						// TODO: Look into exec() family of functions
						printf("[CMD]: `%s`\n", cmd_asmb.c_str());
						system(cmd_asmb.c_str());

						printf("[CMD]: `%s`\n", cmd_link.c_str());
						system(cmd_link.c_str());

						printf("%s\n", "Executable built successfully!");
					}
					else {
						printf("Error: %s\n", ("Linker not found at " + LINK_PATH + "\n").c_str());
						return -1;
					}
				}
				else {
                    printf("Error: %s\n", ("Assembler not found at " + ASMB_PATH + "\n").c_str());
                    return -1;
                }
				#endif
				break;
			}
			
			break;
		}
	}

	return 0;
}
