"""Verify the arena draw-list repair preserves every distinct packet."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = r'''
#include "packet_chain.hpp"
#include <cassert>
#include <vector>
struct Packet {
    Packet* next = nullptr;
    Packet* getNextPacket() { return next; }
    void setNextPacket(Packet* value) { next = value; }
};
int main() {
    assert(!dawnlight::break_packet_cycle<Packet>(nullptr));
    // Includes self-cycles, cycles after a prefix, and long valid queues.
    for (int count : {1, 2, 3, 100, 10000}) {
        for (int loop : {-1, 0, count / 2, count - 1}) {
            std::vector<Packet> packets(count);
            for (int i = 0; i + 1 < count; ++i) packets[i].next = &packets[i+1];
            if (loop >= 0) packets.back().next = &packets[loop];
            assert(dawnlight::break_packet_cycle(packets.data()) == (loop >= 0));
            auto* p = packets.data();
            for (int i = 0; i < count; ++i) {
                assert(p == &packets[i]);
                p = p->getNextPacket();
            }
            assert(p == nullptr);
            assert(!dawnlight::break_packet_cycle(packets.data()));
        }
    }
}
'''
with tempfile.TemporaryDirectory() as temporary:
    folder = Path(temporary)
    cpp = folder / "packet_chains.cpp"
    binary = folder / "packet_chains"
    cpp.write_text(source)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-I", str(root / "src"),
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Arena packet-chain checks passed")
