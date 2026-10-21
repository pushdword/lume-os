#include "lib.h"
#include <stdbool.h>
#include "net_client.h"

int main(int argc, char **argv)
{
    return network(argv, (unsigned)argc);
}
