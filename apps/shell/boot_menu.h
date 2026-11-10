#ifndef LUME_SHELL_BOOT_MENU_H
#define LUME_SHELL_BOOT_MENU_H

static int shell_boot_linux(void)
{
    if (lume_syscall(LUME_PID, 0, 0, 0) != 1)
    {
        app_puts("boot: use the system console\n");
        return 0;
    }
    struct lume_boot_request request = {.version = LUME_IO_VERSION};
    if (app_boot(LUME_BOOT_CAPABILITIES, &request) < 0 ||
        !(request.total & LUME_BOOT_CAP_PARTITION))
    {
        app_puts("boot: Linux is unavailable\n");
        return 0;
    }
    request.total = 0;
    int result = app_boot(LUME_BOOT_SELECT_PARTITION, &request);
    if (result < 0)
    {
        app_error("boot", result);
        return 0;
    }
    app_puts("Starting Linux...\n");
    return 1;
}

static int shell_boot_menu(void)
{
    if (lume_syscall(LUME_PID, 0, 0, 0) != 1)
        return 0;
    struct lume_boot_request request = {.version = LUME_IO_VERSION};
    if (app_boot(LUME_BOOT_INFO, &request) < 0 || request.total & LUME_BOOT_STATE_TRIAL)
        return 0;
    if (app_boot(LUME_BOOT_CAPABILITIES, &request) < 0 ||
        !(request.total & LUME_BOOT_CAP_PARTITION))
        return 0;
    int fd = app_open("/etc/bootmenu.conf", LUME_RDONLY);
    if (fd < 0)
        return 0;
    char config[32];
    int bytes = app_read(fd, config, sizeof(config) - 1);
    app_close(fd);
    if (bytes <= 8 || bytes >= (int)sizeof(config) - 1)
        return 0;
    config[bytes] = 0;
    if (memcmp(config, "timeout=", 8))
        return 0;
    unsigned seconds = 0;
    char *text = config + 8;
    if (*text < '0' || *text > '9')
        return 0;
    while (*text >= '0' && *text <= '9')
    {
        seconds = seconds * 10 + (unsigned)(*text++ - '0');
        if (seconds > 30)
            return 0;
    }
    while (*text == ' ' || *text == '\r' || *text == '\n' || *text == '\t')
        ++text;
    if (*text || !seconds)
        return 0;
    app_puts("\n1. LumeOS (default)\n2. Linux\nSelect 1 or 2. LumeOS starts automatically.\n");
    uint32_t start = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0);
    uint32_t timeout = seconds * 1000;
    for (;;)
    {
        uint32_t elapsed = (uint32_t)lume_syscall(LUME_INFO, LUME_UPTIME, 0, 0) - start;
        if (elapsed >= timeout)
            break;
        int key = lume_syscall(LUME_GETCHAR, 0, 0, 0);
        if (key == '2')
            return shell_boot_linux();
        if (key >= 0)
            break;
        int result = lume_syscall(LUME_TERMINAL, 5, timeout - elapsed, 0);
        if (result < 0 && result != -4)
            break;
    }
    app_puts("Starting LumeOS...\n");
    return 0;
}
#endif
