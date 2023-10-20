#include <thread>

#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

using namespace Clypsalot;
using namespace std::chrono_literals;

int main()
{
    logEngine().makeDestination<Clypsalot::ConsoleDestination>(Clypsalot::LogSeverity::info);

    initThreadQueue(0);

    LOGGER(info, "This is the main thread");

    for (size_t i = 0; i < 3; i++)
    {
        std::this_thread::sleep_for(1s);

        auto value = THREAD_CALL([] { return 1; });
        LOGGER(info, "Value: ", value);
    }

    shutdownThreadQueue();
}
