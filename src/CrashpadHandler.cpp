#include <stdlib.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/crashpad_client.h>
#include <iostream>


bool setupCrashpadHandler()
{
    std::map<std::string, std::string> annotations;
    std::vector<std::string> arguments;
    crashpad::CrashpadClient client;
    bool rc;

#if defined(OS_POSIX)
	const char* handler_path = std::getenv("CRASHPAD_HANDLER_PATH");
	const char* db_path = std::getenv("CRASHPAD_DB_PATH");
#elif defined(OS_WIN)
	const wchar_t* handler_path = _wgetenv(L"CRASHPAD_HANDLER_PATH");
	const wchar_t* db_path = _wgetenv(L"CRASHPAD_DB_PATH");
#endif
    if (handler_path == nullptr) {
        std::cerr << "CRASHPAD_HANDLER_PATH is undefined" << std::endl;
        return false;
    }
    if (db_path == nullptr) {
        std::cerr << "CRASHPAD_DB_PATH is undefined" << std::endl;
        return false;
    }

    std::wcerr << "CRASHPAD_HANDLER_PATH: " << handler_path << std::endl;
    std::wcerr << "CRASHPAD_DB_PATH: " << db_path << std::endl;

    base::FilePath db(db_path);
    base::FilePath handler(handler_path);

    std::string url;

    std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(db);
    if (database == nullptr || database->GetSettings() == nullptr) return false;
    rc = client.StartHandler(handler,
                             db,
                             db,
                             url,
                             annotations,
                             arguments,
                             true,
                             false);

    return rc;
}