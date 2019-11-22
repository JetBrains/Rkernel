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

    const char* handler_path = std::getenv("CRASHPAD_HANDLER_PATH");
    if (handler_path == nullptr) {
        std::cerr << "CRASHPAD_HANDLER_PATH is undefined" << std::endl;
        return false;
    }
    const char *db_path = std::getenv("CRASHPAD_DB_PATH");
    if (db_path == nullptr) {
        std::cerr << "CRASHPAD_HANDLER_PATH is undefined" << std::endl;
        return false;
    }

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