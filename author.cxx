#include <clypsalot/catalog.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/object.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

#include "test/module/object.hxx"
#include "test/module/port.hxx"

using namespace Clypsalot;
using namespace std::chrono_literals;

void process()
{
    auto source = ProcessingTestObject::make();
    auto sink = ProcessingTestObject::make();
    std::unique_lock sourceLock(*source);
    std::unique_lock sinkLock(*sink);
    auto& input = sink->publicAddInput<PTestInputPort>("input");
    auto& output = source->publicAddOutput<PTestOutputPort>("output");

    source->property("Test::Max Process").sizeValue(1000);
    sink->property("Test::Max Process").sizeValue(1000);

    source->configure();
    sink->configure();
    linkPorts(output, input);
    startObject(source);
    startObject(sink);

    sourceLock.unlock();
    sinkLock.unlock();

    std::this_thread::sleep_for(100ms);

    sourceLock.lock();
    sinkLock.lock();
    stopObject(source);
    stopObject(sink);
    sourceLock.unlock();
    sinkLock.unlock();
}

int main()
{
    logEngine().makeDestination<ConsoleDestination>(LogSeverity::trace);
    initThreadQueue(0);

    process();

    threadQueue().shutdown();
}
