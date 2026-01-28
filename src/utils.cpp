#include <iostream>
#include "utils.h" // 1. 包含对应的头文件
#include <vector>
#include <string>
#include <sstream> //used for stringstream
#include <cstdlib> //used for std::getenv
#include <filesystem> //used for fs::exists,etc

namespace fs = std::filesystem; // 起个别名，方便后面写
// 2. 函数的具体实现

//分词，区分命令
std::vector<std::string> split_arguments(const std::string& command){
    std::vector<std::string> args;
    std::string current_token;
    bool in_single_quotes=false;
    
    for(size_t i=0;i<command.length();i++){
        char c=command[i];
        if(c=='\''){
            in_single_quotes=!in_single_quotes;
        }
        else if(c==' ' && !in_single_quotes){
            if(!current_token.empty()){
                args.push_back(current_token);
                current_token.clear();
            }
        }
        else{
            current_token +=c;
        }
        
    }
    // 处理最后一个残留在缓冲区里的参数
        if (!current_token.empty()) {
            args.push_back(current_token);
        }
    
    return args;
}
void print_welcome() {
    std::cout << "------------------------------------" << std::endl;
    std::cout << "   Welcome to My CodeCrafters Shell " << std::endl;
    std::cout << "------------------------------------" << std::endl;
}
std::string get_path_of_command(const std::string & command){
    const char* path_env=std::getenv("PATH");
    if(!path_env) return "";
    std::string path_str=path_env;
    std::stringstream ss(path_str);//把这个string path_str变成一个stringstream ss
    std::string directory;
    // --- 调试：打印一下它读到的 PATH 到底长什么样 ---
    //std::cout << "[DEBUG] Full PATH: " << path_str << std::endl;
    while (std::getline(ss,directory,':'))
    {
        //目录+/+命令
        fs::path full_path=fs::path(directory)/command;

        // --- 调试：打印它正在检查哪个路径 ---
        //std::cout << "[DEBUG] Checking: " << full_path << std::endl;
        if(fs::exists(full_path)&&fs::is_regular_file(full_path)){
            auto perms=fs::status(full_path).permissions();
            if ((perms & fs::perms::owner_exec) != fs::perms::none ||
                (perms & fs::perms::group_exec) != fs::perms::none ||
                (perms & fs::perms::others_exec) != fs::perms::none) {
                
                return full_path.string(); // 找到了！返回绝对路径
                }
        }
        
    }
    
    return ""; // 找遍了所有目录都没找到
}