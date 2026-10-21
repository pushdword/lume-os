// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Native Clang frontend, integrated assembler and ELF linker.
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/CodeGenOptions.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/FrontendTool/Utils.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Job.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TimeProfiler.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "lld/Common/Driver.h"
#include "api.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>

LLD_HAS_DRIVER(elf)
int cc1as_main(llvm::ArrayRef<const char *>, const char *, void *);

static void memoryStats(const char *stage)
{
    if (!std::getenv("LUME_CC_STATS"))
        return;
    lume_memory_info info{};
    if (lume_syscall(LUME_MEMORY, reinterpret_cast<uintptr_t>(&info), sizeof(info),
                     LUME_MEMORY_VERSION))
        return;
    llvm::errs() << "LUME_CC_MEMORY stage=" << stage
                 << " heap_bytes=" << (info.heap_end - info.heap_base)
                 << " process_pages=" << info.process_pages << " free_pages=" << info.free_pages
                 << "\n";
}

static bool loadHeaders(llvm::vfs::InMemoryFileSystem &fs)
{
    auto buffer = llvm::MemoryBuffer::getFile("" LUME_NATIVE_PREFIX "/hdrs.bin");
    if (!buffer)
    {
        llvm::errs() << "clang: missing header package: " << buffer.getError().message() << "\n";
        return false;
    }
    llvm::StringRef bytes = (*buffer)->getBuffer();
    if (!bytes.consume_front("LUMEHDR1"))
    {
        llvm::errs() << "clang: invalid header package\n";
        return false;
    }
    while (!bytes.empty())
    {
        if (bytes.size() < 6)
            return false;
        const auto *p = reinterpret_cast<const unsigned char *>(bytes.data());
        unsigned names = p[0] | (unsigned(p[1]) << 8);
        uint32_t count =
            p[2] | (uint32_t(p[3]) << 8) | (uint32_t(p[4]) << 16) | (uint32_t(p[5]) << 24);
        bytes = bytes.drop_front(6);
        if (!names || names > 512 || count > bytes.size() || names > bytes.size() - count)
            return false;
        auto name = bytes.take_front(names);
        bytes = bytes.drop_front(names);
        auto contents = bytes.take_front(count);
        bytes = bytes.drop_front(count);
        if (!name.starts_with("" LUME_NATIVE_PREFIX "/") || name.contains("..") ||
            name.contains('\0'))
            return false;
        if (!fs.addFile(name, 0, llvm::MemoryBuffer::getMemBufferCopy(contents, name)))
            return false;
    }
    return true;
}

static int compile(llvm::ArrayRef<const char *> args,
                   llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs)
{
    llvm::cl::ResetAllOptionOccurrences();
    clang::DiagnosticOptions diagnosticOptions;
    clang::DiagnosticsEngine diagnostics(
        clang::DiagnosticIDs::create(), diagnosticOptions,
        new clang::TextDiagnosticPrinter(llvm::errs(), diagnosticOptions));
    auto invocation = std::make_shared<clang::CompilerInvocation>();
    llvm::SmallVector<const char *, 32> invocationArgs;
    invocationArgs.push_back("-source-date-epoch");
    invocationArgs.push_back(std::getenv("SOURCE_DATE_EPOCH"));
    invocationArgs.append(args.begin(), args.end());
    // Direct frontend invocations use the same argument validation as driver jobs.
    // A later option overrides an earlier value.
    if (!clang::CompilerInvocation::CreateFromArgs(*invocation, invocationArgs, diagnostics))
        return 1;
    // Driver cc1 jobs normally leave these allocations to process exit. Here
    // several frontend jobs run in one process, so each must release its AST/IR.
    invocation->getFrontendOpts().DisableFree = false;
    invocation->getCodeGenOpts().DisableFree = false;
    clang::CompilerInstance compiler(std::move(invocation));
    compiler.createVirtualFileSystem(fs);
    compiler.createDiagnostics();
    auto &frontend = compiler.getFrontendOpts();
    bool trace = !frontend.TimeTracePath.empty();
    if (trace)
        llvm::timeTraceProfilerInitialize(frontend.TimeTraceGranularity, "clang",
                                          frontend.TimeTraceVerbose);
    bool success = clang::ExecuteCompilerInvocation(&compiler);
    memoryStats("frontend");
    llvm::TimerGroup::printAll(llvm::errs());
    llvm::TimerGroup::clearAll();
    if (trace)
    {
        if (auto output = compiler.createOutputFile(frontend.TimeTracePath, false, false, false))
        {
            llvm::timeTraceProfilerWrite(*output);
            output.reset();
            compiler.clearOutputFiles(false);
        }
        else
            success = false;
        llvm::timeTraceProfilerCleanup();
    }
    return success ? 0 : 1;
}

#if defined(LUME_CC1_ENTRY)
extern "C" int app_main(unsigned argc, char **argv, char **)
{
    if (!std::getenv("SOURCE_DATE_EPOCH") && ::setenv("SOURCE_DATE_EPOCH", "0", 0))
        return 1;
    static bool initialized;
    if (!initialized)
    {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        initialized = true;
    }
    if (argc > 1 && llvm::StringRef(argv[1]) == "--version")
    {
        llvm::outs() << "Lume native Clang cc1 23.1.3 / AArch64\n";
        return 0;
    }
    llvm::BumpPtrAllocator allocation;
    llvm::StringSaver saver(allocation);
    llvm::SmallVector<const char *, 32> arguments;
    for (unsigned i = 0; i < argc; ++i)
        arguments.push_back(argv[i]);
    if (!llvm::cl::ExpandResponseFiles(saver, llvm::cl::TokenizeGNUCommandLine, arguments))
        return 1;
    auto args = llvm::ArrayRef(arguments).drop_front();
    if (!args.empty() && llvm::StringRef(args[0]) == "-cc1as")
        return cc1as_main(args.drop_front(), argv[0], nullptr);
    if (!args.empty() && llvm::StringRef(args[0]) == "-cc1")
        args = args.drop_front();
    auto headers = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
    if (!loadHeaders(*headers))
        return 1;
    auto fs =
        llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(llvm::vfs::getRealFileSystem());
    fs->pushOverlay(headers);
    return compile(args, fs);
}
#else
extern "C" int app_main(unsigned argc, char **argv, char **)
{
    // Lume has no wall-clock service yet. Use Clang's documented reproducible
    // build epoch rather than presenting uptime or a host timestamp as RTC time.
    if (!std::getenv("SOURCE_DATE_EPOCH") && ::setenv("SOURCE_DATE_EPOCH", "0", 0))
    {
        llvm::errs() << "clang: cannot configure reproducible epoch\n";
        return 1;
    }
    static bool initialized;
    if (!initialized)
    {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        initialized = true;
    }
    if (argc == 3 && llvm::StringRef(argv[1]) == "--batch")
    {
        auto input = llvm::MemoryBuffer::getFile(argv[2]);
        if (!input || (*input)->getBufferSize() > 1024 * 1024)
        {
            llvm::errs() << "clang: cannot read response-file list\n";
            return 1;
        }
        llvm::SmallVector<llvm::StringRef, 128> lines;
        (*input)->getBuffer().split(lines, '\n');
        unsigned jobs = 0;
        for (auto line : lines)
        {
            line = line.trim();
            if (line.empty())
                continue;
            if (++jobs > 4096 || line.contains('\0'))
            {
                llvm::errs() << "clang: invalid response-file list\n";
                return 1;
            }
            std::string response = ("@" + line).str();
            char *arguments[] = {argv[0], response.data(), nullptr};
            llvm::outs() << "clang: job " << jobs << " " << line << "\n";
            llvm::outs().flush();
            int status = app_main(2, arguments, nullptr);
            if (status)
                return status;
        }
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--version")
    {
        llvm::outs() << "Lume native Clang 23.1.3 / AArch64\n";
        return 0;
    }
    llvm::BumpPtrAllocator allocation;
    llvm::StringSaver saver(allocation);
    llvm::SmallVector<const char *, 32> userArgs;
    userArgs.push_back("/usr/bin/clang");
    for (unsigned i = 1; i < argc; ++i)
        userArgs.push_back(argv[i]);
    if (!llvm::cl::ExpandResponseFiles(saver, llvm::cl::TokenizeGNUCommandLine, userArgs))
        return 1;
    bool physicalHeadersOnly = false;
    for (const char *argument : userArgs)
        if (llvm::StringRef(argument) == "-nostdinc")
            physicalHeadersOnly = true;
    auto fs =
        llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(llvm::vfs::getRealFileSystem());
    if (!physicalHeadersOnly)
    {
        auto headers = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
        if (!loadHeaders(*headers))
            return 1;
        fs->pushOverlay(headers);
        memoryStats("headers");
    }
    if (userArgs.size() > 1 && llvm::StringRef(userArgs[1]) == "-cc1")
        return compile(llvm::ArrayRef(userArgs).drop_front(2), fs);
    llvm::SmallVector<const char *, 32> args;
    args.push_back(userArgs[0]);
    args.push_back("--target=aarch64-unknown-none-elf");
    args.push_back("--sysroot=" LUME_NATIVE_PREFIX "");
    args.push_back("-resource-dir=" LUME_NATIVE_PREFIX "/res");
    args.push_back("-isystem");
    args.push_back(LUME_NATIVE_PREFIX "/include/posix");
    // Temporary filenames fit the filesystem component limit.
    // The frontend runs in this process unless the caller saves intermediate files.
    args.push_back("-fno-stack-protector");
    args.push_back("-D__lume__=1");
    args.push_back("-DLUME_ARCH_AARCH64=1");
    args.push_back("-D_POSIX_MONOTONIC_CLOCK=200809L");
    args.push_back("-nostdlib");
    args.push_back("-static");
    args.push_back("-fno-pic");
    args.push_back("-fno-pie");
    bool links = true, printOnly = false, runtime = true, startFiles = true,
         defaultLibraries = true;
    for (unsigned i = 1; i < userArgs.size(); ++i)
    {
        auto a = llvm::StringRef(userArgs[i]);
        if (a == "-c" || a == "-S" || a == "-E" || a == "-fsyntax-only")
            links = false;
        if (a == "-###")
            printOnly = true;
        if (a == "-nostdlib")
            runtime = false;
        if (a == "-nostartfiles")
            startFiles = false;
        if (a == "-nodefaultlibs")
            defaultLibraries = false;
        args.push_back(userArgs[i]);
    }
    if (links)
    {
        args.push_back("-Wl,--no-mmap-output-file,--threads=1");
        if (runtime)
        {
            args.push_back("-Wl,-T," LUME_NATIVE_PREFIX
                           "/native.ld,--gc-sections,-z,max-page-size=4096");
            if (startFiles)
                args.push_back("" LUME_NATIVE_PREFIX "/lib/crt0.o");
            if (defaultLibraries)
            {
                args.push_back("-L" LUME_NATIVE_PREFIX "/lib");
                args.push_back("-Wl,--start-group");
                args.push_back("-llume");
                args.push_back("-lc++");
                args.push_back("-lc");
                args.push_back("-lcrt");
                args.push_back("-Wl,--end-group");
            }
        }
    }
    clang::DiagnosticOptions diagnosticOptions;
    clang::DiagnosticsEngine diagnostics(
        clang::DiagnosticIDs::create(), diagnosticOptions,
        new clang::TextDiagnosticPrinter(llvm::errs(), diagnosticOptions));
    clang::driver::Driver driver(userArgs[0], "aarch64-unknown-none-elf", diagnostics,
                                 "Lume native Clang", fs);
    driver.ResourceDir = "" LUME_NATIVE_PREFIX "/res";
    auto jobs = std::unique_ptr<clang::driver::Compilation>(driver.BuildCompilation(args));
    if (!jobs || diagnostics.hasErrorOccurred())
        return 1;
    if (printOnly)
    {
        jobs->getJobs().Print(llvm::errs(), "\n", true);
        return 0;
    }
    for (const auto &job : jobs->getJobs())
    {
        const auto &arguments = job.getArguments();
        if (!arguments.empty() && llvm::StringRef(arguments[0]) == "-cc1")
        {
            if (int result = compile(llvm::ArrayRef(arguments).drop_front(), fs))
                return result;
        }
        else if (!arguments.empty() && llvm::StringRef(arguments[0]) == "-cc1as")
        {
            llvm::cl::ResetAllOptionOccurrences();
            if (int result =
                    cc1as_main(llvm::ArrayRef(arguments).drop_front(), userArgs[0], nullptr))
                return result;
        }
        else
        {
            llvm::SmallVector<const char *, 32> linkArgs;
            linkArgs.push_back("ld.lld");
            linkArgs.append(arguments.begin(), arguments.end());
            auto result =
                lld::lldMain(linkArgs, llvm::outs(), llvm::errs(), {{lld::Gnu, &lld::elf::link}});
            if (result.retCode)
                return result.retCode;
        }
        memoryStats("job-done");
    }
    return 0;
}
#endif
