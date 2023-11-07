#include <clypsalot/logging.hxx>
#include <clypsalot/network.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/module.hxx>
#include <clypsalot/property.hxx>
#include <clypsalot/thread.hxx>
#include <clypsalot/util.hxx>

#include "test/lib/test.hxx"
#include "test/module/module.hxx"
#include "test/module/object.hxx"
#include "test/module/port.hxx"

using namespace Clypsalot;
using namespace std::chrono_literals;

std::shared_ptr<ProcessingTestObject> makeTestObject(Network& network)
{
    return makeTestObject<ProcessingTestObject>(network, "Test::Processing Object");
}

std::shared_ptr<ProcessingTestObject> makeSourceObject(Network& network)
{
    auto object = makeTestObject(network);
    std::unique_lock lock(*object);
    object->addOutput(PTestPortType::typeName, "output");
    object->configure();
    return object;
}

std::shared_ptr<ProcessingTestObject> makeSinkObject(Network& network)
{
    auto object = makeTestObject(network);
    std::unique_lock lock(*object);
    object->addInput(PTestPortType::typeName, "input");
    object->configure();
    return object;
}

std::shared_ptr<ProcessingTestObject> makeFilterObject(Network& network)
{
    auto object = makeTestObject(network);
    std::unique_lock lock(*object);
    object->publicAddOutput<PTestOutputPort>("output");
    object->publicAddInput<PTestInputPort>("input");
    object->configure();
    return object;
}

void linkObjects(const SharedObject& from, const SharedObject& to)
{
    std::unique_lock fromLock(*from);
    std::unique_lock toLock(*to);
    linkPorts(from->output("output"), to->input("input"));
}

void process()
{
    Network network;
    std::vector<SharedObject> objects;

    objects.push_back(makeSourceObject(network));

    for (size_t i = 0; i < 8; i++)
    {
        objects.push_back(makeFilterObject(network));
    }

    objects.push_back(makeSinkObject(network));

    {
        std::unique_lock lock(*objects.at(5));
        objects.at(5)->property("Max Process").sizeValue(5);
    }

    linkObjects(objects.at(0), objects.at(1));
    linkObjects(objects.at(1), objects.at(2));
    linkObjects(objects.at(1), objects.at(3));
    linkObjects(objects.at(1), objects.at(9));
    linkObjects(objects.at(2), objects.at(3));
    linkObjects(objects.at(3), objects.at(4));
    linkObjects(objects.at(4), objects.at(5));
    linkObjects(objects.at(5), objects.at(6));
    linkObjects(objects.at(6), objects.at(7));
    linkObjects(objects.at(7), objects.at(8));
    linkObjects(objects.at(8), objects.at(9));

    network.run();
}

int main()
{
    logEngine().makeDestination<ConsoleDestination>(LogSeverity::trace);
    initThreadQueue(0);

    importModule(testModuleDescriptor());
    process();

    shutdownThreadQueue();
}
