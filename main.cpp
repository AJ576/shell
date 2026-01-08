#include<iostream>
#include<string>
#include<sys/wait.h>
#include<unistd.h>
#include<vector>
#include<sstream>

int main(){

    std::cout<<"This is a shell \n";
    std::string command;
    while (true)
    {
        std::cout<<"shell>";
        std:: getline(std::cin, command);

        if (command == "exit")
        {
            break;
        }

        std::stringstream ss(command);
        std::string token;
        std::vector<std::string> tokens;

        while(ss >> token)
        {
            tokens.push_back(token);
        }

        if (tokens[0] == "cd"){

            std::string path;
            for(size_t i = 1; i<tokens.size();i++)
            {
                path+=tokens[i];
            }
            chdir(path.c_str());
            continue;
        }
        

        
        pid_t pid = fork();
        if( pid < 0 )
        {
            std::cerr<<"Fork failed\n";
            continue;
        }
        else if(pid == 0)
        {
            std::vector<char*>argv;

            for(auto& s: tokens)
            {
                argv.push_back(const_cast<char*>(s.c_str()));
            }
            argv.push_back(nullptr);
            execvp(argv[0],argv.data());
        }
        else
        {
            int status;
            waitpid(pid, &status, 0); 
            std::cout<<"executed \n";  
        }

     

    }
    return 0;
}
