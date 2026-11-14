/* SPDX-License-Identifier: BSD-3-Clause */
extern int main(int, char **, char **);
static char empty_name[] = "";
char *program_invocation_name = empty_name;
char *program_invocation_short_name = empty_name;

const char *getprogname(void)
{
    return program_invocation_short_name ? program_invocation_short_name : "";
}

int app_main(unsigned argc, char **argv, char **envp)
{
    program_invocation_name = argc ? argv[0] : empty_name;
    program_invocation_short_name = program_invocation_name;
    for (char *p = program_invocation_name; *p; ++p)
        if (*p == '/')
            program_invocation_short_name = p + 1;
    return main((int)argc, argv, envp);
}
