#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
static unsigned checks;

static void run(const char *label, const char *program, const char *argument, const char *expected,
                int status)
{
    printf("RUN SHELL TOOLS %s\n", label);
    int descriptors[2];
    if (pipe(descriptors))
        exit(2);
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, descriptors[1], 1);
    posix_spawn_file_actions_adddup2(&actions, descriptors[1], 2);
    posix_spawn_file_actions_addclose(&actions, descriptors[0]);
    posix_spawn_file_actions_addclose(&actions, descriptors[1]);
    char *arguments[] = {(char *)program, "-c", (char *)argument, NULL};
    pid_t child = -1;
    int error = posix_spawn(&child, program, &actions, NULL, arguments, environ);
    printf("SPAWN SHELL TOOLS %s result=%d pid=%d\n", label, error, (int)child);
    posix_spawn_file_actions_destroy(&actions);
    close(descriptors[1]);
    char output[8192];
    size_t used = 0;
    ssize_t count;
    while ((count = read(descriptors[0], output + used, sizeof(output) - used - 1)) > 0)
        used += count;
    output[used] = 0;
    close(descriptors[0]);
    int result = -1;
    pid_t waited = error ? -1 : waitpid(child, &result, 0);
    if (error || waited != child || !WIFEXITED(result) || WEXITSTATUS(result) != status ||
        (expected && !strstr(output, expected)))
    {
        printf("FAIL SHELL TOOLS %s spawn=%d wait=%d status=%d output=[%s]\n", label, error,
               (int)waited, result, output);
        exit(1);
    }
    ++checks;
    printf("PASS SHELL TOOLS %s\n", label);
}

int main(void)
{
    mkdir("/data/tmp", 0777);
    run("version", "/bin/bash", "printf '%s' \"$BASH_VERSION\"", "5.3.20", 0);
    run("arrays-functions", "/bin/bash",
        "a=(zero one two); declare -A m=([key]=value); f(){ printf '%s:%s:%s' \"${a[2]}\" \"${m[key]}\" \"$((7*6))\"; }; f",
        "two:value:42", 0);
    run("expansion", "/bin/bash",
        "v=$(printf 'a b'); printf '<%s>' \"$v\"; for n in {1..3}; do printf '%s' \"$n\"; done",
        "<a b>123", 0);
    run("pipeline", "/bin/bash", "set -o pipefail; printf 'pipe-ok\\n' | /bin/cat", "pipe-ok", 0);
    run("redirect-read", "/bin/bash",
        "printf 'redirect-ok\\n' > /data/tmp/bash-test; IFS= read -r value < /data/tmp/bash-test; printf '%s' \"$value\"",
        "redirect-ok", 0);
    run("source", "/bin/bash",
        "printf 'x=source-ok\\n' > /data/tmp/bash-source; source /data/tmp/bash-source; printf '%s' \"$x\"",
        "source-ok", 0);
    run("regular-expression", "/bin/bash",
        "[[ abc42 =~ ([a-z]+)([0-9]+) ]] && printf '%s:%s' \"${BASH_REMATCH[1]}\" \"${BASH_REMATCH[2]}\"",
        "abc:42", 0);
    run("signal-trap", "/bin/bash", "trap 'printf signal-ok' USR1; kill -USR1 $$", "signal-ok", 0);
    run("programmable-completion", "/bin/bash",
        "complete -W 'alpha beta' example; compgen -W 'alpha beta' a", "alpha", 0);
    run("background-wait", "/bin/bash", "{ exit 7; } & p=$!; wait $p; printf 'wait=%s' $?",
        "wait=7", 0);
    run("exit-status", "/bin/bash", "exit 37", NULL, 37);
    run("command-error", "/bin/bash", "lume_nonexistent_command", "command not found", 127);
    run("hostname", "/bin/bash", "/bin/hostname", "lume", 0);
    run("static-ipv4", "/bin/bash",
        "/bin/ifconfig eth0 10.0.2.15 netmask 255.255.255.0 up && /bin/ifconfig eth0", "10.0.2.15",
        0);
    run("default-gateway", "/bin/bash",
        "/bin/route add default gw 10.0.2.2 dev eth0 && /bin/route -n", "10.0.2.2", 0);
    run("delete-gateway", "/bin/bash", "/bin/route del default", NULL, 0);
    run("restore-gateway", "/bin/bash", "/bin/route add default gw 10.0.2.2 dev eth0", NULL, 0);
    run("interfaces", "/bin/bash", "/bin/ifconfig -a", "mtu", 0);
    run("routes", "/bin/bash", "/bin/route -n", "Kernel IP routing table", 0);
    run("network-statistics", "/bin/bash", "/bin/netstat -i", "Iface", 0);
    run("connections", "/bin/bash", "/bin/netstat -ntu", "Proto", 0);
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_port = htons(23456)};
    if (listener < 0 || bind(listener, (struct sockaddr *)&address, sizeof(address)) ||
        listen(listener, 2))
    {
        puts("FAIL SHELL TOOLS create listener");
        return 1;
    }
    run("live-listener", "/bin/bash", "/bin/netstat -ltn", "23456", 0);
    close(listener);
    run("default-netstat", "/bin/bash", "/bin/netstat -n", "Proto", 0);
    run("neighbors", "/bin/bash", "/bin/arp -n", NULL, 0);
    printf("PASS SHELL TOOLS COMPLETE checks=%u\n", checks);
    return 0;
}
