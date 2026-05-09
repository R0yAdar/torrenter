# Torrenter
Downloading single-file torrents with Boost::Asio C++23 coroutines.

This project is more of a PoC, it supports:

*) UDP torrent trackers

*) TCP torrent peers

*) A local vault is used to store torrent pieces (so progress isn't lost on restart)

*) The current download strategy is just random round robin between peers (a more sophisticated approach is adaptive to the other peer's quality)

*) I didn't include graceful shutdown of all the coroutines (each peer has a coroutine and is like a worker, piece = task), but it shouldn't be hard to add it (just couple of flags...)

    
**** No further work is planned currently.

Inspired by Elden Ring.
![image](https://github.com/user-attachments/assets/5a237d36-339a-413a-8267-43ffa2e7fb4b)
