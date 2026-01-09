#include<iostream>
#include<string>
#include<sys/wait.h>
#include<unistd.h>
#include<vector>
#include<sstream>


int main(){
    char buffer [FILENAME_MAX];
    std::string homePath;
    if(getcwd(buffer, sizeof(buffer)) != NULL){
        homePath = buffer;
    }
    else{
        perror("getcwd() error");
        return 1;
    }
    //std::cout<<"This is a shell \n";
    std::string command;
    while (true)
    {
        // std::cout<<"shell>";
        std::string shell = "shell";
        std::string path;
        if(getcwd(buffer, sizeof(buffer)) != NULL){
            std::string cwd(buffer);
            if (cwd == homePath){
                path = "~";
            } 
            else if(cwd.find(homePath) == 0)
            {
                cwd.erase(0,homePath.size());
                path = "~"+cwd;
            }
            else
            {
                path = std::string(buffer);
            }
        }
        
        else
        {
            perror("getcwd() error");
            return 1;
        }
        std::cout<<path<<"\n"<<"shell>";
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
            if (tokens.size() == 1)
            {
                if(chdir(homePath.c_str()) != 0)
                {
                    perror(("cd: "+ homePath).c_str());
                }
            }
            else{
                std::string path;
                for(size_t i = 1; i<tokens.size();i++)
                {  
                    path+=tokens[i];
                }
                if(chdir(path.c_str()) != 0)
                {
                    perror(("cd: "+ path).c_str());
                }
            }
            
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
        }

    }
    return 0;
}
