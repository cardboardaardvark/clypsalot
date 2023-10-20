#include <clypsalot/event.hxx>
#include <clypsalot/logging.hxx>
#include <clypsalot/macros.hxx>
#include <clypsalot/util.hxx>

using namespace Clypsalot;

struct SomeEvent : public Event
{ };

struct OtherEvent : public Event
{ };

int main()
{
    logEngine().makeDestination<Clypsalot::ConsoleDestination>(Clypsalot::LogSeverity::trace);

    auto sender = std::make_shared<EventSender>();

    sender->registerEvent<SomeEvent>();
    sender->registerEvent<OtherEvent>();

    auto someSubscription = sender->subscribe<SomeEvent>([](const Event&)
    {
        LOGGER(info, "Got SomeEvent");
    });

    auto otherSubscription = sender->subscribe<OtherEvent>([](const OtherEvent&)
    {
        LOGGER(info, "Got OtherEvent");
    });

    sender->send(SomeEvent());
    sender->send(OtherEvent());

    otherSubscription = nullptr;
    sender->send(SomeEvent());
    sender->send(OtherEvent());
}
