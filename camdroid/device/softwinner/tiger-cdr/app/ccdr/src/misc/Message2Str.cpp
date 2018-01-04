#include "msgstr.h"
#include "cdr_message.h"

const char*  Message2Str (int message)
{
    if (message >= 0x0000 && message <= 0x006F)
        return __mg_msgstr1 [message];
    else if (message >= 0x00A0 && message <= 0x010F)
        return __mg_msgstr2 [message - 0x00A0];
    else if (message >= 0x0120 && message <= 0x017F)
        return __mg_msgstr3 [message - 0x0120];
    else if (message >= 0xF000)
        return "Control Messages";
    else
        return "MSG_USER";
}
