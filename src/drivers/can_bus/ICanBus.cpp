#include "ICanBus.h"

namespace uullrich::playground
{

const char* ICanBus::toString(Status status)
{
    using enum Status;
    switch (status)
    {
    case Ok:          return "OK";
    case FilterError: return "ERR:filter";
    case StartError:  return "ERR:start";
    case NotifyError: return "ERR:notify";
    case TxQueueFull: return "ERR:txfull";
    }
    return "ERR:unknown";
}

}
