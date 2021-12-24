#include <iostream>    // printf
#include <stdio.h>     // cout, cin
#include <fstream>     // ofstream, ifstream (reading/writing files)
#include <assert.h>
#include <string>
#include <vector>

std::string g_WorkingDirectory; // The directory the program is being run from

namespace Corth {
	void PrintUsage(){ printf("%s\n", "Usage: `Corth.exe Path/To/File.corth`"); }

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
    		printf("%s\n", "Generating assembly");


			// WRITE HEADER TO ASM FILE
			/* MISSING .bss SECTION FOR STDIN STDOUT HANDLES AND .data SECTION FOR STDIN/OUT QUERY CODES */
			asm_file << "    ;; CORTH COMPILER GENERATED THIS ASSEMBLY -- (BY LENSOR RADII)\n"
					 << "\n"
					 << "    ;; DEFINE EXTERNAL WINAPI SYMBOLS (LINK AGAINST KERNEL32.DLL AND USER32.DLL)\n"
					 << "    extern GetStdHandle\n"
					 << "    extern WriteFile\n"
					 << "    extern ExitProcess\n"
					 << "\n"
					 << "    SECTION .text\n"
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
    		}

			asm_file << "    ;; EXIT PROCESS WITH GRACEFUL EXIT CODE USING WINDOWS API\n"
					 << "    mov rax, 0\n"
					 << "    call ExitProcess";

			asm_file.close();
			printf("%s\n", "Assembly generated");
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
		printf("Attempting to push TOKEN(%s, %s)\n", TokenTypeStr(tok.type).c_str(), tok.text.c_str());
		// Add token to program if it is not whitespace
		if (tok.type != TokenType::WHITESPACE) {
			std::cout << "Pushing token!\n";
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
}

std::string RemovePathFromFileName(std::string str){
    return str.substr(str.find_last_of("/\\"), str.size());
}

std::string RemoveFileNameFromPath(std::string str){
    return str.substr(0, str.find_last_of("/\\"));
}

void PrintTokens(Corth::Program& p) {
	printf("%s\n", "TOKENS:");
	for (auto& tok : p.tokens){
		printf("    TOKEN(%s, %s)\n", Corth::TokenTypeStr(tok.type).c_str(), tok.text.c_str());
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

int main(int argc, char** argv){
	g_WorkingDirectory = RemoveFileNameFromPath(argv[0]);
	printf("Working Directory: %s\n", g_WorkingDirectory.c_str());

	int EXIT_CODE = 0;

    // Print arguments
    for (size_t i = 0; i < argc; i++){
        printf("Arg. %zi: %s\n", i, argv[i]);
    }
	std::cout << "\n";

	Corth::Program prog;
	bool lexSuccessful = false;

	// This is okay for dealing with arguments right now, but I would like to tokenize arguments if possible.
    if (argc < 2){
        printf("Error: %s\n", "Path to file not given!");
		Corth::PrintUsage();
		EXIT_CODE = -1;
    }
    else if (argc == 2 && (argv[1] != "-h" || argv[1] != "--help")) {
        // Try to compile a program from given file path
		std::string relativeFilePath = argv[1];
		try {
		    prog.source = loadFromFile(relativeFilePath);
			printf("%s\n", "Successfully loaded file.");
			Corth::Lex(prog);
			printf("%s\n", "Lexed file into tokens");
			lexSuccessful = true;
			PrintTokens(prog);
		}
		catch (std::exception e) {
			printf("Error during loading file: %s\n", e.what());
			EXIT_CODE = -1;
		}
		catch (...) {
			std::cout << "Error during loading file!";
			EXIT_CODE = -1;
		}
    }
    else if (argc > 2){
		// Eventually deal with things like custom paths to NASM and/or GoLink, as well as specifying type of assembly to generate
        printf("Error: %s\n", "Can not comprehend arguments given.");
		Corth::PrintUsage();
		EXIT_CODE = -1;
    }

	if (lexSuccessful) {
		Corth::GenerateAssembly_NASM_win64(prog);
    }
	
    // Wait for character input
	printf("\n%s", "Waiting to halt execution. Press 'Return' or 'Enter' to exit.");
	std::cin.get();
    return EXIT_CODE;
}
