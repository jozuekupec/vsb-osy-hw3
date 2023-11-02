#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>

using namespace std;

struct command
{
    string executable;
    vector<string> args;
};

string trim(string t_input)
{
    size_t trimmed_start_position = t_input.find_first_not_of(" ");
    size_t trimmed_end_position = t_input.find_last_not_of(" ");
    if (trimmed_start_position == string::npos || trimmed_end_position == string::npos)
        return (string) "";

    return t_input.substr(trimmed_start_position, trimmed_end_position - trimmed_start_position + 1);
}

command parse_command(string t_command)
{
    command l_result;
    size_t l_part_start = 0, l_part_end = 0;

    // Načtení spustielného příkazu (argv[0])
    l_part_end = t_command.find_first_of(" ");
    if (l_part_end == string::npos)
        l_part_end = t_command.size();
    l_result.executable = t_command.substr(l_part_start, l_part_end);

    // Parse argumentů (pokud jsou)
    if (l_part_end < t_command.size())
    {
        l_part_end++; // Prázdný znak za spustitelným souborem
        string l_args = t_command.substr(l_part_end, t_command.size() - l_part_end);
        while ((l_part_end = l_args.find(' ', l_part_start)) != string::npos)
        {
            string command = l_args.substr(l_part_start, l_part_end - l_part_start);
            l_result.args.push_back(command);
            l_part_start = l_part_end + 1;
        }
        l_result.args.push_back(l_args.substr(l_part_start)); // Na závěr přidáme poslední hodnotu
    }

    return l_result;
}

vector<command> parse_input(const char *t_input)
{
    string l_input = t_input;
    vector<string> l_comands;
    size_t l_command_start = 0, l_command_end = 0;
    vector<command> l_result;

    // Explode příkazů
    while ((l_command_end = l_input.find('|', l_command_start)) != string::npos)
    {
        string command = l_input.substr(l_command_start, l_command_end - l_command_start);
        l_comands.push_back(trim(command));
        l_command_start = l_command_end + 1;
    }
    l_comands.push_back(trim(l_input.substr(l_command_start))); // Na závěr přidáme poslední hodnotu

    // Parse příkazů
    for (auto &&command : l_comands)
    {
        l_result.push_back(parse_command(command));
    }

    return l_result;
}

int main()
{
    char input[1024];

    printf("$ ");
    fgets(input, sizeof(input), stdin);

    size_t length = strlen(input);
    if (length > 0 && input[length - 1] == '\n')
        input[length - 1] = '\0'; // Nahrazení newline za konec řetězce

    vector<command> l_commands = parse_input(input);

    for (auto &&command : l_commands)
    {
        vector<char *> args;
        char *l_tmp_str = nullptr;

        l_tmp_str = const_cast<char *>(command.executable.c_str());
        args.push_back(l_tmp_str);
        for (auto &&arg : command.args)
        {
            l_tmp_str = const_cast<char *>(arg.c_str());
            args.push_back(l_tmp_str);
        }
        args.push_back(nullptr);

        char **argsv = args.data();

        printf("Name: %s\n", argsv[0]);

        execvp(argsv[0], argsv);
    }

    // int l_pipe[2], l_pipe_2[2];
    // pipe(l_pipe);
    // pipe(l_pipe_2);

    // if (fork() == 0)
    // {
    //     close(l_pipe[0]);
    //     close(l_pipe_2[0]);
    //     close(l_pipe_2[1]);
    //     dup2(l_pipe[1], 1);
    //     close(l_pipe[1]);

    //     int l_file_descriptor = open("lsout.txt", O_CREAT | O_RDWR | O_TRUNC, 0600);
    //     dup2(l_file_descriptor, 1);
    //     close(l_file_descriptor);

    //     char *args[] = {"ls", "-la", "/usr", "/var", nullptr};
    //     execvp(args[0], args);

    //     fprintf(stderr, "!!! Program should not reach this content... !!!");
    //     exit(1);
    // }

    // if (fork() == 0)
    // {
    //     close(l_pipe[1]);
    //     close(l_pipe_2[0]);
    //     dup2(l_pipe[0], 0);
    //     dup2(l_pipe_2[1], 1);
    //     close(l_pipe[0]);
    //     close(l_pipe_2[1]);

    //     execlp("tr", "tr", "[a-z]", "[A-Z]", nullptr);
    //     fprintf(stderr, "!!! Program should not reach this content... !!!");
    //     exit(1);
    // }

    // close(l_pipe[0]);
    // close(l_pipe[1]);
    // close(l_pipe_2[1]);

    // while (true)
    // {
    //     char buffer[128];
    //     int len = read(l_pipe[0], buffer, sizeof(buffer));
    //     if (len <= 0)
    //         break;
    //     for (int i = 0; i < len; i++)
    //         buffer[i] = toupper(buffer[i]);

    //     write(1, buffer, sizeof(len));
    // }

    // close(l_pipe[0]);
    // wait(nullptr);
    // wait(nullptr);

    return 0;
}
