/* SPDX-License-Identifier: BSD-3-Clause */
#include <config.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_SIMD
#define STBI_MAX_DIMENSIONS 6000
#include "stb_image.h"
#include "djpeg.h"
#include "cache.h"
#include "capi.h"
#include "dicache.h"
#include <limits.h>
#include <stdlib.h>

struct jpeg_image
{
    DilloImage *image;
    DilloUrl *url;
    int version;
    int width, height;
    bool_t decoded;
};

void *a_Jpeg_new(DilloImage *image, DilloUrl *url, int version)
{
    struct jpeg_image *state = calloc(1, sizeof(*state));
    if (state)
    {
        state->image = image;
        state->url = url;
        state->version = version;
    }
    return state;
}

static bool_t header(struct jpeg_image *state, const void *buffer, size_t bytes)
{
    if (!state->image)
        return TRUE;
    int channels;
    if (!buffer || bytes > INT_MAX ||
        !stbi_info_from_memory(buffer, (int)bytes, &state->width, &state->height, &channels) ||
        state->width <= 0 || state->height <= 0 || state->width > IMAGE_MAX_AREA / state->height)
        return FALSE;
    a_Dicache_set_parms(state->url, state->version, state->image, (uint_t)state->width,
                        (uint_t)state->height, DILLO_IMG_TYPE_RGB, 1 / 2.2);
    state->image = NULL;
    return TRUE;
}

static void decode(struct jpeg_image *state, const void *buffer, size_t bytes)
{
    if (state->decoded)
        return;
    state->decoded = TRUE;
    int width, height, channels;
    if (!header(state, buffer, bytes))
        return;
    unsigned char *pixels =
        stbi_load_from_memory(buffer, (int)bytes, &width, &height, &channels, 3);
    if (!pixels)
        return;
    if (width != state->width || height != state->height)
    {
        stbi_image_free(pixels);
        return;
    }
    for (int row = 0; row < height; ++row)
        a_Dicache_write(state->url, state->version, pixels + (size_t)row * (size_t)width * 3,
                        (uint_t)row);
    stbi_image_free(pixels);
}

void a_Jpeg_callback(int operation, void *data)
{
    if (operation == CA_Abort)
    {
        free(data);
        return;
    }
    CacheClient_t *client = data;
    struct jpeg_image *state = client->CbData;
    if (!state)
        return;
    if (operation == CA_Send)
        (void)header(state, client->Buf, client->BufSize);
    const unsigned char *bytes = client->Buf;
    bool_t finished = bytes && client->BufSize >= 2 && bytes[client->BufSize - 2] == 0xff &&
                      bytes[client->BufSize - 1] == 0xd9;
    if (operation == CA_Close ||
        (operation == CA_Send && (finished || (a_Capi_get_flags(state->url) & CAPI_Completed))))
        decode(state, client->Buf, client->BufSize);
    if (operation == CA_Close)
    {
        a_Dicache_close(state->url, state->version, client);
        free(state);
    }
}

const char *a_Jpeg_version(void)
{
    return "stb_image 2.30";
}
