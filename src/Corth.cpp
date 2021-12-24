#include <iostream>    // printf
#include <stdio.h>     // cout, cin
#include <fstream>     // ofstream, ifstream (reading/writing files)
#include <assert.h>
#include <string>
#include <vector>

namespace Corth {
	void PrintUsage(){
		printf("%s\n", "Usage: `Corth.exe <options> Path/To/File.corth`");
		printf("    %s\n", "Options:");
		printf("        %s\n", "-a, --nasm-path      | *optional Specify path to NASM");
        printf("        %s\n", "-a, --golink-path    | *optional Specify path to GoLink");
	}

    enum class TokenType {
        WHITESPACE,
        INT,
        OP,
		COUNT
    };

	std::string TokenTypeStr(TokenType t){
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

	void GenerateAssembly_NASM_win64(Program prog){
		// Loop through a lexed program and then generate assembly file from it.
		
		std::fstream asm_file;
		asm_file.open("corth_program.asm", std::ios::out);
		if (asm_file) {
    		printf("%s\n", "Generating NASM win64 assembly");


			// WRITE HEADER TO ASM FILE
			/* MISSING .bss SECTION FOR STDIN STDOUT HANDLES AND .data SECTION FOR STDIN/OUT QUERY CODES */
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

			asm_file << "    ;; EXIT PROCESS WITH GRACEFUL EXIT CODE USING WINDOWS API\n"
					 << "    mov rax, 0\n"
					 << "    call ExitProcess";

			asm_file.close();
			printf("%s\n", "NASM win64 assembly generated at corth_program.obj");
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

// Load a file into a string from a path.
std::string loadFromFile(std::string filePath) {
    std::ifstream inFileStream(filePath);
    if (!inFileStream) {
		throw std::exception(("File not found at " + filePath).c_str());
	}
    return std::string(std::istreambuf_iterator<char>(inFileStream), std::istreambuf_iterator<char>());
}

std::string NASM_PATH = "nasm";
std::string GOLINK_PATH = "golink";

int main(int argc, char** argv){
    // Print arguments
    for (size_t i = 0; i < argc; i++){
        printf("Arg. %zi: %s\n", i, argv[i]);
    }
	std::cout << "\n";
	
	std::string sourceFilePath;
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			Corth::PrintUsage();
			return 0;
		}
		else if (arg == "-a" || arg == "--nasm-path") {
			if (i + 1 < argc) {
				i++;
				NASM_PATH = argv[i];
			}
			else {
				printf("Error: %s\n", "Expected NASM path to be specified after `-a`!");
				return -1;
			}
		}
		else if (arg == "-l" || arg == "--golink-path") {
			if (i + 1 < argc) {
				i++;
				GOLINK_PATH = argv[i];
			}
			else {
				printf("Error: %s\n", "Expected GoLink path to be specified after `-l`!");
				return -1;
			}
		}
		else {
			sourceFilePath = argv[i];
		}
	}

	Corth::Program prog;
	bool lexSuccessful = false;

    try {
        prog.source = loadFromFile(sourceFilePath);
        printf("%s\n", "Successfully loaded file.");
        Corth::Lex(prog);
        printf("%s\n", "Lexed file into tokens");
        lexSuccessful = true;
        PrintTokens(prog);
    }
    catch (std::exception e) {
        printf("Error during loading file: %s\n", e.what());
        return -1;
    }
    catch (...) {
        std::cout << "Error during loading file!";
        return -1;
    }

	if (lexSuccessful) {
		Corth::GenerateAssembly_NASM_win64(prog);

		std::string cmd_asmb = NASM_PATH + " -f win64 corth_program.asm";
		std::string cmd_link = GOLINK_PATH + " /console /ENTRY:main kernel32.dll user32.dll msvcrt.dll corth_program.obj";
		
		printf("Running: `%s`\n", cmd_asmb.c_str());
		system(cmd_asmb.c_str());
		printf("Running: `%s`\n", cmd_link.c_str());
		system(cmd_link.c_str());
		printf("%s\n", "Executable built successfully!");
	}
}
