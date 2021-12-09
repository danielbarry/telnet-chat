#define PORT 6666
#define MAX_READ 512
/* NOTE: Timeout is 24 hours. */
#define TIMEOUT_MS (24 * 60 * 60 * 1000)

#define MSG_PRE ":"
#define MSG_BANNER (\
MSG_PRE "--------------------------------\n"\
MSG_PRE "Welcome to Telnet Chat!\n"         \
MSG_PRE "NOTE: All IPs displayed in chat!\n"\
MSG_PRE "WARN: All data unencrypted!\n"     \
MSG_PRE "Type `/h` for more info.\n"        \
MSG_PRE "--------------------------------\n"\
)
#define MSG_HELP (\
MSG_PRE "Commands are as follows:\n"        \
MSG_PRE "/b       - Display the banner\n"   \
MSG_PRE "/h or /? - Display this help\n"    \
MSG_PRE "/l       - List connected IPs\n"   \
MSG_PRE "/q       - Quit the connection\n"  \
)
#define MSG_EXIT (\
MSG_PRE "Have a nice trip!\n"               \
)
