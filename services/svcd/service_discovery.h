#ifndef LUME_SERVICE_DISCOVERY_H
#define LUME_SERVICE_DISCOVERY_H

static bool discovery_valid;
static int discovery_result;

static void discovery_invalidate(void)
{
    discovery_valid = false;
}

static int discovery_refresh(void)
{
    if (!discovery_valid)
    {
        discovery_result = discover(SERVICE_OVERRIDE);
        if (!discovery_result)
            discovery_result = discover(SERVICE_INSTALLED);
        discovery_valid = true;
    }
    return discovery_result;
}

#endif
