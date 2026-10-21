// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Lume has synchronous file descriptors, no symlinks or mmap. Unsupported
// operations fail explicitly. The Clang driver uses an overlay for headers.
#include "llvm/Support/Errno.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "errno_map.h"
#include "api.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
extern "C" int32_t lume_syscall(uint32_t, uint32_t, uint32_t, uint32_t);

namespace llvm
{
namespace sys
{
namespace fs
{
const file_t kInvalidFile = -1;

static std::error_code unsupported()
{
    return errc::function_not_supported;
}

static std::error_code ioresult(int R)
{
    return R < 0 ? errnoAsErrorCode() : std::error_code();
}

// Filesystem component limits apply to physical files. Read-only VFS
// probes must report absence so __has_include_next can continue normally.
// Creation and rename retain the honest ENAMETOOLONG error from libc.
static std::error_code readonlyPathError()
{
    return errno == ENAMETOOLONG ? make_error_code(errc::no_such_file_or_directory)
                                 : errnoAsErrorCode();
}

static std::string &fdname(unsigned FD)
{
    static std::string Names[LUME_FDS];
    return Names[FD < LUME_FDS ? FD : 0];
}

static SmallString<256> canonicalName(StringRef S)
{
    SmallString<256> Canonical(S);
    if (path::is_relative(Canonical))
    {
        char Cwd[256];
        if (::getcwd(Cwd, sizeof Cwd))
        {
            SmallString<256> Absolute(Cwd);
            path::append(Absolute, Canonical);
            Canonical = Absolute;
        }
    }
    path::remove_dots(Canonical, true);
    while (Canonical.size() > 1 && Canonical.back() == '/')
        Canonical.pop_back();
    return Canonical;
}

static uint64_t pathID(StringRef S)
{
    auto Canonical = canonicalName(S);
    uint64_t H = 1469598103934665603ULL;
    for (unsigned char C : Canonical)
    {
        if (C >= 'a' && C <= 'z')
            C -= ('a' - 'A');
        H ^= C;
        H *= 1099511628211ULL;
    }
    return H;
}

std::string getMainExecutable(const char *Argv0, void *)
{
    return Argv0 ? Argv0 : "/usr/bin/clang";
}

TimePoint<> basic_file_status::getLastAccessedTime() const
{
    return TimePoint<>(std::chrono::seconds(fs_st_atime));
}

TimePoint<> basic_file_status::getLastModificationTime() const
{
    return TimePoint<>(std::chrono::seconds(fs_st_mtime));
}

UniqueID file_status::getUniqueID() const
{
    return UniqueID(fs_st_dev, fs_st_ino);
}

uint32_t file_status::getLinkCount() const
{
    return fs_st_nlinks;
}

ErrorOr<space_info> disk_space(const Twine &)
{
    return unsupported();
}

std::error_code current_path(SmallVectorImpl<char> &Out)
{
    char B[256];
    if (!::getcwd(B, sizeof B))
        return errnoAsErrorCode();
    Out.assign(B, B + strlen(B));
    return {};
}

std::error_code set_current_path(const Twine &P)
{
    return ioresult(::chdir(P.str().c_str()));
}

std::error_code create_directory(const Twine &P, bool Ignore, perms)
{
    if (::mkdir(P.str().c_str(), 0777) == 0)
        return {};
    if (Ignore && errno == EEXIST)
        return {};
    return errnoAsErrorCode();
}

std::error_code create_symlink(const Twine &, const Twine &)
{
    return unsupported();
}

std::error_code create_link(const Twine &, const Twine &)
{
    return unsupported();
}

std::error_code create_hard_link(const Twine &, const Twine &)
{
    return unsupported();
}

std::error_code remove(const Twine &P, bool Ignore)
{
    std::string N = P.str();
    if (::unlink(N.c_str()) == 0 || ::rmdir(N.c_str()) == 0)
        return {};
    if (Ignore && errno == ENOENT)
        return {};
    return errnoAsErrorCode();
}

std::error_code is_local(const Twine &, bool &R)
{
    R = true;
    return {};
}

std::error_code is_local(int, bool &R)
{
    R = true;
    return {};
}

std::error_code rename(const Twine &A, const Twine &B)
{
    std::string Source = A.str(), Target = B.str();
    if (StringRef(canonicalName(Source)).equals_insensitive(canonicalName(Target)))
    {
        struct stat Same;
        return ioresult(::stat(Source.c_str(), &Same));
    }
    if (::rename(Source.c_str(), Target.c_str()) == 0)
        return {};
    auto First = errnoAsErrorCode();
    if (errno != EEXIST)
        return First;
    // FatFs cannot atomically replace a destination. Preserve its old bytes by
    // moving it aside before publish, and restore it if publication fails.
    // This protocol is not crash-atomic and does not promise POSIX concurrency.
    struct stat Src, Dst;
    if (::stat(Source.c_str(), &Src) || ::stat(Target.c_str(), &Dst))
        return errnoAsErrorCode();
    if (!S_ISREG(Src.st_mode) || !S_ISREG(Dst.st_mode))
        return First;
    SmallString<256> Model(Target), Backup;
    path::remove_filename(Model);
    path::append(Model, "b%%%%%%.bak");
    int FD;
    if (auto E = createUniqueFile(Model, FD, Backup))
        return E;
    if (::close(FD))
        return errnoAsErrorCode();
    if (::unlink(Backup.c_str()))
        return errnoAsErrorCode();
    if (::rename(Target.c_str(), Backup.c_str()))
        return errnoAsErrorCode();
    if (::rename(Source.c_str(), Target.c_str()))
    {
        auto Publish = errnoAsErrorCode();
        if (::rename(Backup.c_str(), Target.c_str()))
            errs() << "Lume: output publication failed. Original preserved at " << Backup << "\n";
        return Publish;
    }
    if (::unlink(Backup.c_str()))
        errs() << "Lume: output published. Old backup retained at " << Backup << "\n";
    return {};
}

std::error_code resize_file(int FD, uint64_t Size)
{
    if (Size > INT32_MAX)
        return errc::file_too_large;
    struct stat S;
    if (::fstat(FD, &S))
        return errnoAsErrorCode();
    off_t Old = ::lseek(FD, 0, SEEK_CUR);
    if (Old < 0)
        return errnoAsErrorCode();
    // An offset beyond the new EOF cannot be restored. Reject the resize before writing.
    if ((uint64_t)Old > Size)
        return unsupported();
    if (Size == (uint64_t)S.st_size)
        return {};
    std::error_code Work = [&]() -> std::error_code
    {
        if (Size > (uint64_t)S.st_size)
        {
            char Zero[512] = {};
            if (::lseek(FD, 0, SEEK_END) < 0)
                return errnoAsErrorCode();
            uint64_t Left = Size - S.st_size;
            while (Left)
            {
                size_t N = std::min<uint64_t>(Left, sizeof Zero);
                ssize_t W = ::write(FD, Zero, N);
                if (W <= 0)
                    return W < 0 ? errnoAsErrorCode() : make_error_code(errc::io_error);
                Left -= W;
            }
        }
        else
        {
            if (::lseek(FD, Size, SEEK_SET) < 0)
                return errnoAsErrorCode();
            int R = lume_syscall(31, FD, 0, 0);
            if (R < 0)
                return std::error_code(lume_errno_from_syscall((unsigned)-R),
                                       std::generic_category());
        }
        return {};
    }();
    auto Restore = ioresult(::lseek(FD, Old, SEEK_SET));
    return Work ? Work : Restore;
}

std::error_code resize_file_sparse(int FD, uint64_t S)
{
    return resize_file(FD, S);
}

static file_type typeForMode(mode_t M)
{
    return S_ISDIR(M)   ? file_type::directory_file
           : S_ISREG(M) ? file_type::regular_file
                        : file_type::type_unknown;
}

static void fromStat(const struct stat &S, StringRef Name, file_status &R)
{
    R = file_status(typeForMode(S.st_mode), perms(S.st_mode & 0777), 1, 1, pathID(Name), 0, 0, 0, 0,
                    S.st_uid, S.st_gid, S.st_size);
}

std::error_code status(const Twine &P, file_status &R, bool)
{
    struct stat S;
    std::string Name = P.str();
    if (::stat(Name.c_str(), &S))
    {
        auto E = readonlyPathError();
        R = file_status(E == errc::no_such_file_or_directory ? file_type::file_not_found
                                                             : file_type::status_error);
        return E;
    }
    fromStat(S, Name, R);
    return {};
}

std::error_code status(int FD, file_status &R)
{
    struct stat S;
    if (::fstat(FD, &S))
    {
        R = file_status(file_type::status_error);
        return errnoAsErrorCode();
    }
    fromStat(S, fdname(FD), R);
    return {};
}

std::error_code access(const Twine &P, AccessMode M)
{
    file_status S;
    if (auto E = status(P, S, true))
        return E;
    if (M == AccessMode::Write && !(S.permissions() & owner_write))
        return errc::permission_denied;
    return {};
}

bool can_execute(const Twine &P)
{
    file_status S;
    return !status(P, S, true) && S.type() == file_type::regular_file;
}

bool equivalent(file_status A, file_status B)
{
    return A.getUniqueID() == B.getUniqueID();
}

std::error_code equivalent(const Twine &A, const Twine &B, bool &R)
{
    file_status X
    , Y;
    if (auto E = status(A, X, true))
        return E;
    if (auto E = status(B, Y, true))
        return E;
    R = equivalent(X, Y);
    return {};
}

void expand_tilde(const Twine &P, SmallVectorImpl<char> &D)
{
    std::string S = P.str();
    if (!S.empty() && S[0] == '~' && (S.size() == 1 || S[1] == '/'))
    {
        const char *H = ::getenv("HOME");
        if (H)
            S = std::string(H) + S.substr(1);
    }
    D.assign(S.begin(), S.end());
}

unsigned getUmask()
{
    return 0;
}

std::error_code setPermissions(const Twine &P, perms V)
{
    file_status S;
    if (auto E = status(P, S, true))
        return E;
    return S.permissions() == V ? std::error_code() : unsupported();
}

std::error_code setPermissions(int FD, perms V)
{
    file_status S;
    if (auto E = status(FD, S))
        return E;
    return S.permissions() == V ? std::error_code() : unsupported();
}

std::error_code setLastAccessAndModificationTime(int, TimePoint<>, TimePoint<>)
{
    return unsupported();
}

std::error_code mapped_file_region::init(int, uint64_t, mapmode, const char *)
{
    return unsupported();
}

mapped_file_region::mapped_file_region(int, mapmode M, size_t N, uint64_t, std::error_code &E,
                                       const char *)
    : Size(N), Mode(M)
{
    E = unsupported();
    copyFrom(mapped_file_region());
}

void mapped_file_region::unmapImpl()
{
}

std::error_code mapped_file_region::sync() const
{
    return unsupported();
}

void mapped_file_region::dontNeedImpl()
{
}

void mapped_file_region::willNeedImpl()
{
}

void mapped_file_region::randomAccessImpl()
{
}

int mapped_file_region::alignment()
{
    return 4096;
}

struct DirWalk
{
    std::string Path;
    unsigned Index;
};

using LumeDirent = lume_directory_entry;

std::error_code detail::directory_iterator_construct(detail::DirIterState &I, StringRef P,
                                                     bool Follow)
{
    auto *W = new DirWalk{P.str(), 0};
    I.IterationHandle = reinterpret_cast<intptr_t>(W);
    SmallString<128> S(P);
    path::append(S, ".");
    I.CurrentEntry = directory_entry(S, Follow);
    return directory_iterator_increment(I);
}

std::error_code detail::directory_iterator_destruct(detail::DirIterState &I)
{
    delete reinterpret_cast<DirWalk *>(I.IterationHandle);
    I.IterationHandle = 0;
    I.CurrentEntry = directory_entry();
    return {};
}

std::error_code detail::directory_iterator_increment(detail::DirIterState &I)
{
    auto *W = reinterpret_cast<DirWalk *>(I.IterationHandle);
    if (!W)
        return {};
    LumeDirent D;
    int R = lume_syscall(11, reinterpret_cast<uintptr_t>(W->Path.c_str()),
                         reinterpret_cast<uintptr_t>(&D), W->Index);
    if (R < 0)
        return std::error_code(lume_errno_from_syscall((unsigned)-R), std::generic_category());
    if (!R)
        return directory_iterator_destruct(I);
    W->Index = (unsigned)R;
    I.CurrentEntry.replace_filename(D.name, typeForMode(D.mode));
    return {};
}

ErrorOr<basic_file_status> directory_entry::status() const
{
    file_status S;
    if (auto E = fs::status(Path, S, FollowSymlinks))
        return E;
    return S;
}

std::error_code openFile(const Twine &N, int &FD, CreationDisposition D, FileAccess A, OpenFlags F,
                         unsigned Mode)
{
    int Flags = A == FA_Read ? O_RDONLY : A == FA_Write ? O_WRONLY : O_RDWR;
    switch (D)
    {
    case CD_CreateAlways:
        Flags |= O_CREAT | O_TRUNC;
        break;
    case CD_CreateNew:
        Flags |= O_CREAT | O_EXCL;
        break;
    case CD_OpenAlways:
        Flags |= O_CREAT;
        break;
    case CD_OpenExisting:
        break;
    }
    if (F & OF_Append)
        Flags |= O_APPEND;
    FD = ::open(N.str().c_str(), Flags, Mode);
    if (FD < 0)
        return A == FA_Read && D == CD_OpenExisting ? readonlyPathError() : errnoAsErrorCode();
    fdname(FD) = canonicalName(N.str()).str().str();
    return {};
}

Expected<int> openNativeFile(const Twine &N, CreationDisposition D, FileAccess A, OpenFlags F,
                             unsigned M)
{
    int FD;
    if (auto E = openFile(N, FD, D, A, F, M))
        return errorCodeToError(E);
    return FD;
}

std::error_code openFileForRead(const Twine &N, int &FD, OpenFlags F, SmallVectorImpl<char> *Real)
{
    if (auto E = openFile(N, FD, CD_OpenExisting, FA_Read, F, 0))
        return E;
    if (Real)
    {
        std::string S = N.str();
        Real->assign(S.begin(), S.end());
    }
    return {};
}

Expected<file_t> openNativeFileForRead(const Twine &N, OpenFlags F, SmallVectorImpl<char> *R)
{
    int FD;
    if (auto E = openFileForRead(N, FD, F, R))
        return errorCodeToError(E);
    return FD;
}

file_t getStdinHandle()
{
    return 0;
}

file_t getStdoutHandle()
{
    return 1;
}

file_t getStderrHandle()
{
    return 2;
}

Expected<size_t> readNativeFile(file_t FD, MutableArrayRef<char> B)
{
    ssize_t N;
    do
    {
        N = ::read(FD, B.data(), B.size());
    } while (N < 0 && errno == EINTR);
    if (N < 0)
        return errorCodeToError(errnoAsErrorCode());
    return (size_t)N;
}

Expected<size_t> readNativeFileSlice(file_t FD, MutableArrayRef<char> B, uint64_t O)
{
    if (O > INT32_MAX)
        return errorCodeToError(make_error_code(errc::file_too_large));
    struct stat S;
    if (::fstat(FD, &S))
        return errorCodeToError(errnoAsErrorCode());
    if (O >= (uint64_t)S.st_size)
        return size_t(0);
    off_t Save = ::lseek(FD, 0, SEEK_CUR);
    if (Save < 0)
        return errorCodeToError(errnoAsErrorCode());
    if (::lseek(FD, O, SEEK_SET) < 0)
        return errorCodeToError(errnoAsErrorCode());
    auto R = readNativeFile(FD, B);
    auto Restore = ioresult(::lseek(FD, Save, SEEK_SET));
    if (!R)
        return R;
    if (Restore)
        return errorCodeToError(Restore);
    return R;
}

std::error_code tryLockFile(int, std::chrono::milliseconds, LockKind)
{
    return unsupported();
}

std::error_code lockFile(int, LockKind)
{
    return unsupported();
}

std::error_code unlockFile(int)
{
    return unsupported();
}

std::error_code closeFile(file_t &F)
{
    int Old = F;
    F = kInvalidFile;
    auto E = ioresult(::close(Old));
    if (Old >= 0 && Old < 8)
        fdname(Old).clear();
    return E;
}

std::error_code remove_directories(const Twine &P, bool Ignore)
{
    return remove(P, Ignore);
}

std::error_code real_path(const Twine &P, SmallVectorImpl<char> &D, bool Expand)
{
    SmallString<256> S;
    if (Expand)
        expand_tilde(P, S);
    else
        P.toVector(S);
    if (path::is_relative(S))
    {
        SmallString<256> C;
        if (auto E = current_path(C))
            return E;
        path::append(C, S);
        S = C;
    }
    path::remove_dots(S, true);
    file_status St;
    if (auto E = status(S, St, true))
        return E;
    D.assign(S.begin(), S.end());
    return {};
}

std::error_code readlink(const Twine &, SmallVectorImpl<char> &)
{
    return unsupported();
}

std::error_code changeFileOwnership(int, uint32_t, uint32_t)
{
    return unsupported();
}
}

namespace path
{
bool home_directory(SmallVectorImpl<char> &R)
{
    const char *H = ::getenv("HOME");
    if (!H)
        return false;
    R.assign(H, H + strlen(H));
    return true;
}

bool user_config_directory(SmallVectorImpl<char> &R)
{
    return home_directory(R);
}

bool cache_directory(SmallVectorImpl<char> &R)
{
    return home_directory(R);
}

void system_temp_directory(bool, SmallVectorImpl<char> &R)
{
    const char *T = ::getenv("TMPDIR");
    if (!T)
        T = "/tmp";
    R.assign(T, T + strlen(T));
}
}
}
}
