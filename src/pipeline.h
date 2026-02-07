#ifndef PIPELINE_H
#define PIPELINE_H
// 执行 command1 | command2
// cmd1: ["ls", "-l", NULL]
// cmd2: ["grep", ".c", NULL]
#include <vector>
#include <string>

// 将结构体定义移到这里，这样 main.cpp 和 pipeline.cpp 都能看见它
struct Command {
    std::vector<std::string> args;
    std::string redirect_file = "";
    bool is_append = false;
    int redirect_fd_type = 1;

    // 辅助转换函数也可以放在这里（内联）
    char** to_argv() const {
        char** argv = new char*[args.size() + 1];
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = const_cast<char*>(args[i].c_str());
        }
        argv[args.size()] = nullptr;
        return argv;
    }
};
int execute_pipeline(const Command& cmd1, const Command& cmd2);
void execute_multi_pipeline(const std::vector<Command>& commands);
#endif