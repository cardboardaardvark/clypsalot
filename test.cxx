#include <chrono>
#include <cstdlib>
#include <thread>

#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

int main()
{
    auto& console = Clypsalot::logEngine().makeDestination<Clypsalot::ConsoleDestination>(Clypsalot::LogSeverity::trace);

    for(size_t i = 0; i < 10; i++)
    {
        std::thread([]
        {
            while(true)
            {
                const auto currentThread = std::this_thread::get_id();
                const auto message = Clypsalot::makeString(currentThread, ": saying something");
                LOGGER(trace, "trace");
                LLOGGER(debug, { return "debug"; });
                std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 100));
            }
        }).detach();
    }

    for(size_t i = 0; i < 10; i++)
    {
        Clypsalot::LogSeverity severity = Clypsalot::LogSeverity::trace;

        if (i % 2 == 0)
        {
            severity = Clypsalot::LogSeverity::debug;
        }

        console.severity(severity);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
