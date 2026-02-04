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

namespace fs = std::filesystem;
int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  //print_welcome();
  while (true)
  {
    std::cout << "$ ";
    std::string command;

    std::getline(std::cin,command);
    
    std::vector<std::string> all_tokens = split_arguments(command);
    if (all_tokens.empty()) continue;

    std::string redirect_file="";
    bool is_append=false;
    int redirect_fd_type=1;

    std::vector<std::string> filtered_tokens;
    for(size_t i=0;i < all_tokens.size();i++){
      if(all_tokens[i]==">" || all_tokens[i]=="1>"){
        if(i+1<all_tokens.size()){
          redirect_file=all_tokens[i+1];
          is_append=false;
          i++;//跳过名字
        }
      }
      else if(all_tokens[i]==">>"||all_tokens[i]=="1>>"){
          //>> 是增加到尾部
          if(i+1<all_tokens.size()){
            redirect_file=all_tokens[i+1];
            is_append=true;
            i++;
          }
      }
      else if(all_tokens[i]=="2>"){
          //stderr
          if(i+1<all_tokens.size()){
            redirect_file=all_tokens[i+1];
            redirect_fd_type=2;
            is_append=false;
            i++;
          }
        }
      else if(all_tokens[i]=="2>>"){
        //stderr append
        if(i+1<all_tokens.size()){
          redirect_file=all_tokens[i+1];
          redirect_fd_type=2;
          is_append=true;
          i++;
        }
      }
      else{
          filtered_tokens.push_back(all_tokens[i]);
      }
      
    }

    if (filtered_tokens.empty()) continue;
    std::string order = filtered_tokens[0];
  
   
    // 将剩余的 token 存入你之前的 args_list
    std::vector<std::string> args_list;
    for (size_t i = 1; i < filtered_tokens.size(); ++i) {
        args_list.push_back(filtered_tokens[i]); // 这会自动拿到 /tmp/ant/f   11（不带引号）
    }
   
    
   
   
      //带有参数的命令
      // std::string order=command.substr(0,space_pos);
      //std::string arguments = command.substr(space_pos + 1);
        if (order == "exit"){
          break;
        }
        else if (order == "echo") {
            int saved_stdout=-1;
            int target_fd=(redirect_fd_type==2)?STDERR_FILENO : STDOUT_FILENO;
            if(!redirect_file.empty()){
              saved_stdout=dup(target_fd);
              int flags = O_WRONLY | O_CREAT | (is_append ? O_APPEND : O_TRUNC);
              int fd = open(redirect_file.c_str(), flags, 0644);
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
            arguments=="cd"){
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
        }else if(order=="cd"){
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
                  if(!redirect_file.empty()){
                    int flags=O_WRONLY | O_CREAT;
                    if(is_append){
                      flags |=O_APPEND;
                    }else{
                      flags |=O_TRUNC;
                    }
                    int fd=open(redirect_file.c_str(),flags,0644);
                    if(fd<0){
                      perror("open");
                      exit(1);
                    }

                    if(dup2(fd,redirect_fd_type)<0){
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
  
