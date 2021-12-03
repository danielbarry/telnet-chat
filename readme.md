# Telnet Chat

*A simple telnet chat server with basic functionality.*

## Why?

I personally have a really small server that does *a lot* of heavy lifting, and
literally doesn't have more than 1MB of RAM to spare at best. I want to run a
chat client for a handful of people to talk encase other methods fail.

I saw there are [some](https://lunatic.solutions/blog/lunatic-chat/)
[other](https://github.com/tywkeene/telnet-chat)
[implementations](https://github.com/stubee84/Telnet-Chat), but they are all
actually a little heavier than I would like.

## Features

This project focuses on the following features:

* *Low-CPU* - We don't have much available, so we do very limited processing on
strings.
* *Low-RAM* - We don't buffer anything, we get rid of it as soon as we can.
* *Simple* - It needs to be easy to use, otherwise people will not use it.

## Implementation Details

The initial server design was based on the [GNU '16.9.7 Byte Stream Connection
Server
Example'](https://www.gnu.org/software/libc/manual/html_node/Server-Example.html).

The server uses [`TCP_CORK`](https://baus.net/on-tcp_cork/), meaning that
messages get buffered in the kernel rather than user space. Ultimately this
saves on bandwidth and means that I don't have to implement my own buffering.

## Future

Some ideas about where to take this in the future:

* *Cloaking* - Add at least some privacy.
* *Moderation* - The ability to kick out annoying people.
* *Auto-moderation* - Stop people from spamming/abusing the service.
* *Limits* - Put limits on connection time, bandwidth, number of connections,
etc.
