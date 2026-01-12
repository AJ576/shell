#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <sstream>

std::string redner_prompt(const std::string &homePath);
std::vector<std::string> tokenize_prompt(const std::string &command);
bool handle_builtin(const std::vector<std::string> &tokens, const std::string &homePath);
void run_external(const std::vector<std::string> &tokens);
std::vector<char *> make_args(const std::vector<std::string> &tokens);
bool check_has_pipe(const std::vector<std::string> &tokens);
std::vector<std::vector<std::string>> parse_pipeline(const std::vector<std::string> &tokens);
void run_pipeline(const std::vector<std::vector<std::string>> &pipeline);

int main()
{
    // this sets the homepath. it can be used for a lot of things in a real shell
    // for now we use it to format the paths while the program runs and a direct way to get
    // back to the initial path by just typing cd, which is a shell feature.
    char buffer[FILENAME_MAX];
    std::string homePath;
    if (getcwd(buffer, sizeof(buffer)) != NULL)
    {
        homePath = buffer;
    }
    else
    {
        perror("getcwd() error");
        return 1;
    }

    while (true)
    {
        std::cout << redner_prompt(homePath);

        std::string command;
        std::getline(std::cin, command);

        std::vector<std::string> tokens = tokenize_prompt(command);

        bool has_pipe = check_has_pipe(tokens);

        // ugly ahh piece of policy check
        if (has_pipe && tokens[0] == "cd")
        {
            std::cerr << "cd: cannot be piped\n";
            return 1;
        }

        if (has_pipe)
        {
            // parse pipeline
            auto pipeline = parse_pipeline(tokens);

            // execture pipeline
            run_pipeline(pipeline);
        }
        else if (not(handle_builtin(tokens, homePath)))
        {
            run_external(tokens);
        }

        std::cout << "\n";
    }
    return 0;
}

// pre: takes in a const refernce for the homepath
std::string redner_prompt(const std::string &homePath)
{
    // make the buffer that can hold the path. gotta do this c style for some reason
    char buffer[FILENAME_MAX];
    std::string shell = "shell>";
    std::string path;
    // we are not doing the perror thing here cuz seperation of concerns is a good practice
    //  a function that computes the string should not be doing error stuff, especially in a shell
    //  program as it will pollute the shell. also getcwd rarely fails
    if (getcwd(buffer, sizeof(buffer)) == NULL)
    {
        return shell;
    }

    // turn the buffer into a string for comparisons and use homepath for formatting
    std::string cwd(buffer);
    if (cwd == homePath)
    {
        path = "~";
    }
    else if (cwd.find(homePath) == 0)
    {
        cwd.erase(0, homePath.size());
        path = "~" + cwd;
    }
    else
    {
        path = std::string(buffer);
    }
    return path + "\n" + shell;
}
// post: returns the prompt.

// pre: takes in a const reference of a command
std::vector<std::string> tokenize_prompt(const std::string &command)
{
    // this one is pretty simple I think?
    // takes in the string command and basically each ss is a word from the line command.
    //  ss >> token puts one word fo the line into the token to be pushed back.
    //  this is basically some real epic shit for c++
    std::stringstream ss(command);
    std::string token;
    std::vector<std::string> tokens;
    while (ss >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}
// post: returns a string vector of tokens fromt the command

// pre takes in the tokens and the homePath(mostly for the cd to be able to return to home easily)
bool handle_builtin(const std::vector<std::string> &tokens, const std::string &homePath)
{
    if (tokens.empty())
    {
        return true; // nothing to do
    }

    if (tokens[0] == "cd")
    {
        if (tokens.size() == 1)
        {
            if (chdir(homePath.c_str()) != 0)
            {
                perror(("cd: " + homePath).c_str());
            }
        }
        else
        {
            std::string path;
            for (size_t i = 1; i < tokens.size(); i++)
            {
                path += tokens[i];
            }
            if (chdir(path.c_str()) != 0)
            {
                perror(("cd: " + path).c_str());
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
// post: returns true if the command was handled by the function, i.e. it was builtin, false otherwise

// pre takes in const reference of tokens for the command
void run_external(const std::vector<std::string> &tokens)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        return;
    }
    else if (pid == 0)
    {
        if (tokens.empty())
            _exit(0); // always do this in child if not using exec

        std::vector<char *> argv = make_args(tokens);
        execvp(argv[0], argv.data());
        perror("execvp");
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
}
// post has succesfully run the child process with the command

// pre accepts list of tokens that contain the command as a vector of strings
std::vector<char *> make_args(const std::vector<std::string> &tokens)
{
    std::vector<char *> argv;
    for (auto &s : tokens)
    {
        argv.push_back(const_cast<char *>(s.c_str()));
    }
    argv.push_back(nullptr);

    return argv;
}

// post: returns a vector of const char* type tokens which can be fed into the C posix api

//pre: takes in a vector of tokens of the command
bool check_has_pipe(const std::vector<std::string> &tokens)
{
    for (const auto &t : tokens)
    {
        if (t == "|")
        {
            return true;
        }
    }
    return false;
}
//post: returns a bool value whether the command had the pipe symbol which can be used to execute the pipeline path in main

//pre: takes in tokens
std::vector<std::vector<std::string>> parse_pipeline (const std::vector<std::string> &tokens)
{

    std::vector<std::vector<std::string>> pipeline;
    std::vector<std::string> command;

    for(const auto &t: tokens){

        if(t == "|")
        {
            pipeline.push_back(command);
            command.clear();
        }
        else
        {
            command.push_back(t);
        }
    }
    pipeline.push_back(command);
    return pipeline;

}
//post: returns pipeline which is basically all commands of the pipe command as seperate vector of vector of strings so that each command is seperate.
//looks like {{tokenized command 1}, {tokenized command 2}...{tokenize command n}}

//pre: takes in pipeline which is a vector of vector of each command of the pipeline
//looks like {{tokenized command 1}, {tokenized command 2}...{tokenize command n}}
void run_pipeline(const std::vector<std::vector<std::string>> &pipeline)
{
    
    int n = pipeline.size();

    //create n-1 pipes for n commands
    std::vector<std::array<int, 2>> pipes(n-1);
    for(int i = 0; i < n-1; i++)
    {
        pipe(pipes[i].data());
    }

    //fork and run n commands
    for(int i = 0; i < n; i++)
    {
        pid_t pid = fork();

        if(pid == 0)
        {
            //if i > 1 meaning these are middle command and the end command they will take input from pipe[n-1]
            if(i>0)
            {
                dup2(pipes[i-1][0],STDIN_FILENO);
            }
            // if i < n-1 then these are either the first command or the middle commands so they put outputs in a pipe
            if(i<n-1)
            {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            //now we close all the pipes for THIS specific child
            for(auto &p: pipes)
            {
                close(p[0]);
                close(p[1]);
            }

            //now we execute shit like real DAWGS
            auto argv = make_args(pipeline[i]);
            execvp(argv[0],argv.data());
            perror("execvp");
            _exit(1);
        }
    }

    //now its time for the parent to close all the pipes to make sure EOF happens so that we dont get hung
    //by random commands that dont work until they get EOF. what that means. who knows.
    //remember each process clones all the pipes and stuff INCLUDING THE PARENT
    for(auto &p: pipes)
    {
        close(p[0]);
        close(p[1]);
    }

    //and now we wait.
    //here we will use the wait(nullptr) cuz it allows us to run each child concurently instead of serially, which is needed
    //for piping to work. Hence above we didnt write else for each if(pid == 0) cuz in case of parent it just starts the second
    // child and the third and so on all while the first child is still running and here after all is done it comes to wait
    for(int i = 0; i<n;i++)
    {
        wait(nullptr);
    }
}
//post: runs the pipeline succesfully piping output of one into input of the other till all execution is completed.