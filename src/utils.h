#pragma once // 1. 防止头文件被重复引用
#include <string> // <--- 必须加！为了让编译器认识 std::string
#include <vector>   
// 2. 函数声明
void print_welcome();
std::string get_path_of_command(const std::string& command);
std::vector<std::string> split_arguments(const std::string& command);