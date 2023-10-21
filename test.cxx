#include <clypsalot/catalog.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"

using namespace Clypsalot;
using namespace std::chrono_literals;

void stateChangedHandler(const ObjectStateChangedEvent& event)
{
    LOGGER(info, "Object changed state: ", event);
}

int main()
{
    logEngine().makeDestination<ConsoleDestination>(LogSeverity::trace);

    initTesting();

    auto object = objectCatalog().make("Test::Object");

    std::unique_lock lock(*object);
    auto subscription = object->subscribe<ObjectStateChangedEvent>(&stateChangedHandler);
    object->configure();
    object->stop();
    lock.unlock();
}
