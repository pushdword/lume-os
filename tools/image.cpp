#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
using Bytes = std::vector<unsigned char>;

static void check(bool v, const std::string &s)
{
    if (!v)
        throw std::runtime_error(s);
}

static Bytes read(const fs::path &p)
{
    std::ifstream f(p, std::ios::binary);
    check(bool(f), "Cannot read " + p.string());
    return Bytes(std::istreambuf_iterator<char>(f), {});
}

static void write(const fs::path &p, const Bytes &b)
{
    std::ofstream f(p, std::ios::binary);
    check(bool(f), "Cannot create " + p.string());
    f.write((const char *)b.data(), b.size());
    check(bool(f), "Write failed");
}

static uint64_t get(const Bytes &b, size_t p, unsigned n)
{
    check(p <= b.size() && n <= b.size() - p, "Truncated image");
    uint64_t v = 0;
    for (unsigned i = 0; i < n; i++)
        v |= uint64_t(b[p + i]) << (8 * i);
    return v;
}

static void put(Bytes &b, size_t p, uint64_t v, unsigned n)
{
    check(p <= b.size() && n <= b.size() - p, "Image field outside buffer");
    for (unsigned i = 0; i < n; i++)
        b[p + i] = uint8_t(v >> (i * 8));
}

static uint32_t crc32(const Bytes &b)
{
    uint32_t c = ~0u;
    for (auto v : b)
    {
        c ^= v;
        for (unsigned n = 0; n < 8; n++)
            c = (c >> 1) ^ ((0u - (c & 1)) & 0xedb88320u);
    }
    return ~c;
}

static std::string string_at(const Bytes &b, size_t at, size_t end)
{
    check(at < end && end <= b.size(), "Invalid string table");
    auto e = std::find(b.begin() + at, b.begin() + end, 0);
    check(e != b.begin() + end, "Unterminated string");
    return std::string(b.begin() + at, e);
}

static uint32_t symbol(const Bytes &b, const std::string &name)
{
    uint32_t sh = uint32_t(get(b, 32, 4));
    unsigned count = unsigned(get(b, 48, 2));
    check(get(b, 46, 2) == 40 && uint64_t(sh) + uint64_t(count) * 40 <= b.size(),
          "Invalid section table");
    for (unsigned i = 0; i < count; i++)
    {
        size_t s = sh + 40 * i;
        if (get(b, s + 4, 4) != 2)
            continue;
        uint32_t link = uint32_t(get(b, s + 24, 4));
        check(link < count && get(b, s + 36, 4) == 16, "Invalid symbol table");
        size_t st = sh + 40 * link, lo = size_t(get(b, st + 16, 4)),
               end = lo + size_t(get(b, st + 20, 4)), p = size_t(get(b, s + 16, 4)),
               size = size_t(get(b, s + 20, 4));
        check(p + size <= b.size(), "Symbol table outside file");
        for (size_t n = 0; n < size; n += 16)
            if (get(b, p + n, 4) && string_at(b, lo + size_t(get(b, p + n, 4)), end) == name)
                return uint32_t(get(b, p + n + 4, 4));
    }
    throw std::runtime_error("Missing symbol " + name);
}

static void package(const fs::path &input, const fs::path &output)
{
    Bytes b = read(input);
    check(b.size() >= 52 && get(b, 0, 4) == 0x464c457f && b[4] == 1 && b[5] == 1 &&
              get(b, 16, 2) == 2 && get(b, 18, 2) == 0xfe33 && get(b, 36, 4) == 1,
          "Expected R32 ELF ABI 1");
    uint32_t start = uint32_t(get(b, 24, 4)), end = symbol(b, "__bss_start"),
             memend = symbol(b, "__bss_end");
    check(start >= 0x10010000 && start < end && end <= memend && memend <= 0x13e00000 &&
              !(start & 3) && end - start <= 8 * 1024 * 1024,
          "Kernel load bounds");
    Bytes payload(end - start);
    uint32_t ph = uint32_t(get(b, 28, 4));
    unsigned count = unsigned(get(b, 44, 2));
    check(get(b, 42, 2) == 32 && uint64_t(ph) + uint64_t(count) * 32 <= b.size(),
          "Invalid program headers");
    for (unsigned i = 0; i < count; i++)
    {
        size_t p = ph + 32 * i;
        if (get(b, p, 4) != 1)
            continue;
        uint32_t off = uint32_t(get(b, p + 4, 4)), va = uint32_t(get(b, p + 8, 4)),
                 size = uint32_t(get(b, p + 16, 4));
        check(uint64_t(off) + size <= b.size() && uint64_t(va) + size <= uint64_t(UINT32_MAX) + 1,
              "Invalid load segment");
        uint64_t low = std::max(uint64_t(start), uint64_t(va)),
                 high = std::min(uint64_t(end), uint64_t(va) + size);
        if (high > low)
            std::copy_n(b.begin() + off + size_t(low - va), size_t(high - low),
                        payload.begin() + size_t(low - start));
    }
    Bytes out(32 + payload.size());
    uint32_t fields[] = {0x4b323352,     1,     start,          uint32_t(payload.size()),
                         memend - start, start, crc32(payload), 0};
    for (unsigned i = 0; i < 8; i++)
        put(out, i * 4, fields[i], 4);
    std::copy(payload.begin(), payload.end(), out.begin() + 32);
    write(output, out);
}

struct Node
{
    fs::path source;
    std::string name;
    bool directory;
    std::vector<size_t> children;
    uint32_t cluster = 0, size = 0;
    size_t parent = 0;
    std::string alias;
    bool long_name = false;
};

static std::string shortname(const std::string &name)
{
    if (name == "." || name == "..")
        return name + std::string(11 - name.size(), ' ');
    size_t dot = name.find_last_of('.');
    std::string stem = dot == std::string::npos ? name : name.substr(0, dot),
                ext = dot == std::string::npos ? "" : name.substr(dot + 1);
    check(!stem.empty() && stem.size() <= 8 && ext.size() <= 3,
          "FAT 8.3 filename required: " + name);
    auto upper = [](std::string s)
    {
        for (char &c : s)
        {
            check((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                      c == '_' || c == '-' || c == '~',
                  "Unsupported FAT filename");
            if (c >= 'a' && c <= 'z')
                c -= 32;
        }
        return s;
    };
    return upper(stem) + std::string(8 - stem.size(), ' ') + upper(ext) +
           std::string(3 - ext.size(), ' ');
}

static bool short_compatible(const std::string &name)
{
    size_t dot = name.find_last_of('.');
    std::string stem = dot == std::string::npos ? name : name.substr(0, dot),
                ext = dot == std::string::npos ? "" : name.substr(dot + 1);
    if (stem.empty() || stem.size() > 8 || ext.size() > 3)
        return false;
    for (char c : stem + ext)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '~'))
            return false;
    return true;
}

static std::string alias_name(const std::string &name, unsigned sequence)
{
    size_t dot = name.find_last_of('.');
    auto clean = [](std::string text)
    {
        std::string out;
        for (char c : text)
        {
            if (c >= 'a' && c <= 'z')
                c -= 32;
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                out += c;
        }
        return out;
    };
    std::string stem = clean(dot == std::string::npos ? name : name.substr(0, dot)),
                ext = dot == std::string::npos ? "" : clean(name.substr(dot + 1)),
                suffix = "~" + std::to_string(sequence);
    check(suffix.size() < 8, "FAT alias capacity exceeded");
    if (stem.empty())
        stem = "FILE";
    stem = stem.substr(0, 8 - suffix.size()) + suffix;
    return stem + std::string(8 - stem.size(), ' ') + ext.substr(0, 3) +
           std::string(3 - std::min(size_t(3), ext.size()), ' ');
}

static void fat(const fs::path &root, const fs::path &output, bool mbr, uint32_t mib = 34)
{
    check(fs::is_directory(root), "Root directory required");
    std::vector<Node> nodes = {{root, "", true, {}, 0, 0, 0}};
    for (size_t i = 0; i < nodes.size(); i++)
        if (nodes[i].directory)
        {
            std::vector<fs::directory_entry> entries;
            for (auto &e : fs::directory_iterator(nodes[i].source))
            {
                check(!e.is_symlink(), "Symlinks are not supported in the FAT image");
                entries.push_back(e);
            }
            std::sort(entries.begin(), entries.end(),
                      [](auto &a, auto &b)
                      {
                          return a.path().filename().string() < b.path().filename().string();
                      });
            std::map<std::string, bool> seen;
            std::map<std::string, bool> names;
            for (auto &e : entries)
            {
                std::string name = e.path().filename().string(), folded = name;
                check(!name.empty() && name.size() <= 255 && name.back() != '.' &&
                          name.back() != ' ',
                      "Invalid FAT filename");
                for (char &c : folded)
                {
                    check(c >= 32 && c < 127 &&
                              std::string("\"*/:<>?\\|").find(c) == std::string::npos,
                          "ASCII FAT filename required");
                    if (c >= 'A' && c <= 'Z')
                        c += 32;
                }
                check(!names[folded], "FAT name collision: " + name);
                names[folded] = true;
                bool long_name = !short_compatible(name);
                std::string shortn = long_name ? "" : shortname(name);
                if (shortn.empty() || seen[shortn])
                {
                    long_name = true;
                    unsigned sequence = 0;
                    do
                        shortn = alias_name(name, ++sequence);
                    while (seen[shortn]);
                }
                seen[shortn] = true;
                bool dir = e.is_directory();
                check(dir || e.is_regular_file(), "Unsupported root entry");
                uint64_t size = dir ? 0 : e.file_size();
                check(size <= UINT32_MAX, "FAT file exceeds 4 GiB");
                nodes[i].children.push_back(nodes.size());
                nodes.push_back({e.path(), name, dir, {}, 0, uint32_t(size), i});
                nodes.back().alias = shortn;
                nodes.back().long_name = long_name;
            }
        }
    const uint32_t sectors = mib * 1024 * 1024 / 512, reserved = 32;
    /* A FAT sector holds 128 entries. The two FAT copies also consume sectors. */
    const uint32_t divisor = 128 + 2;
    const uint32_t fatsectors = (sectors - reserved + 2 + divisor - 1) / divisor;
    const uint32_t clusters = sectors - reserved - 2 * fatsectors;
    check(clusters >= 65525, "FAT32 cluster count");
    uint32_t first = reserved + 2 * fatsectors, cursor = 2;
    Bytes volume(size_t(sectors) * 512), table(size_t(fatsectors) * 512);
    put(table, 0, 0x0ffffff8, 4);
    put(table, 4, 0xffffffff, 4);
    for (size_t i = 0; i < nodes.size(); i++)
    {
        auto &n = nodes[i];
        size_t slots = n.children.size() + 3;
        for (size_t child : n.children)
            if (nodes[child].long_name)
                slots += (nodes[child].name.size() + 12) / 13;
        uint32_t size = n.directory ? uint32_t(slots * 32) : n.size,
                 count = std::max(1u, (size + 511) / 512);
        check(uint64_t(cursor) + count <= uint64_t(clusters) + 2, "Root volume is full");
        n.cluster = cursor;
        for (uint32_t j = 0; j < count; j++)
            put(table, size_t(cursor + j) * 4, j + 1 < count ? cursor + j + 1 : 0x0fffffff, 4);
        cursor += count;
        if (!n.directory)
        {
            Bytes b = read(n.source);
            check(b.size() == n.size, "Root file changed during image construction");
            std::copy(b.begin(), b.end(), volume.begin() + size_t(first + n.cluster - 2) * 512);
        }
    }
    auto entry = [&](size_t at, const std::string &sn, uint32_t cluster, uint32_t size, bool dir)
    {
        std::copy(sn.begin(), sn.end(), volume.begin() + at);
        volume[at + 11] = dir ? 16 : 32;
        volume[at + 12] = 0x18;
        for (unsigned o : {16u, 18u, 24u})
            put(volume, at + o, 0x21, 2);
        put(volume, at + 20, cluster >> 16, 2);
        put(volume, at + 26, cluster & 65535, 2);
        put(volume, at + 28, size, 4);
    };
    for (size_t i = 0; i < nodes.size(); i++)
        if (nodes[i].directory)
        {
            auto &n = nodes[i];
            size_t at = size_t(first + n.cluster - 2) * 512;
            if (i)
            {
                entry(at, shortname("."), n.cluster, 0, true);
                entry(at + 32, shortname(".."), n.parent ? nodes[n.parent].cluster : 0, 0, true);
                at += 64;
            }
            else
            {
                std::string label = "LUMEAPPS   ";
                std::copy(label.begin(), label.end(), volume.begin() + at);
                volume[at + 11] = 8;
                at += 32;
            }
            for (size_t child : n.children)
            {
                auto &c = nodes[child];
                if (c.long_name)
                {
                    unsigned checksum = 0;
                    for (unsigned char value : c.alias)
                        checksum = (((checksum & 1) << 7) + (checksum >> 1) + value) & 255;
                    unsigned count = unsigned((c.name.size() + 12) / 13);
                    for (unsigned part = count; part; --part)
                    {
                        std::fill_n(volume.begin() + at, 32, 0xff);
                        volume[at] = uint8_t(part | (part == count ? 0x40 : 0));
                        volume[at + 11] = 0x0f;
                        volume[at + 12] = 0;
                        volume[at + 13] = uint8_t(checksum);
                        put(volume, at + 26, 0, 2);
                        unsigned word = 0;
                        for (unsigned start : {1u, 14u, 28u})
                            for (unsigned offset = start; offset < start + (start == 1    ? 10
                                                                            : start == 14 ? 12
                                                                                          : 4);
                                 offset += 2)
                            {
                                size_t index = (part - 1) * 13 + word++;
                                put(volume, at + offset,
                                    index < c.name.size()    ? uint16_t(c.name[index])
                                    : index == c.name.size() ? 0
                                                             : 0xffff,
                                    2);
                            }
                        at += 32;
                    }
                }
                entry(at, c.alias, c.cluster, c.size, c.directory);
                at += 32;
            }
        }
    const unsigned char jump[] = {0xeb, 0x58, 0x90, 'L', 'U', 'M', 'E', 'A', 'P', 'P', ' '};
    std::copy(std::begin(jump), std::end(jump), volume.begin());
    put(volume, 11, 512, 2);
    volume[13] = 1;
    put(volume, 14, reserved, 2);
    volume[16] = 2;
    volume[21] = 0xf8;
    put(volume, 24, 63, 2);
    put(volume, 26, 255, 2);
    put(volume, 28, mbr ? 2048 : 0, 4);
    put(volume, 32, sectors, 4);
    put(volume, 36, fatsectors, 4);
    put(volume, 44, nodes[0].cluster, 4);
    put(volume, 48, 1, 2);
    put(volume, 50, 6, 2);
    volume[64] = 0x80;
    volume[66] = 0x29;
    put(volume, 67, 0x4c554d45, 4);
    std::string label = "LUMEAPPS   ", type = "FAT32   ";
    std::copy(label.begin(), label.end(), volume.begin() + 71);
    std::copy(type.begin(), type.end(), volume.begin() + 82);
    put(volume, 510, 0xaa55, 2);
    put(volume, 512, 0x41615252, 4);
    put(volume, 512 + 484, 0x61417272, 4);
    put(volume, 512 + 488, clusters - (cursor - 2), 4);
    put(volume, 512 + 492, cursor, 4);
    put(volume, 512 + 508, 0xaa550000, 4);
    std::copy_n(volume.begin(), 512, volume.begin() + 6 * 512);
    std::copy_n(volume.begin() + 512, 512, volume.begin() + 7 * 512);
    for (unsigned i = 0; i < 2; i++)
        std::copy(table.begin(), table.end(),
                  volume.begin() + size_t(reserved + i * fatsectors) * 512);
    if (mbr)
    {
        Bytes image(2048 * 512 + volume.size());
        put(image, 446, 0x80, 1);
        image[450] = 0x0c;
        put(image, 454, 2048, 4);
        put(image, 458, sectors, 4);
        put(image, 510, 0xaa55, 2);
        std::copy(volume.begin(), volume.end(), image.begin() + 2048 * 512);
        write(output, image);
    }
    else
        write(output, volume);
}

int main(int argc, char **argv)
{
    try
    {
        check(argc >= 2, "Usage: image-tool {fat|disk|r32-kernel|alias} INPUT OUTPUT [VOLUME_MIB]");
        std::string op = argv[1];
        check(argc == 4 || argc == 5, "Input and output required");
        uint32_t mib = 34;
        if (argc == 5)
        {
            check(op == "fat" || op == "disk", "Volume size only applies to FAT images");
            std::string value = argv[4];
            check(!value.empty() && value.find_first_not_of("0123456789") == std::string::npos,
                  "Invalid volume size in MiB");
            uint64_t size = std::stoull(value);
            check(size >= 34 && size <= 1024, "Volume size must be between 34 and 1024 MiB");
            mib = uint32_t(size);
        }
        if (op == "fat" || op == "disk")
            fat(argv[2], argv[3], op == "disk", mib);
        else if (op == "r32-kernel")
            package(argv[2], argv[3]);
        else if (op == "alias")
        {
            Bytes b = {'L', 'U', 'M', 'E', 'A', 'L', '1', 0};
            std::string path = argv[2];
            check(path.size() < 128 && path.front() == '/',
                  "Alias target must be an absolute target pathname");
            b.insert(b.end(), path.begin(), path.end());
            b.push_back(0);
            write(argv[3], b);
        }
        else
            throw std::runtime_error("Unknown image operation");
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "image-tool: " << e.what() << '\n';
        return 1;
    }
}
