#include <clypsalot/logging.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

using namespace Clypsalot;
using namespace std::chrono_literals;

void stateChangedHandler(const ObjectStateChangedEvent& event)
{
    LOGGER(info, "Object changed state: ", event);
}

int main()
{
    logEngine().makeDestination<ConsoleDestination>(LogSeverity::trace);

    auto object = std::make_shared<Object>();

    std::unique_lock lock(*object);
    auto subscription = object->subscribe<ObjectStateChangedEvent>(&stateChangedHandler);
    object->configure();
    object->stop();
    lock.unlock();
}
