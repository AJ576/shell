#include<iostream>
#include<unistd.h>
#include<sys/wait.h>
#include<vector>
#include<stdio.h>

std::vector<char*> makeArgs(const std::vector<std::string>& tokens)
{
    std::vector<char*>argv;
    for(auto& s: tokens)
    {
        argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);

    return argv;
}

int main()
{
    std::vector<std::vector<std::string>> commands = {
        {"printf", "c\nb\na\n"},
        {"sort"},
        {"tr", "a-z", "A-Z"},
        {"uniq"}
    };

    int n = commands.size();
    // Create N-1 pipes for N commands
    std::vector<std::array<int,2>> pipes(n - 1);
    for (int i = 0; i < n - 1; i++)
        pipe(pipes[i].data());
    
    // FORK N processes

    for(int i = 0; i < n; i++)
    {
        pid_t pid = fork();

        if(pid == 0)
        {
            // i > 0. means this is a middle pipe so expect input from previous pipe
            if(i> 0)
                dup2(pipes[i-1][0],STDIN_FILENO);
            // i<n-1 means all the middle pipes so forward to the next pipe for the next child
            if(i < n-1)
                dup2(pipes[i][1], STDOUT_FILENO);
            
            //close all file FDs
            for(auto& p : pipes)
            {
                close(p[0]);
                close (p[1]);
            }

            auto argv = makeArgs(commands[i]);
            execvp(argv[0], argv.data());
            perror("execvp");
            _exit(1);
        }
    }

    // parent closes all pipe fds, cuz each process has its own pipes for all of em
    for (auto& p : pipes)
    {
        close(p[0]);
        close(p[1]);
    }

    // wait for all children. we can use wait cuz 1 wait waits for 1 child only so n waits will wait till all n children are done.
    for (int i = 0; i < n; i++)
        wait(nullptr);

}
//this was wrong cuz we were using the same pipe also waiting too early. it was wrong. the children run concurrently not serially
//run the first fork and put output in a pipe.
    // int fd[2];
    // pipe(fd);

    // pid_t p1 = fork();
    // if (p1 == 0)
    // {
    //     dup2(fd[1],STDOUT_FILENO);
    //     close (fd[0]);
    //     close (fd[1]);

    //     std::vector<char*> argv = makeArgs(commandVector[0]);

    //     execvp(argv[0],argv.data());
    //     perror("execvp");

    // }
    // else{
    //     int status;
    //     waitpid(p1, &status, 0); 
    // }

    // for(int i = 1; i < len-1; i ++)
    // {
    //     pid_t pi = fork();
    //     if (pi == 0)
    //     {
    //         dup2(fd[0],STDIN_FILENO);
    //         dup2(fd[1], STDOUT_FILENO);
    //         close(fd[0]);
    //         close(fd[1]);

    //         std::vector<char*> argv = makeArgs(commandVector[i]);
    //         execvp(argv[0],argv.data());
    //         perror("execvp");
    //     }
    // }

    // pid_t pn = fork();
    // if (pn == 0)
    // {
    //     dup2(fd[0], STDIN_FILENO); // stdin <- pipe read
    //     close(fd[1]);
    //     close(fd[0]);


    //     std::vector<char*> argv = makeArgs(commandVector[len-1]);

    //     execvp(argv[0],argv.data());
    //     perror("execvp");

    // }
    // else{
    //     int status;
    //     waitpid(pn, &status, 0); 
    // }