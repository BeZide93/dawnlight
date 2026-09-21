#pragma once

namespace dawnlight {

// J3D draw queues are intrusive lists. A repeated packet can link a list back
// into itself: native drawing then submits the same geometry without end.
// Remove only that closing link, preserving every distinct packet and its order.
// Acyclic lists are never modified; there is no geometry/count limit.
template <class Packet>
bool break_packet_cycle(Packet* head) {
    auto* slow = head;
    auto* fast = head;
    do {
        if (fast == nullptr || fast->getNextPacket() == nullptr) return false;
        slow = slow->getNextPacket();
        fast = fast->getNextPacket()->getNextPacket();
    } while (slow != fast);

    slow = head;
    while (slow != fast) {
        slow = slow->getNextPacket();
        fast = fast->getNextPacket();
    }
    auto* last = slow;
    while (last->getNextPacket() != slow) last = last->getNextPacket();
    last->setNextPacket(nullptr);
    return true;
}

} // namespace dawnlight
