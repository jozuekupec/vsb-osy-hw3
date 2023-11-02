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

#define INPUT_BUFFER_SIZE 1024
#define OUTPUT_BUFFER_SIZE 1024

using namespace std;

enum output_type
{
    STD_OUTPUT,
    FILE_OUTPUT
};

struct command
{
    string executable;
    vector<string> args;
};

struct output
{
    output_type type;
    string destination; // Používá se pro file output
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

vector<command> parse_input(const char *t_input, output *out)
{
    string l_input = t_input;
    vector<string> l_comands;
    size_t l_command_start = 0, l_command_end = 0;
    vector<command> l_result;

    // Vyhodnocení cíle výstupu
    size_t l_arrow_position = l_input.find('>', l_command_start);
    if (l_arrow_position != string::npos)
    {
        l_arrow_position++; // Operátor přesměrování přeskakujeme
        string l_destination = l_input.substr(l_arrow_position, l_input.size() - l_arrow_position);
        l_destination = trim(l_destination);
        if (l_destination.size() == 0)
        {
            fprintf(stderr, "Invalid output destination!\n");
            exit(1);
        }

        out->type = FILE_OUTPUT;
        out->destination = l_destination;

        l_input = l_input.substr(l_command_start, l_arrow_position - l_command_start - 1); // Aktualizace vstupu (seříznutí o destination)
    }
    else
    {
        out->type = STD_OUTPUT;
        out->destination = ""; // Pro STD output se nepoužívá, tak jej pouze uklidíme
    }

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

int process_command(command t_command, int input_pipe)
{
    int output_pipe[2];
    pipe(output_pipe);

    if (fork() == 0)
    {
        close(output_pipe[0]);
        dup2(output_pipe[1], STDOUT_FILENO);
        dup2(input_pipe, STDIN_FILENO);

        vector<char *> args;
        char *l_tmp_str = nullptr;

        l_tmp_str = const_cast<char *>(t_command.executable.c_str());
        args.push_back(l_tmp_str);
        for (auto &&arg : t_command.args)
        {
            l_tmp_str = const_cast<char *>(arg.c_str());
            args.push_back(l_tmp_str);
        }
        args.push_back(nullptr);

        char **argsv = args.data();
        execvp(argsv[0], argsv);
        exit(1); // Tady by to nemělo ani dojít
    }

    close(output_pipe[1]);
    wait(NULL);

    return output_pipe[0]; // Řizení předáváme zpět a s tím i konec pro čtení
}

void process_output(output out, int input_pipe_read_end)
{
    char buffer[OUTPUT_BUFFER_SIZE];
    ssize_t read_size;

    if (out.type == STD_OUTPUT)
    {
        while ((read_size = read(input_pipe_read_end, buffer, OUTPUT_BUFFER_SIZE)) > 0)
        {
            write(STDOUT_FILENO, buffer, read_size);
        }
    }
    else if (out.type == FILE_OUTPUT)
    {
        FILE *file = fopen(out.destination.c_str(), "w");
        while ((read_size = read(input_pipe_read_end, buffer, OUTPUT_BUFFER_SIZE)) > 0)
        {
            fwrite(buffer, 1, read_size, file);
        }
        fclose(file);
    }
    else
    {
        fprintf(stderr, "Undefined output type!\n");
        exit(1);
    }

    close(input_pipe_read_end);
}

int main()
{
    char input[INPUT_BUFFER_SIZE];
    int input_pipe_read_end = open("/dev/null", O_RDONLY);

    while (true)
    {
        printf("$ ");
        fflush(stdout);
        fgets(input, sizeof(input), stdin);
        output out;

        size_t length = strlen(input);
        if (length > 0 && input[length - 1] == '\n')
            input[length - 1] = '\0'; // Nahrazení newline za konec řetězce

        vector<command> l_commands = parse_input(input, &out);
        if (l_commands.size() > 10) // Nastavení limitu pro vstup uživatele v jednom řádku
        {
            fprintf(stderr, "Exceeded limit of 10 commands in row!\n");
            exit(1);
        }

        // Zpracování příkazů
        for (auto &&command : l_commands)
        {
            input_pipe_read_end = process_command(command, input_pipe_read_end);
        }

        // Vypsání výstupu
        process_output(out, input_pipe_read_end);
        printf("\n");
    }

    return 0;
}
