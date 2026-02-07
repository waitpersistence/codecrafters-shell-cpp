#include <cstdio>      // 显式包含 stdio
#include <iostream>
#include <string>
#include "utils.h"
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#include <cstdlib>
#include <fcntl.h>
#include "autocomplete.h"
#include "pipeline.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <algorithm>

Command parse_command(const std::vector<std::string>& tokens){
  
  Command cmd;
  for(size_t i=0;i < tokens.size();i++){
        if(tokens[i]==">" || tokens[i]=="1>"){
          if(i+1<tokens.size()){
            cmd.redirect_file=tokens[i+1];
            cmd.is_append=false;
            i++;//跳过名字
          }
        }
        else if(tokens[i]==">>"||tokens[i]=="1>>"){
            //>> 是增加到尾部
            if(i+1<tokens.size()){
              cmd.redirect_file=tokens[i+1];
              cmd.is_append=true;
              i++;
            }
        }
        else if(tokens[i]=="2>"){
            //stderr
            if(i+1<tokens.size()){
              cmd.redirect_file=tokens[i+1];
              cmd.redirect_fd_type=2;
              cmd.is_append=false;
              i++;
            }
          }
        else if(tokens[i]=="2>>"){
          //stderr append
          if(i+1<tokens.size()){
            cmd.redirect_file=tokens[i+1];
            cmd.redirect_fd_type=2;
            cmd.is_append=true;
            i++;
          }
        }
        else{
            cmd.args.push_back(tokens[i]);
        }
        
      }
      return cmd;
}
std::vector<Command> parse_pipeline(const std::vector<std::string>& all_tokens) {
    std::vector<Command> pipeline;
    std::vector<std::string> current_cmd_tokens;

    for (const auto& token : all_tokens) {
        if (token == "|") {
            // 遇到管道符，说明前一个命令结束了
            if (!current_cmd_tokens.empty()) {
                pipeline.push_back(parse_command(current_cmd_tokens));
                current_cmd_tokens.clear();
            }
        } else {
            current_cmd_tokens.push_back(token);
        }
    }

    // 处理最后一个管道符之后的命令
    if (!current_cmd_tokens.empty()) {
        pipeline.push_back(parse_command(current_cmd_tokens));
    }

    return pipeline;
}
void display_history_list(int limit) {
    HIST_ENTRY **hist_list = history_list();
    if (!hist_list) return;

    int total = 0;
    while (hist_list[total]) total++;

    int start = 0;
    // 如果 limit 为正且小于 total，则计算起始点
    if (limit > 0 && limit < total) {
        start = total - limit;
    }

    for (int i = start; i < total; i++) {
        // 使用 printf 方便对齐格式
        printf("%5d  %s\n", i + history_base, hist_list[i]->line);
    }
}
namespace fs = std::filesystem;
void load_history_from_env() {
    const char* histfile = std::getenv("HISTFILE");
    if (histfile) {
        // 加载历史文件到内存
        read_history(histfile);
    }
}
void append_history_on_exit() {
    const char* histfile = std::getenv("HISTFILE");
    if (histfile) {
        // history_length 是 readline 维护的全局变量，代表内存中条数
        append_history(history_length, histfile);
    }
}
// 工具函数：处理路径中的 ~
std::string expand_path(std::string path) {
    if (!path.empty() && path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) path.replace(0, 1, home);
    }
    return path;
}

void save_history_to_file() {
    const char* histfile = std::getenv("HISTFILE");
    if (histfile) {
        // write_history 会将内存中所有的条目覆盖写入到指定文件
        if (write_history(histfile) != 0) {
            // 可选：如果写入失败，输出错误信息
            // std::cerr << "Error: Could not save history to " << histfile << std::endl;
        }
    }
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  //print_welcome();
  // 记录启动时或上一次同步后，历史记录的数量
  int history_count_at_sync = 0;
  initialize_readline();
  load_history_from_env();
  while (true)
  {
    // 1. 用 C 指针接住 Readline 的返回值
    char* temp_ptr = readline("$ ");

    // 2. 检查是否遇到了 Ctrl+D (EOF)
    if (temp_ptr == nullptr) {
        save_history_to_file(); // 退出前保存
        std::cout << "\nExit" << std::endl;
        break;
    }
    
    std::string command = temp_ptr;

    
    if (!command.empty()) {
        
         add_history(temp_ptr); // 历史记录依然建议传原始指针
        // 这里开始用你的 std::string command 做事
        if (command == "exit") {
            save_history_to_file(); // 退出前保存
            free(temp_ptr); // 别忘了最后一次释放
            break;
        }
       
       
    }
    free(temp_ptr);
    

    std::vector<std::string> tokens = split_arguments(command);
    if (tokens.empty()) continue;
    // 2. 解析成管道链条
    std::vector<Command> pipeline = parse_pipeline(tokens);

    if (pipeline.empty()) continue;
    if (pipeline.size() != 1) {
        
        execute_multi_pipeline(pipeline);
        continue; // 管道处理完，进入下一轮循环
    }
    Command single_cmd = parse_command(tokens);

   
   
    std::string order = single_cmd.args[0];
  
   
    // 将剩余的 token 存入你之前的 args_list
    std::vector<std::string> args_list;
    for (size_t i = 1; i < single_cmd.args.size(); ++i) {
        args_list.push_back(single_cmd.args[i]); // 这会自动拿到 /tmp/ant/f   11（不带引号）
    }
   

   
   
      //带有参数的命令
      // std::string order=command.substr(0,space_pos);
      //std::string arguments = command.substr(space_pos + 1);
        if (order == "exit"){
          break;
        }
        else if (order == "echo") {
            int saved_stdout=-1;
            int target_fd=(single_cmd.redirect_fd_type==2)?STDERR_FILENO : STDOUT_FILENO;
            if(!single_cmd.redirect_file.empty()){
              saved_stdout=dup(target_fd);
              int flags = O_WRONLY | O_CREAT | (single_cmd.is_append ? O_APPEND : O_TRUNC);
              int fd = open(single_cmd.redirect_file.c_str(), flags, 0644);
              dup2(fd, target_fd);
              close(fd);
            }
            for(size_t i=0;i<args_list.size();i++){
              std::cout<<args_list[i];
              if(i<args_list.size()-1){
                std::cout<<" ";
              }
            }
            std::cout<<std::endl;
            // 恢复现场
            if (saved_stdout != -1) {
                dup2(saved_stdout, target_fd);
                close(saved_stdout);
            }
        }else if(order =="type"){
            if(args_list.empty()){
              continue;
            }
            std::string arguments = args_list[0]; // 获取要查询的命令名
          if(arguments=="echo"||arguments=="exit"||arguments=="type"||arguments=="pwd"||
            arguments=="cd"|| arguments=="history"){
            std::cout<<arguments<<" is a shell builtin"<<std::endl;
          }
          else{
            std::string path=get_path_of_command(arguments);
            if(!path.empty()){
              std::cout<<arguments<<" is "<<path<<std::endl;
            }else{
            std::cout << arguments << ": not found" << std::endl;
            }
          }          
        
        }
       else if (order == "history") {
    // 1. 处理带参数的情况 (args_list 不为空)
    if (!args_list.empty()) {
        std::string first_arg = args_list[0];

        // 处理 -r (读取) 或 -w (写入)
        if (first_arg == "-r" || first_arg == "-w") {
            if (args_list.size() < 2) {
                std::cerr << "history: " << first_arg << ": filename argument required" << std::endl;
            } else {
                std::string filepath = args_list[1];
                
                // 处理 ~ 展开
                if (!filepath.empty() && filepath[0] == '~') {
                    const char* home = std::getenv("HOME");
                    if (home) filepath.replace(0, 1, home);
                }

                if (first_arg == "-r") {
                    if (read_history(filepath.c_str()) != 0) {
                        std::cerr << "history: " << filepath << ": No such file or directory" << std::endl;
                    }
                } else {
                    // 执行写入操作
                    if (write_history(filepath.c_str()) != 0) {
                        std::cerr << "history: " << filepath << ": Cannot write history to file" << std::endl;
                    }
                }
            }
        } 
        // 在 history 命令的处理逻辑中增加：
      else if (first_arg == "-a") {
          std::string filepath = expand_path(args_list[1]);

        // 计算自上次同步以来新增的条目数量
        // history_length 是 readline 库提供的全局变量
        int new_entries = history_length - history_count_at_sync;

        if (new_entries > 0) {
            // append_history 只会将最后 n 条记录追加到文件末尾
            if (append_history(new_entries, filepath.c_str()) != 0) {
                std::cerr << "history: " << filepath << ": Cannot append history" << std::endl;
            } else {
                // 同步成功后，更新计数，防止下次 -a 重复追加
                history_count_at_sync = history_length;
            }
        }
      }
        // 2. 处理数字参数 history <n>
        else {
            try {
                int n = std::stoi(first_arg);
                display_history_list(n); // 建议提取成辅助函数
            } catch (...) {
                std::cerr << "history: " << first_arg << ": numeric argument or -r/-w required" << std::endl;
            }
        }
    } 
    // 3. 处理不带参数的情况：打印全部
    else {
        display_history_list(-1); 
    }
}
        else if(order=="cd"){
          std::string arguments = args_list[0];
          std::string path=get_path_of_command(order);
          std::stringstream ss(arguments);
          std::string temp;
          while(ss>>temp){
                args_list.push_back(temp);
          }
          std::string target_path = args_list[0];
          if(!target_path.empty() && target_path[0] == '~'){
            const char* home_env = std::getenv("HOME");
            if (home_env) {
                std::string home_dir = home_env;
                target_path.replace(0, 1, home_dir);
                fs::current_path(target_path);
            }
          }else {
            std::string target_path=args_list[0];
            try{
              fs::current_path(target_path);
            }catch(const fs::filesystem_error& e){
              // 如果目录不存在或权限不足，打印错误
              std::cout << "cd: " << target_path << ": No such file or directory" << std::endl;
            }
          }
        }
        else if(order== "pwd"){
        std::cout << std::filesystem::current_path().string() << std::endl;
          }
        else{
          //外部命令
          std::string path;
          // 1. 优先检查 order 是否直接指向一个存在的文件（处理本地带空格的可执行文件）
          if (fs::exists(order) && !fs::is_directory(order)) {
              path = order;
          } 
          // 2. 如果本地找不到，再去 PATH 环境变量里搜
          else {
              path = get_path_of_command(order);
          }

         // std::cerr << "[DEBUG] Opening file: " << redirect_file << std::endl;
          if(!path.empty()){
                pid_t pid=fork();
                if(pid==0){
                  //子进程切换执行
                  if(!single_cmd.redirect_file.empty()){
                    int flags=O_WRONLY | O_CREAT;
                    if(single_cmd.is_append){
                      flags |=O_APPEND;
                    }else{
                      flags |=O_TRUNC;
                    }
                    int fd=open(single_cmd.redirect_file.c_str(),flags,0644);
                    if(fd<0){
                      perror("open");
                      exit(1);
                    }

                    if(dup2(fd,single_cmd.redirect_fd_type)<0){
                      perror("dup2");
                      exit(1);
                    }
                    close(fd);//关闭对之前打开文件的控制，没影响，因为此时 标准输出1连接这个文件
                  }
                  std::vector<char*> exec_args;
                  exec_args.push_back(const_cast<char*>(order.c_str()));
                  for(int i=0;i<args_list.size();i++){
                    //std::cout<<args_list[i]<<std::endl;
                    exec_args.push_back(const_cast<char*>(args_list[i].c_str()));
                  }
                  exec_args.push_back(nullptr);
                
                  execv(path.c_str(), exec_args.data());
                  exit(1);
                }
                else if(pid>0){
                wait(NULL);
                }else{
                std::cerr<<"Fork failed!"<<std::endl;
                }
            }
          else {
            std::cout << order << ": command not found" << std::endl;
          }

        }
      
    }
    return 0;
  }
  
