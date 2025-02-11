#include "asyncio.h"

void ReleaseMessage(Message* message) {
    if (message->msg != NULL) free(message->msg);
    free(message);
}