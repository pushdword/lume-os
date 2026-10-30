/* SPDX-License-Identifier: BSD-3-Clause */
extern int main(int, char **, char **);
static const char *program_name;

const char *getprogname(void)
{
    return program_name ? program_name : "";
}

int app_main(unsigned argc, char **argv, char **envp)
{
    program_name = argc ? argv[0] : "";
    for (const char *p = program_name; *p; ++p)
        if (*p == '/')
            program_name = p + 1;
    return main((int)argc, argv, envp);
}
