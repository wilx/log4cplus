// Exercise shared-lock initialization and recovery with long-lived processes.
#include <log4cplus/initializer.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/helpers/fileinfo.h>
#include <log4cplus/helpers/lockfile.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/spi/loggingevent.h>

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#if defined (_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char ** environ;
#endif

namespace
{
using namespace log4cplus;
using Clock = std::chrono::steady_clock;

void require (bool condition, char const * message)
{
    if (! condition)
        throw std::runtime_error (message);
}

tstring nativePath (std::filesystem::path const & path)
{
#if defined (_WIN32)
#if defined (UNICODE)
    return path.wstring ();
#else
    return helpers::tostring (path.wstring ());
#endif
#else
    return LOG4CPLUS_STRING_TO_TSTRING (path.string ());
#endif
}

std::string readFile (std::filesystem::path const & path)
{
    std::ifstream input (path, std::ios::binary);
    require (input.good (), "cannot read log file");
    return {std::istreambuf_iterator<char> (input), {}};
}

class DailyWriter : public DailyRollingFileAppender
{
public:
    using DailyRollingFileAppender::DailyRollingFileAppender;
    using DailyRollingFileAppender::rollover;

    void log (std::string const & message)
    {
        spi::InternalLoggingEvent event (LOG4CPLUS_TEXT ("daily-process-test"),
            INFO_LOG_LEVEL, LOG4CPLUS_STRING_TO_TSTRING (message), "", 0);
        doAppend (event);
    }
};

int worker (std::filesystem::path const & directory, bool truncate)
{
    Initializer initializer;
    helpers::Properties props;
    props.setProperty (LOG4CPLUS_TEXT ("File"), nativePath (directory / "active.log"));
    props.setProperty (LOG4CPLUS_TEXT ("LockFile"), nativePath (directory / "shared.lock"));
    props.setProperty (LOG4CPLUS_TEXT ("UseLockFile"), LOG4CPLUS_TEXT ("true"));
    props.setProperty (LOG4CPLUS_TEXT ("Append"),
        truncate ? LOG4CPLUS_TEXT ("false") : LOG4CPLUS_TEXT ("true"));
    props.setProperty (LOG4CPLUS_TEXT ("Schedule"), LOG4CPLUS_TEXT ("DAILY"));
    props.setProperty (LOG4CPLUS_TEXT ("DatePattern"), LOG4CPLUS_TEXT ("archive"));
    props.setProperty (LOG4CPLUS_TEXT ("MaxBackupIndex"), LOG4CPLUS_TEXT ("2"));
    props.setProperty (LOG4CPLUS_TEXT ("RollOnClose"), LOG4CPLUS_TEXT ("false"));
    props.setProperty (LOG4CPLUS_TEXT ("ImmediateFlush"), LOG4CPLUS_TEXT ("false"));
    props.setProperty (LOG4CPLUS_TEXT ("TextMode"), LOG4CPLUS_TEXT ("Binary"));
    std::cout << "STARTING\n" << std::flush;
    DailyWriter writer (props);
    writer.setLayout (std::make_unique<PatternLayout> (LOG4CPLUS_TEXT ("%m|")));
    std::cout << "READY\n" << std::flush;
    for (std::string command; std::getline (std::cin, command);)
    {
        if (command == "ROLL")
            writer.rollover ();
        else if (command.starts_with ("APPEND "))
            writer.log (command.substr (7));
        else if (command == "QUIT")
        {
            writer.close ();
            std::cout << "OK\n" << std::flush;
            return 0;
        }
        else
            throw std::runtime_error ("unknown worker command");
        std::cout << "OK\n" << std::flush;
    }
    throw std::runtime_error ("worker command pipe closed unexpectedly");
}

#if defined (_WIN32)
using PipeHandle = HANDLE;
PipeHandle const invalidPipe = INVALID_HANDLE_VALUE;
void closePipe (PipeHandle handle) { CloseHandle (handle); }
#else
using PipeHandle = int;
constexpr PipeHandle invalidPipe = -1;
void closePipe (PipeHandle handle) { close (handle); }
#endif

struct PipeOwner
{
    PipeHandle value = invalidPipe;
    ~PipeOwner () { if (value != invalidPipe) closePipe (value); }
};

class Child
{
    PipeOwner commands, replies;
#if defined (_WIN32)
    HANDLE process = nullptr;
#else
    pid_t process = -1;
#endif

public:
    Child (std::filesystem::path const & executable,
        std::filesystem::path const & directory, bool truncate = false)
    {
        PipeOwner commandRead, replyWrite;
#if defined (_WIN32)
        SECURITY_ATTRIBUTES security {sizeof (security), nullptr, TRUE};
        require (CreatePipe (&commandRead.value, &commands.value, &security, 0),
            "CreatePipe commands failed");
        require (CreatePipe (&replies.value, &replyWrite.value, &security, 0),
            "CreatePipe replies failed");
        require (SetHandleInformation (commands.value, HANDLE_FLAG_INHERIT, 0)
            && SetHandleInformation (replies.value, HANDLE_FLAG_INHERIT, 0),
            "cannot disable parent pipe inheritance");
        PipeOwner errors;
        require (DuplicateHandle (GetCurrentProcess (), GetStdHandle (STD_ERROR_HANDLE),
            GetCurrentProcess (), &errors.value, 0, TRUE, DUPLICATE_SAME_ACCESS),
            "cannot duplicate stderr");
        SIZE_T bytes = 0;
        InitializeProcThreadAttributeList (nullptr, 1, 0, &bytes);
        std::vector<unsigned char> storage (bytes);
        auto * attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST> (storage.data ());
        require (InitializeProcThreadAttributeList (attributes, 1, 0, &bytes),
            "cannot initialize process attributes");
        struct AttributeOwner
        {
            LPPROC_THREAD_ATTRIBUTE_LIST value;
            ~AttributeOwner () { DeleteProcThreadAttributeList (value); }
        } attributeOwner {attributes};
        HANDLE inherited[] {commandRead.value, replyWrite.value, errors.value};
        require (UpdateProcThreadAttribute (attributes, 0,
            PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof (inherited),
            nullptr, nullptr), "cannot restrict pipe inheritance");
        STARTUPINFOEXW startup {};
        startup.StartupInfo.cb = sizeof (startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        startup.StartupInfo.hStdInput = commandRead.value;
        startup.StartupInfo.hStdOutput = replyWrite.value;
        startup.StartupInfo.hStdError = errors.value;
        startup.lpAttributeList = attributes;
        PROCESS_INFORMATION child {};
        std::wstring command = L"\"" + executable.wstring () + L"\" --worker \""
            + directory.wstring () + (truncate ? L"\" truncate" : L"\" append");
        require (CreateProcessW (executable.c_str (), command.data (), nullptr,
            nullptr, TRUE, EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
            &startup.StartupInfo, &child), "CreateProcessW failed");
        CloseHandle (child.hThread);
        process = child.hProcess;
#else
        int pair[2];
        require (pipe (pair) == 0, "pipe commands failed");
        commandRead.value = pair[0]; commands.value = pair[1];
        require (pipe (pair) == 0, "pipe replies failed");
        replies.value = pair[0]; replyWrite.value = pair[1];
        for (int fd : {commandRead.value, commands.value, replies.value, replyWrite.value})
            require (fcntl (fd, F_SETFD, FD_CLOEXEC) == 0, "cannot set close-on-exec");
        posix_spawn_file_actions_t actions;
        require (posix_spawn_file_actions_init (&actions) == 0, "spawn actions failed");
        struct ActionsOwner
        {
            posix_spawn_file_actions_t * value;
            ~ActionsOwner () { posix_spawn_file_actions_destroy (value); }
        } actionsOwner {&actions};
        require (posix_spawn_file_actions_adddup2 (&actions, commandRead.value, STDIN_FILENO) == 0
            && posix_spawn_file_actions_adddup2 (&actions, replyWrite.value, STDOUT_FILENO) == 0,
            "spawn pipe redirection failed");
        std::string exe = executable.string (), dir = directory.string ();
        char * args[] {exe.data (), const_cast<char *> ("--worker"), dir.data (),
            const_cast<char *> (truncate ? "truncate" : "append"), nullptr};
        require (posix_spawn (&process, exe.c_str (), &actions, nullptr, args, environ) == 0,
            "posix_spawn failed");
#endif
    }

    Child (Child const &) = delete;
    Child & operator= (Child const &) = delete;
    ~Child ()
    {
#if defined (_WIN32)
        if (process)
        {
            if (WaitForSingleObject (process, 0) != WAIT_OBJECT_0)
            {
                TerminateProcess (process, 90);
                WaitForSingleObject (process, 5000);
            }
            CloseHandle (process);
        }
#else
        if (process > 0)
        {
            kill (process, SIGKILL);
            while (waitpid (process, nullptr, 0) < 0 && errno == EINTR) {}
        }
#endif
    }

    bool ready (int timeout)
    {
        auto const deadline = Clock::now () + std::chrono::milliseconds (timeout);
        do
        {
#if defined (_WIN32)
            DWORD available = 0;
            require (PeekNamedPipe (replies.value, nullptr, 0, nullptr, &available, nullptr),
                "worker reply pipe failed");
            if (available)
                return true;
            if (Clock::now () >= deadline)
                return false;
            DWORD result = WaitForSingleObject (process, 10);
            if (result != WAIT_TIMEOUT)
            {
                require (result == WAIT_OBJECT_0 && PeekNamedPipe (replies.value,
                    nullptr, 0, nullptr, &available, nullptr) && available,
                    "worker exited before replying");
                return true;
            }
#else
            auto const remaining = std::chrono::duration_cast<std::chrono::milliseconds>
                (deadline - Clock::now ()).count ();
            pollfd fd {replies.value, POLLIN, 0};
            int result = poll (&fd, 1, static_cast<int> (std::max<int64_t> (remaining, 0)));
            if (result >= 0)
                return result > 0;
            require (errno == EINTR, "poll failed");
#endif
        } while (Clock::now () < deadline);
        return false;
    }

    std::string receive ()
    {
        std::string line;
        auto const deadline = Clock::now () + std::chrono::seconds (10);
        for (;;)
        {
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>
                (deadline - Clock::now ()).count ();
            require (remaining > 0 && ready (static_cast<int> (remaining)),
                "worker response timed out");
            char ch;
#if defined (_WIN32)
            DWORD count = 0;
            require (ReadFile (replies.value, &ch, 1, &count, nullptr) && count == 1,
                "cannot read worker reply");
#else
            ssize_t count = read (replies.value, &ch, 1);
            if (count < 0 && errno == EINTR)
                continue;
            require (count == 1, "cannot read worker reply");
#endif
            if (ch == '\n')
                return line;
            if (ch != '\r')
                line += ch;
        }
    }

    void expect (char const * expected)
    {
        require (receive () == expected, "unexpected worker reply");
    }

    void command (std::string text)
    {
        text += '\n';
        size_t offset = 0;
        while (offset < text.size ())
        {
#if defined (_WIN32)
            DWORD count = 0;
            require (WriteFile (commands.value, text.data () + offset,
                static_cast<DWORD> (text.size () - offset), &count, nullptr) && count > 0,
                "cannot send worker command");
#else
            ssize_t count = write (commands.value, text.data () + offset, text.size () - offset);
            if (count < 0 && errno == EINTR)
                continue;
            require (count > 0, "cannot send worker command");
#endif
            offset += static_cast<size_t> (count);
        }
        expect ("OK");
    }

    void finish ()
    {
        command ("QUIT");
#if defined (_WIN32)
        require (WaitForSingleObject (process, 10000) == WAIT_OBJECT_0,
            "worker shutdown timed out");
        DWORD status = 0;
        require (GetExitCodeProcess (process, &status) && status == 0, "worker failed");
        CloseHandle (process);
        process = nullptr;
#else
        int status = 0;
        pid_t result;
        do { result = waitpid (process, &status, 0); } while (result < 0 && errno == EINTR);
        require (result == process && WIFEXITED (status) && WEXITSTATUS (status) == 0,
            "worker failed");
        process = -1;
#endif
    }
};

int parent (std::filesystem::path const & executable)
{
    Initializer initializer;
    struct Directory
    {
        std::filesystem::path path = std::filesystem::temp_directory_path ()
            / ("log4cplus-daily-process-" + std::to_string (Clock::now ().time_since_epoch ().count ()));
        Directory () { require (std::filesystem::create_directory (path), "cannot create test directory"); }
        ~Directory () { std::error_code error; std::filesystem::remove_all (path, error); }
    } directory;

    // STARTING acknowledges that the child is about to construct the appender.
    // A truncating initial open must wait for the explicitly supplied lock.
    std::ofstream (directory.path / "active.log", std::ios::binary) << "protected\n";
    helpers::LockFile shared (nativePath (directory.path / "shared.lock"));
    helpers::LockFileGuard guard (shared);
    Child opening (executable, directory.path, true);
    opening.expect ("STARTING");
    require (! opening.ready (250), "initial opening ignored the explicit shared lock");
    require (readFile (directory.path / "active.log") == "protected\n",
        "initial opening truncated the log while another process held the lock");
    guard.unlock ();
    guard.detach ();
    opening.expect ("READY");
    opening.finish ();

    Child first (executable, directory.path), second (executable, directory.path);
    first.expect ("STARTING"); first.expect ("READY");
    second.expect ("STARTING"); second.expect ("READY");
    second.command ("APPEND original");
    helpers::FileInfo original;
    require (helpers::getFileInfo (&original, nativePath (directory.path / "active.log")),
        "cannot query original identity");
    for (int i = 0; i < 5; ++i)
    {
        first.command ("ROLL");
        first.command ("APPEND current-" + std::to_string (i));
    }
    for (char const * name : {"active.log", "active.log.archive", "active.log.archive.1"})
    {
        helpers::FileInfo info;
        require (helpers::getFileInfo (&info, nativePath (directory.path / name)),
            "missing active log or retained archive");
        require (info.identity != original.identity, "original file was not removed by retention");
    }
    auto const oldest = directory.path / "active.log.archive.2";
    helpers::FileInfo retained;
#if defined (_WIN32)
    // Windows queues deletion until the survivor closes its original handle.
    // Wine still permits metadata queries on the pending path and does not
    // expose DeletePending. Confirm deletion is accepted in that case, then
    // require the original identity to disappear after recovery closes it.
    if (helpers::getFileInfo (&retained, nativePath (oldest)))
    {
        if (retained.identity == original.identity)
            require (DeleteFileW (oldest.c_str ()), "original archive deletion was not accepted");
    }
    else
        require (GetLastError () == ERROR_ACCESS_DENIED || GetLastError () == ERROR_FILE_NOT_FOUND,
            "unexpected error querying oldest archive");
#else
    require (helpers::getFileInfo (&retained, nativePath (oldest))
        && retained.identity != original.identity, "original archive was not removed");
#endif
    second.command ("APPEND survivor");
    require (readFile (directory.path / "active.log") == "current-4|survivor|",
        "surviving writer did not append to the active file");
    require (! helpers::getFileInfo (&retained, nativePath (oldest))
        || retained.identity != original.identity, "original archive survived recovery");
    first.finish ();
    second.finish ();
    return 0;
}
} // namespace

#if defined (_WIN32)
int wmain (int argc, wchar_t ** argv)
#else
int main (int argc, char ** argv)
#endif
{
    try
    {
        if (argc == 4)
        {
            require (std::filesystem::path (argv[1]) == "--worker", "unknown mode");
            return worker (argv[2], std::filesystem::path (argv[3]) == "truncate");
        }
        require (argc == 1, "unexpected arguments");
#if defined (_WIN32)
        wchar_t executable[32768];
        DWORD length = GetModuleFileNameW (nullptr, executable, std::size (executable));
        require (length > 0 && length < std::size (executable), "cannot obtain executable path");
        return parent (executable);
#else
        signal (SIGPIPE, SIG_IGN);
        return parent (std::filesystem::absolute (argv[0]));
#endif
    }
    catch (std::exception const & error)
    {
        std::cerr << error.what () << '\n';
        return 1;
    }
}
