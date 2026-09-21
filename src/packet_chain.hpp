#pragma once

namespace dawnlight {

// J3D draw queues are intrusive lists. A repeated packet can link a list back
// into itself: native drawing then submits the same geometry without end.
// Remove only that closing link, preserving every distinct packet and its order.
// Acyclic lists are never modified; there is no geometry/count limit.
template <class Packet, class Next, class Unlink>
bool break_link_cycle(Packet* head, Next next, Unlink unlink) {
    auto* slow = head;
    auto* fast = head;
    do {
        if (fast == nullptr || next(fast) == nullptr) return false;
        slow = next(slow);
        fast = next(next(fast));
    } while (slow != fast);

    slow = head;
    while (slow != fast) {
        slow = next(slow);
        fast = next(fast);
    }
    auto* last = slow;
    while (next(last) != slow) last = next(last);
    unlink(last);
    return true;
}

template <class Packet>
bool break_packet_cycle(Packet* head) {
    return break_link_cycle(head,
        [](Packet* p) { return p->getNextPacket(); },
        [](Packet* p) { p->setNextPacket(nullptr); });
}

} // namespace dawnlight
