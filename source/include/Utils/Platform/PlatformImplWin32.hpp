#ifdef _WIN32

inline os_library_handle library_load(const os_string& path)
{
    return LoadLibraryW(path.c_str());
}

inline os_library_function library_get_address(os_library_handle handle, const std::string& name)
{
    return reinterpret_cast<os_library_function>(GetProcAddress(handle, name.c_str())); // should be _cdecl or x64
}

inline void library_free(os_library_handle handle)
{
    FreeLibrary(handle);
}

inline static size_t get_page_size()
{
    SYSTEM_INFO sysInfo {};
    GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
}

inline static const size_t page_size = get_page_size();

inline void* aligned_alloc(size_t len, size_t align)
{
    return _aligned_malloc(len, align);
}

inline void aligned_free(void* ptr)
{
    _aligned_free(ptr);
}

inline static std::string get_ansi_short_path(const std::filesystem::path& path)
{
    wchar_t short_path[MAX_PATH] {};
    auto& osstr = path.native();
    auto shortlen = GetShortPathNameW(osstr.c_str(), short_path, MAX_PATH);
    if (shortlen == 0) return {};
    BOOL failed = FALSE;
    auto ansilen = WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, nullptr, 0, nullptr, &failed);
    if (failed) return {};
    std::string result(ansilen, 0);
    WideCharToMultiByte(CP_ACP, 0, short_path, shortlen, result.data(), ansilen, nullptr, nullptr);
    return result;
}

inline std::string path_to_crt_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    size_t mbsize = 0;
    auto& osstr = path.native();
    auto err = wcstombs_s(&mbsize, nullptr, 0, osstr.c_str(), osstr.size());
    if (err == 0) {
        std::string result(mbsize, 0);
        err = wcstombs_s(&mbsize, result.data(), mbsize, osstr.c_str(), osstr.size());
        if (err != 0) return {};
        return result.substr(0, mbsize - 1);
    }
    else {
        // cannot convert (CRT is not using UTF-8), fallback to short path name in ACP
        return get_ansi_short_path(path);
    }
}

inline std::string path_to_ansi_string(const std::filesystem::path& path)
{
    // UCRT may use UTF-8 encoding while ANSI code page is still some other MBCS encoding
    // so we use CRT wcstombs instead of WideCharToMultiByte
    BOOL failed = FALSE;
    auto& osstr = path.native();
    auto ansilen = WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), nullptr, 0, nullptr, &failed);
    if (!failed) {
        std::string result(ansilen, 0);
        WideCharToMultiByte(CP_ACP, 0, osstr.c_str(), (int)osstr.size(), result.data(), ansilen, nullptr, &failed);
        return result;
    }
    else {
        // contains character that cannot be converted, fallback to short path name in ACP
        return get_ansi_short_path(path);
    }
}

inline os_string to_osstring(std::string_view utf8_str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), nullptr, 0);
    os_string result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), (int)utf8_str.size(), result.data(), len);
    return result;
}

inline std::string from_osstring(const os_string_view& os_str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, os_str.data(), (int)os_str.size(), nullptr, 0, nullptr, nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, os_str.data(), (int)os_str.size(), result.data(), len, nullptr, nullptr);
    return result;
}

// 转义参考:
// https://ipvb.gitee.io/windows/2019/07/21/CmdlineReEscape/
// https://github.com/microsoft/terminal/pull/1815

inline os_string escape_one(os_string_view arg)
{
    if (arg.empty()) {
        return os_string(2, '"');
    }

    bool space = false;
    auto len = arg.size();

    for (auto ch : arg) {
        switch (ch) {
        case '"':
        case '\\':
            len++;
            break;
        case ' ':
        case '\t':
            space = true;
            break;
        }
    }

    if (space) {
        len += 2;
    }

    if (len == arg.size()) {
        return os_string(arg);
    }

    os_string buf;
    buf.reserve(len);

    if (space) {
        buf.push_back('"');
    }

    int slash = 0;

    for (auto ch : arg) {
        switch (ch) {
        case '\\':
            slash++;
            buf.push_back('\\');
            break;
        case '"':
            buf.append(os_string(slash + 1, '\\'));
            buf.push_back('"');
            break;
        default:
            slash = 0;
            buf.push_back(ch);
        }
    }
    if (space) {
        buf.append(os_string(slash, '\\'));
        buf.push_back('"');
    }
    return buf;
}

inline os_string args_to_cmd(const std::vector<os_string>& args)
{
    if (args.size() == 0) {
        return os_string {};
    }

    os_string res = escape_one(args[0]);
    for (int i = 1; i < args.size(); i++) {
        res.push_back(' ');
        res.append(escape_one(args[i]));
    }
    return res;
}

inline os_string args_to_cmd(const std::vector<os_string_view>& args)
{
    if (args.size() == 0) {
        return os_string {};
    }

    os_string res = escape_one(args[0]);
    for (int i = 1; i < args.size(); i++) {
        res.push_back(' ');
        res.append(escape_one(args[i]));
    }
    return res;
}

inline std::vector<os_string> cmd_to_args(const os_string& cmd)
{
    int argc;
    std::vector<os_string> res;

    auto result = CommandLineToArgvW(cmd.c_str(), &argc);

    for (int i = 0; i < argc; i++) {
        res.emplace_back(result[i]);
    }

    LocalFree(result);

    return res;
}

// Allow construct a path from utf8-string in win32; string_view ver.
inline std::filesystem::path path(std::string_view utf8_str)
{
    return std::filesystem::path(to_osstring(utf8_str));
}

inline std::string path_to_utf8_string(const std::filesystem::path& path)
{
    return from_osstring(path.native());
}

inline std::string path_to_crt_string(std::string_view utf8_path)
{
    return path_to_crt_string(path(utf8_path));
}

inline std::string path_to_ansi_string(std::string_view utf8_path)
{
    return path_to_crt_string(path(utf8_path));
}

#endif