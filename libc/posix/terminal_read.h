#ifndef LUME_TERMINAL_READ_H
#define LUME_TERMINAL_READ_H
#include <stddef.h>
#include <stdint.h>

struct lume_terminal_reader
{
    uint64_t (*clock)(void *);
    int64_t (*read)(void *, void *, size_t);
    int (*wait)(void *, uint64_t);
    void *context;
};

static int64_t lume_terminal_read(struct lume_terminal_reader *reader, void *buffer, size_t bytes,
                                  unsigned minimum, unsigned deciseconds)
{
    uint64_t duration = (uint64_t)deciseconds * 100;
    size_t goal = minimum < bytes ? minimum : bytes, total = 0;
    uint64_t deadline =
        !minimum && duration ? reader->clock(reader->context) + duration : UINT64_MAX;
    for (;;)
    {
        int64_t count =
            reader->read(reader->context, (unsigned char *)buffer + total, bytes - total);
        if (count > (int64_t)(bytes - total))
            return total ? (int64_t)total : -5;
        if (count > 0)
        {
            total += (size_t)count;
            if (total >= goal)
                return (int64_t)total;
            if (duration)
                deadline = reader->clock(reader->context) + duration;
            continue;
        }
        if (!count || (count == -11 && !minimum && !duration))
            return (int64_t)total;
        if (count != -11)
            return total ? (int64_t)total : count;
        uint64_t now = reader->clock(reader->context);
        if (deadline != UINT64_MAX && now >= deadline)
            return (int64_t)total;
        int result =
            reader->wait(reader->context, deadline == UINT64_MAX ? UINT64_MAX : deadline - now);
        if (result == -11)
            continue;
        if (result < 0)
            return total ? (int64_t)total : result;
        if (!result && deadline != UINT64_MAX)
            return (int64_t)total;
    }
}
#endif
