#include "pipeline.h"
#include<iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string>
#include "utils.h"
void execute_command(const Command& cmd) {
    
    if (cmd.args.empty()) return;
    // 1. 处理重定向
    if (!cmd.redirect_file.empty()) {
        int flags = O_WRONLY | O_CREAT | (cmd.is_append ? O_APPEND : O_TRUNC);
        int fd = open(cmd.redirect_file.c_str(), flags, 0644);
        if (fd < 0) {
            perror("open");
            return;
        }
        dup2(fd, cmd.redirect_fd_type);
        close(fd);
    }
    std::string order = cmd.args[0];
    if (order == "exit") {
        exit(0); // 子进程退出
    } else if (order == "type") {
        // ... 执行 type 的逻辑 (建议也封装成函数) ..
          if(cmd.args[1]=="echo"||cmd.args[1]=="exit"||cmd.args[1]=="type"||cmd.args[1]=="pwd"||
            cmd.args[1]=="cd"){
            std::cout<<cmd.args[1]<<" is a shell builtin"<<std::endl;
          }
          exit(0);
        // 注意：这里要用 std::cout 输出，它会自动流向管道

    } else if (order == "echo") {
        // ... 执行 echo 的逻辑 ...
       for (size_t i = 1; i < cmd.args.size(); ++i) {
            std::cout << cmd.args[i] << (i == cmd.args.size() - 1 ? "" : " ");
        }
        std::cout << std::endl;
        exit(0);
    } else {
        // 如果不是内建命令，才调用 execvp
        std::string path = get_path_of_command(order);
        char** argv = cmd.to_argv(); // 使用你在 Command 结构体里定义的转换函数
        execv(path.c_str(), argv);
        perror("execv");
        exit(1);
    }
}

int execute_pipeline(const Command& cmd1, const Command& cmd2){
    int pipefd[2];
    pid_t pid1,pid2;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    // --- 创建第一个子进程执行 cmd1 ---
    pid1 = fork();
    if (pid1 == 0) {
        // 子进程1：将标准输出(stdout)重定向到管道写端
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); // 关闭不需要的读端
        close(pipefd[1]); // 已经dup2了，原始fd可以关闭
        execute_command(cmd1); // 关键：如果 cmd1 是 type，它会在这里直接运行并输出到管道
        perror("execvp cmd1"); // 如果execvp返回，说明出错了
        exit(EXIT_FAILURE);
    }

    // --- 创建第二个子进程执行 cmd2 ---
    pid2 = fork();
    if (pid2 == 0) {
        // 子进程2：将标准输入(stdin)重定向到管道读端
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]); // 关闭不需要的写端
        close(pipefd[0]);
        
        execute_command(cmd2); // 关键：如果 cmd1 是 type，它会在这里直接运行并输出到管道
        perror("execvp cmd2");
        exit(EXIT_FAILURE);
    }
    // --- 父进程逻辑 ---
    // 关键！父进程必须关闭管道的两端，否则子进程2永远读不到 EOF
    close(pipefd[0]);
    close(pipefd[1]);

    // 等待两个子进程结束
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
void execute_multi_pipeline(const std::vector<Command>& commands){
    int num_cmds=commands.size();
    int pipefd[2];
    int prev_read_end=-1;

    std::vector<pid_t>pids;
    for(int i=0;i<num_cmds;i++){
        if(i<num_cmds-1){
            //如果不是最后一个命令，创建一个新管道
            if(pipe(pipefd)==-1){
                perror("pipe");
                return;
            }
        }
        pid_t pid = fork();
            if(pid==0){
                if(i>0){
                    dup2(prev_read_end, STDIN_FILENO);
                    close(prev_read_end);
                }
                // B. 处理输出：如果不是最后一个命令，往当前管道写
                if (i < num_cmds - 1) {
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[0]); // 子进程不需要当前管道的读端
                    close(pipefd[1]);
                }

            // 执行命令逻辑（调用你之前的封装函数）
                execute_command(commands[i]);
                exit(0);
            }
            // --- 父进程逻辑 ---
            pids.push_back(pid);

            // 关闭不再需要的旧读端
            if (i > 0) {
                close(prev_read_end);
            }
            // 准备下一轮循环：记录当前管道的读端，并关闭写端
            if (i < num_cmds - 1) {
                prev_read_end = pipefd[0]; 
                close(pipefd[1]); // 关键：父进程必须关掉写端，否则下游命令会卡死
            }
        }
        // 等待所有子进程结束
        for (pid_t p : pids) {
            waitpid(p, NULL, 0);
        }
}
