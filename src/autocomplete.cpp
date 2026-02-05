#include <readline/readline.h>
#include <vector>
#include <string>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <set>
#include <algorithm>

static std::vector<std::string> builtins = {"echo", "exit", "type", "pwd", "cd"};

std::set<std::string> get_external_commands(){
    std::set<std::string> commands;
    char* path_env=getenv("PATH");
    if(!path_env) return commands;
    char* path_dup=strdup(path_env);
    char* dir_path=strtok(path_dup,":");
    
    while(dir_path!=nullptr){
        DIR* dir=opendir(dir_path);
        if(dir){
            struct dirent* entry;
            while((entry=readdir(dir))!=nullptr){
                if(entry->d_name[0]=='.') continue;
                std::string full_path=std::string(dir_path)+"/"+entry->d_name;
                if(access(full_path.c_str(),X_OK)==0){
                    commands.insert(entry->d_name);
                }
            }
            closedir(dir);
        }
        //切换目录
        dir_path = strtok(nullptr, ":");
    }
    free(path_dup);
    return commands;
    
}
static char* command_generator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t match_index;


    // 【优化关键】：使用 static 存储，只在第一次使用时初始化
    static std::set<std::string> cached_external_cmds;
    static bool commands_loaded = false;
    if (!state) { // 第一次按下 Tab，搜集所有候选
        matches.clear();
        match_index = 0;
        std::string prefix(text);

        // 1. 如果没加载过，只加载这一次
        if (!commands_loaded) {
            cached_external_cmds = get_external_commands();
            commands_loaded = true;
        }
       
        for (const auto& cmd : builtins) {
            if (cmd.compare(0, prefix.size(), prefix) == 0) {
                matches.push_back(cmd);
            }
        }

        // 2. 检查外部命令 (PATH)
        std::set<std::string> external_cmds = get_external_commands();
        for (const auto& cmd : external_cmds) {
            if (cmd.compare(0, prefix.size(), prefix) == 0) {
                // 避免与内置命令重复
                if (std::find(builtins.begin(), builtins.end(), cmd) == builtins.end()) {
                    //在builtins里找，找不到则加入
                    matches.push_back(cmd);
                }
            }
        }
    }

    if (match_index < matches.size()) {
        return strdup(matches[match_index++].c_str());
    }

    return nullptr;
}
char** my_completion(const char* text,int start,int end){
    if(start==0){
        return rl_completion_matches(text, command_generator);
    }
    return nullptr;
}
// 4. 初始化函数（供 main.cpp 调用）
void initialize_readline() {
    rl_attempted_completion_function = my_completion;
}