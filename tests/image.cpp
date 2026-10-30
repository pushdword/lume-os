#define main image_main
#include "../tools/image.cpp"
#undef main

static int invoke(std::vector<std::string> args)
{
    std::vector<char *> pointers;
    for (auto &arg : args)
        pointers.push_back(arg.data());
    return image_main(int(pointers.size()), pointers.data());
}

int main(int argc, char **argv)
{
    try
    {
        check(argc == 2, "Test output directory required");
        fs::path output = argv[1], root = output / "root";
        fs::create_directories(root);
        fs::remove(root / "large.bin");
        write(root / "data.bin", Bytes{1, 3, 5, 7, 9});
        check(invoke({"image-tool", "fat", root.string(), (output / "default.fat").string()}) == 0,
              "Default FAT generation failed");
        check(fs::file_size(output / "default.fat") == 34u * 1024 * 1024,
              "Default volume size changed");
        check(invoke({"image-tool", "fat", root.string(), (output / "explicit.fat").string(),
                      "34"}) == 0,
              "Explicit default FAT generation failed");
        check(read(output / "default.fat") == read(output / "explicit.fat"),
              "Explicit size changes the default image");
        check(invoke({"image-tool", "disk", root.string(), (output / "data64.img").string(),
                      "64"}) == 0,
              "64 MiB FAT generation failed");
        {
            Bytes image = read(output / "data64.img");
            check(image.size() == 65u * 1024 * 1024, "64 MiB partition extent differs");
            size_t bpb = 2048u * 512;
            uint32_t fatsectors = uint32_t(image[bpb + 36]) | uint32_t(image[bpb + 37]) << 8;
            check(fatsectors == 1009, "64 MiB FAT capacity differs");
            uint32_t clusters = 64u * 2048 - 32 - 2 * fatsectors;
            check(clusters + 2 <= fatsectors * 128, "FAT cannot describe every data cluster");
        }
        {
            std::ofstream large(root / "large.bin", std::ios::binary);
            large.seekp(36u * 1024 * 1024 - 1);
            large.put(0);
            check(bool(large), "Large fixture creation failed");
        }
        check(invoke({"image-tool", "fat", root.string(), (output / "full.fat").string()}) == 1,
              "An overflowing root was accepted");
        check(invoke({"image-tool", "disk", root.string(), (output / "sdk.img").string(), "127"}) ==
                  0,
              "Larger root volume generation failed");
        check(fs::file_size(output / "sdk.img") == 128u * 1024 * 1024,
              "Partition prefix or native SDK disk size differs");
        for (const char *size : {"0", "33", "1025", "127x", "-1"})
            check(invoke({"image-tool", "disk", root.string(), (output / "invalid.img").string(),
                          size}) == 1,
                  "Invalid volume size accepted");
        fs::path long_root = output / "long-root";
        fs::create_directories(long_root);
        write(long_root / "bcm2712-rpi-5-b.dtb", Bytes{1, 2, 3});
        write(long_root / "bcm2712-rpi-5-c.dtb", Bytes{4, 5, 6});
        write(long_root / "LICENSE", Bytes{7});
        check(invoke({"image-tool", "fat", long_root.string(), (output / "long.fat").string()}) ==
                  0,
              "Long FAT filename generation failed");
        {
            Bytes image = read(output / "long.fat");
            size_t directory = (32 + 2 * size_t(get(image, 36, 4))) * 512;
            unsigned long_entries = 0, short_entries = 0;
            std::map<std::string, bool> aliases;
            for (size_t at = directory + 32; image[at]; at += 32)
            {
                if (image[at + 11] == 0x0f)
                    ++long_entries;
                else
                {
                    std::string alias(image.begin() + at, image.begin() + at + 11);
                    check(!aliases[alias], "Long FAT aliases collide");
                    aliases[alias] = true;
                    ++short_entries;
                }
            }
            check(long_entries == 5 && short_entries == 3, "Long FAT entries differ");
        }
        std::cout << "PASS FAT default compatibility, capacity, partition size and rejection\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "image-test: " << error.what() << '\n';
        return 1;
    }
}
