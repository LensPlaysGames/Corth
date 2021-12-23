#include <iostream>    // printf
#include <stdio.h>     // cout, cin
#include <string>      // string

std::string g_WorkingDirectory; // The directory the program is being run from

namespace Corth {
    void PrintUsage(){
        printf("%s\n", "Usage: `%s Path/To/File.corth`");
    }
}

int main(int argc, char** argv){
	std::string exec = argv[0];
	g_WorkingDirectory = exec.substr(0, exec.find_last_of("/\\"));
	printf("Working Directory: %s\n", g_WorkingDirectory.c_str());

    // Print number of arguments
    printf("Number of args: %i\n", argc);

    // Print arguments
    for (size_t i = 0; i < argc; i++){
        printf("Arg. %zi: %s\n", i, argv[i]);
    }
	std::cout << "\n";

    if (argc < 2){
        printf("Error: %s\n", "Path to file not given!");
		Corth::PrintUsage();
    }
    else if (argc == 2) {
        // Do the thing, I guess
    }
    else if (argc > 2){
        printf("Error: %s\n", "Can not comprehend arguments given.");
		Corth::PrintUsage();
    }
    
    // Wait for character input
    char in;
    std::cin >> in;
    return 0;
}
