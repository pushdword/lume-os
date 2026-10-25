#include "native.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

static int directory(const char *path, mode_t mode)
{
    if (mkdir(path, mode) && errno != EEXIST)
        return -1;
    struct stat metadata;
    if (stat(path, &metadata) || !S_ISDIR(metadata.st_mode) || metadata.st_uid ||
        (metadata.st_mode & 0022))
    {
        errno = EACCES;
        return -1;
    }
    return chmod(path, mode);
}

static int copy_default(void)
{
    int output = open("/data/etc/ssh/sshd_config", O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (output < 0)
        return errno == EEXIST ? 0 : -1;
    int source = open("/data/share/openssh/sshd_config", O_RDONLY);
    if (source < 0)
    {
        close(output);
        unlink("/data/etc/ssh/sshd_config");
        return -1;
    }
    char buffer[1024];
    ssize_t count;
    int result = 0;
    while ((count = read(source, buffer, sizeof(buffer))) > 0)
    {
        ssize_t sent = 0;
        while (sent < count)
        {
            ssize_t n = write(output, buffer + sent, count - sent);
            if (n <= 0)
            {
                result = -1;
                break;
            }
            sent += n;
        }
        if (result)
            break;
    }
    if (count < 0 || fsync(output))
        result = -1;
    close(source);
    if (close(output))
        result = -1;
    if (result)
        unlink("/data/etc/ssh/sshd_config");
    return result;
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc != 1 || getuid() || geteuid())
    {
        fputs("usage: sshd-setup (requires root)\n", stderr);
        return 1;
    }
    umask(077);
    const char *paths[] = {"/data/etc",     "/data/etc/ssh",        "/data/var",
                           "/data/var/run", "/data/var/empty-sshd", "/data/root"};
    for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
        if (directory(paths[i], i == 1 || i == 5 ? 0700 : 0755))
        {
            fprintf(stderr, "sshd-setup: directory %s: %s\n", paths[i], strerror(errno));
            return 1;
        }
    if (copy_default())
    {
        perror("sshd-setup: configuration");
        return 1;
    }
    const char *key = "/data/etc/ssh/ssh_host_ed25519_key";
    struct stat metadata;
    if (stat(key, &metadata))
    {
        if (errno != ENOENT)
        {
            perror("sshd-setup: host key");
            return 1;
        }
        pid_t child = fork();
        if (!child)
        {
            execl("/data/bin/ssh-keygen", "ssh-keygen", "-q", "-t", "ed25519", "-N", "", "-f", key,
                  (char *)NULL);
            _exit(127);
        }
        int status, result;
        if (child < 0)
        {
            perror("sshd-setup: key generator");
            return 1;
        }
        do
        {
            result = waitpid(child, &status, 0);
        } while (result < 0 && errno == EINTR);
        if (result < 0 || !WIFEXITED(status) || WEXITSTATUS(status) || stat(key, &metadata))
        {
            fputs("sshd-setup: host key generation failed\n", stderr);
            return 1;
        }
    }
    if (!S_ISREG(metadata.st_mode) || metadata.st_uid || (metadata.st_mode & 0077))
    {
        fputs("sshd-setup: host key requires root ownership and mode 0600\n", stderr);
        return 1;
    }
    int fd = open("/data/etc/ssh/authorized_keys", O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd >= 0)
    {
        if (fsync(fd) || close(fd))
            return 1;
    }
    else if (errno != EEXIST)
    {
        perror("sshd-setup: authorized keys");
        return 1;
    }
    puts("OpenSSH host key and configuration are ready.");
    puts("Install an Ed25519 public key in /data/etc/ssh/authorized_keys before starting sshd.");
    return 0;
}
