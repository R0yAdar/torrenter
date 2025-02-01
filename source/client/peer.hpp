#include "bitTorrent/messages.hpp"

template<typename T>
concept Sender = requires(T sender, btr::Choke choke) { sender.send(choke); };

class Peer
{

};