#include<iostream>
#include<string>
#include<sys/wait.h>
#include<unistd.h>
#include<vector>
#include<sstream>

std::string renderPrompt(const std::string& homePath);
std::vector<std::string> tokenizePrompt(const std::string& command);
bool handle_builtin(const std::vector<std::string>& tokens, const std::string& homePath);
void run_external(const std::vector<std::string>& tokens);

int main(){
    // this sets the homepath. it can be used for a lot of things in a real shell
    // for now we use it to format the paths while the program runs and a direct way to get 
    // back to the initial path by just typing cd, which is a shell feature.
    char buffer [FILENAME_MAX];
    std::string homePath;
    if(getcwd(buffer, sizeof(buffer)) != NULL){
        homePath = buffer;
    }
    else{
        perror("getcwd() error");
        return 1;
    }
    
    while (true)
    {
        std::cout<<renderPrompt(homePath);

        std::string command;
        std:: getline(std::cin, command);

        std::vector<std::string> tokens = tokenizePrompt(command);

        if (not (handle_builtin(tokens,homePath)))
        {
            run_external(tokens);
        }

        std::cout<<"\n";
    }
    return 0;
}



//pre: takes in a const refernce for the homepath
std::string renderPrompt(const std::string& homePath)
{
    //make the buffer that can hold the path. gotta do this c style for some reason
    char buffer [FILENAME_MAX];
    std::string shell = "shell>";
    std::string path;
    //we are not doing the perror thing here cuz seperation of concerns is a good practice
    // a function that computes the string should not be doing error stuff, especially in a shell
    // program as it will pollute the shell. also getcwd rarely fails
    if(getcwd(buffer, sizeof(buffer)) == NULL){
        return shell;
    } 

    //turn the buffer into a string for comparisons and use homepath for formatting
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
    return path+"\n"+shell;
}
//post: returns the prompt.

//pre: takes in a const reference of a command
std::vector<std::string> tokenizePrompt(const std::string& command)
{
    //this one is pretty simple I think?
    //takes in the string command and basically each ss is a word from the line command.
    // ss >> token puts one word fo the line into the token to be pushed back.
    // this is basically some real epic shit for c++
    std::stringstream ss(command);
    std::string token;
    std::vector<std::string> tokens;
    while(ss >> token)
    {
        tokens.push_back(token);
    }

    return tokens;

}
//post: returns a string vector of tokens fromt the command

//pre takes in the tokens and the homePath(mostly for the cd to be able to return to home easily)
bool handle_builtin(const std::vector<std::string>& tokens, const std::string& homePath)
{
    if (tokens[0] == "cd")
    {
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
        return true;
    }
    if (tokens[0] == "exit")
    {
        exit(0);
    }
    return false;
}
//post: returns true if the command was handled by the function, i.e. it was builtin, false otherwise

//pre takes in const reference of tokens for the command
void run_external(const std::vector<std::string>& tokens)
{
    pid_t pid = fork();
    if( pid < 0 )
    {
        perror("fork failed");
        return;
    }
    else if(pid == 0)
    {
        if (tokens.empty()) return;

        std::vector<char*>argv;
        for(auto& s: tokens)
        {
            argv.push_back(const_cast<char*>(s.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0],argv.data());
        perror("execvp");
    }
    else
    {
        int status;
        waitpid(pid, &status, 0); 
    }
}
//post has succesfully run the child process with the command
