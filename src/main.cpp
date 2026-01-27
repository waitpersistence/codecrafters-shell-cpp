#include <iostream>
#include <string>
#include "utils.h"
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
    if (command == "exit"){
      break;
    }
    size_t space_pos=command.find(" ");
    if(space_pos!=std::string::npos){
      std::string order=command.substr(0,space_pos);
      std::string arguments = command.substr(space_pos + 1);
      // 逻辑判断
        if (order == "echo") {
            std::cout << arguments << std::endl;
        }else if(order =="type"){
          if(arguments=="echo"||arguments=="exit"||arguments=="type"){
            std::cout<<arguments<<" is a shell builtin"<<std::endl;
          }else{
            std::string path=get_path_of_command(arguments);
            if(!path.empty()){
              std::cout<<arguments<<" is "<<path<<std::endl;
            }else{
            std::cout << arguments << ": not found" << std::endl;
            }
          }

          
        
        }
    }else{
      if (command == "echo"){
        std::cout << "" << std::endl; // echo 空内容只打印一个换行
      }else {
            std::cout << command << ": command not found" << std::endl;
      }
    }
   
  
  }
  return 0;
  
  
}
