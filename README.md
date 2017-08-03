# chat_srv
Proof of concept of coroutine based chat server


# Prerequisites

It is Xcode 8 project with C++ code.
To be able to run and debug xcode project you will need macOs 10.12.x and Xcode 8.3.3.
Also - boost is required - http://brewformulas.org/Boost .

# Run
Build binary (look at project/chat_srv/Build/Debug )
Run with port argument:
./chat_srv 65536

# Test
telnet 127.0.0.1 65536
