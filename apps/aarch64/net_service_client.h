#include "lume/service.h"
static struct lume_net_service_status net_service;

static int service_get(void)
{
    return (int)call(LUME_SYS_SERVICE, LUME_SERVICE_GET, (uintptr_t)&net_service,
                     sizeof(net_service));
}

static int service_send(unsigned command, const char *name)
{
    struct lume_service_message m = {.version = 1, .command = command};
    if (name)
    {
        size_t n = len(name);
        if (!n || n >= sizeof(m.name))
            return 1;
        for (unsigned i = 0; i < n; ++i)
            m.name[i] = name[i];
    }
    int64_t seq = call(LUME_SYS_SERVICE, LUME_SERVICE_SEND, (uintptr_t)&m, sizeof(m));
    if (seq < 0)
    {
        net_text("net: service busy or unavailable\n");
        return 1;
    }
    uint64_t until = (uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) + 90000;
    do
    {
        if (service_get() < 0)
            return 1;
        if (net_service.acknowledged == (uint64_t)seq)
        {
            if (net_service.result)
            {
                net_text("net: command failed\n");
                return 1;
            }
            net_text("net: command applied\n");
            return 0;
        }
        (void)call(LUME_SYS_SLEEP, 50, 0, 0);
    } while ((uint64_t)call(LUME_SYS_CLOCK, 0, 0, 0) < until);
    net_text("net: command acknowledgement timed out\n");
    return 1;
}

/* -1 delegates a mechanism/diagnostic command to the typed network client. */
static int service_command(char **args, unsigned count)
{
    if (count < 2)
        return -1;
    const char *verb = args[1];
    unsigned command = 0;
    if (eq(verb, "service") || eq(verb, "profiles"))
    {
        if (count != 2 || service_get() < 0)
        {
            net_text("net: service unavailable\n");
            return 1;
        }
        if (eq(verb, "service"))
        {
            static const char *const states[] = {"starting",   "off",    "waiting",
                                                 "connecting", "online", "error"};
            net_text("net service PID ");
            net_number(net_service.pid);
            net_text(" state ");
            net_text(net_service.state < 6 ? states[net_service.state] : "invalid");
            net_text(net_service.automatic ? " auto on" : " auto off");
            net_text(" profile ");
            net_text(net_service.active);
            net_text(" generation ");
            net_number(net_service.generation);
            net_text(" attempts ");
            net_number(net_service.attempts);
            net_text(" retry-at-ms ");
            net_number(net_service.retry_at);
            net_text("\n");
        }
        else
            for (unsigned i = 0; i < net_service.profiles && i < 4; ++i)
            {
                net_text(net_service.profile[i].name);
                net_text(" priority ");
                net_number(net_service.profile[i].priority);
                net_text("\n");
            }
        return 0;
    }
    if (eq(verb, "auto"))
    {
        if (count != 3 || (!eq(args[2], "on") && !eq(args[2], "off")))
        {
            net_text("net auto on|off\n");
            return 1;
        }
        command = eq(args[2], "on") ? NETD_AUTO_ON : NETD_AUTO_OFF;
    }
    else if (eq(verb, "import"))
        command = NETD_IMPORT;
    else if (eq(verb, "forget"))
        command = NETD_FORGET;
    else if (eq(verb, "prefer"))
        command = NETD_PREFER;
    else if (eq(verb, "connect"))
        command = NETD_CONNECT;
    else if (eq(verb, "reconnect"))
        command = NETD_RECONNECT;
    else if (eq(verb, "reload"))
        command = NETD_RELOAD;
    else if (eq(verb, "leave"))
        command = NETD_PAUSE;
    else if (eq(verb, "join"))
        return service_send(NETD_CONNECT, "current");
    else
        return -1;
    bool named = command == NETD_IMPORT || command == NETD_FORGET || command == NETD_PREFER ||
                 command == NETD_CONNECT;
    if ((named && count != 3) ||
        (!named && command != NETD_AUTO_ON && command != NETD_AUTO_OFF && count != 2))
    {
        net_text(
            "net service|profiles|auto on|off|import NAME|forget NAME|prefer NAME|connect NAME|reconnect|reload|leave\n");
        return 1;
    }
    return service_send(command, named ? args[2] : 0);
}
