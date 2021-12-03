#define PORT 6666
#define MAX_READ 512

#define MSG_BANNER (\
"| ------------------------------\n"  \
"| Welcome to Telnet Chat!\n"         \
"| NOTE: All IPs displayed in chat!\n"\
"| WARN: All data unencrypted!\n"     \
"| Type `/h` for more info.\n"        \
"| ------------------------------\n"  \
)
#define MSG_HELP (\
"| Commands are as follows:\n"        \
"| /b       - Display the banner\n"   \
"| /h or /? - Display this help\n"    \
"| /l       - List connected IPs\n"   \
"| /q       - Quit the connection\n"  \
)
#define MSG_EXIT (\
"| Have a nice trip!\n"               \
)
