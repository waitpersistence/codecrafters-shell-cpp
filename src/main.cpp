#include <iostream>
#include <string>
#include "utils.h"
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <filesystem>
#include <cstdlib>


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
    std::vector<std::string> args_list;
    
    std::getline(std::cin,command);
    if (command == "exit"){
      break;
    }
    size_t space_pos=command.find(" ");
    if(space_pos!=std::string::npos){
      //带有参数的命令
      std::string order=command.substr(0,space_pos);
      std::string arguments = command.substr(space_pos + 1);
      
        if (order == "echo") {
            std::cout << arguments << std::endl;
        }else if(order =="type"){
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
        else{
        std::string path=get_path_of_command(order);
        std::stringstream ss(arguments);
        std::string temp;
         if(!path.empty()){
              pid_t pid=fork();
              if(pid==0){
                while(ss>>temp){
                  args_list.push_back(temp);
                }
                std::vector<char*> exec_args;
                exec_args.push_back(const_cast<char*>(order.c_str()));
                for(int i=0;i<args_list.size();i++){
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

    }
  }else{
    //单独命令
      if (command == "echo"){
        std::cout << "" << std::endl; // echo 空内容只打印一个换行
      }
      else if(command== "pwd"){
        std::cout << std::filesystem::current_path().string() << std::endl;
      }
      else {
            std::cout << command << ": command not found" << std::endl;
      }
    }
  
  }
  return 0;
}
