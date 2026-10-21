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
        std::cout << "PASS FAT default compatibility, capacity, partition size and rejection\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "image-test: " << error.what() << '\n';
        return 1;
    }
}
