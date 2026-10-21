#include "api.h"
int main(int, char **);
int lume_rotate_seed(void);
int ssh_crypto_initialize(void);

int app_main(unsigned argc, char **argv)
{
    int r = ssh_crypto_initialize();
    if (r < 0)
    {
        app_error("sshd: crypto initialization failed", r);
        return 1;
    }
    r = lume_rotate_seed();
    if (r < 0)
    {
        app_error("sshd: seed rotation failed", r);
        return 1;
    }
    return main((int)argc, argv);
}
