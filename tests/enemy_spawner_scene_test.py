"""Exercise production spawn ownership against the engine's layer/queue rules."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/enemy_spawner.cpp").read_text()
start = source.index("ModResult create_test_actor_in_player_layer(")
body = source.index("{", start)
depth = 0
for end in range(body, len(source)):
    depth += (source[end] == "{") - (source[end] == "}")
    if depth == 0:
        function = source[start:end + 1]
        break
assert "create_test_actor_in_player_layer(link, profile, params, actorId)" in source

stubs = r'''
#include <cassert>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <stdexcept>
using ActorId = unsigned;
using ProfileName = short;
enum ModResult { MOD_OK, MOD_ERROR, MOD_UNAVAILABLE };
struct ModContext {};
struct ActorSpawnParams { int room_num; };
struct layer_class { bool deleting=false; };
layer_class rootLayer, playLayer, otherLayer;
layer_class* currentLayer=&rootLayer;
layer_class* fpcLy_CurrentLayer() { return currentLayer; }
layer_class* fpcLy_RootLayer() { return &rootLayer; }
void fpcLy_SetCurrentLayer(layer_class* p) { currentLayer=p; }
bool fpcLy_IsDeletingMesg(layer_class* p) { return p->deleting; }
struct daAlink_c { struct { layer_class* layer; } layer_tag; };
bool nextStage=false;
bool dComIfGp_isEnableNextStage() { return nextStage; }
struct Request { ActorId id; ProfileName profile; int room; layer_class* layer; };
std::vector<Request> pending, actors;
ModResult createResult=MOD_OK;
bool throwInService=false;
int calls=0;
ModContext* mod_ctx=nullptr;
ModResult create_actor(ModContext*, ProfileName profile, const ActorSpawnParams* params, ActorId* id) {
    ++calls;
    if (throwInService) throw std::runtime_error("service failure");
    if (createResult!=MOD_OK) return createResult;
    *id=100+calls;
    // ActorService -> fopAcM_create -> fpcM_Create records CURRENT layer now;
    // async construction later must not use the restored UI layer.
    pending.push_back({*id,profile,params->room_num,fpcLy_CurrentLayer()});
    return MOD_OK;
}
using CreateFn = decltype(&create_actor);
struct ActorService { CreateFn create_actor; } service{&create_actor};
ActorService* svc_actor=&service;
struct Packet { Packet* next=nullptr; };
bool link_survives_drawing(layer_class* owner, int& draws) {
    Packet link, enemy;
    Packet* head=&link;
    auto submit = [&] { enemy.next=head; head=&enemy; ++draws; };
    // dScnPly_Draw draws the global actor queue, regardless of owner layer.
    submit();
    // fpcM_DrawIterater ALSO draws every root-owned process directly.
    if (owner==&rootLayer) submit();
    std::unordered_set<Packet*> seen;
    while (head && seen.insert(head).second) {
        if (head==&link) return true;
        head=head->next;
    }
    return false;
}
void delete_play_scene() {
    // Native recursive scene deletion owns only descendants of its layer.
    std::erase_if(actors, [](const Request& a) { return a.layer==&playLayer; });
}
'''

cases = r'''
int main() {
    daAlink_c link{{&playLayer}};
    ActorSpawnParams params{51}; ActorId id=0;
    // Reproduce the old UI path: correct room number, WRONG owner, double draw,
    // lost Link packet, and a surviving enemy after player/scene deletion.
    assert(create_actor(nullptr,451,&params,&id)==MOD_OK);
    assert(pending.back().room==51 && pending.back().layer==&rootLayer);
    int draws=0;
    assert(!link_survives_drawing(pending.back().layer,draws) && draws==2);
    actors=pending; delete_play_scene(); assert(actors.size()==1);
    actors.clear(); pending.clear();

    for (auto* caller : {&rootLayer,&otherLayer,&playLayer}) {
        currentLayer=caller;
        // Room 51 and ordinary gameplay rooms use the same ownership fix.
        for (int room : {0,51}) {
            params.room_num=room;
            assert(create_test_actor_in_player_layer(&link,451,params,id)==MOD_OK);
            assert(currentLayer==caller);
            assert(pending.back().layer==&playLayer && pending.back().room==room);
            assert(pending.back().profile==451 && pending.back().id==id);
            draws=0;
            assert(link_survives_drawing(pending.back().layer,draws) && draws==1);
        }
    }
    actors=pending; pending.clear(); delete_play_scene();
    assert(actors.empty()); // no enemy remains to dereference the deleted player

    currentLayer=&otherLayer;
    createResult=MOD_ERROR;
    assert(create_test_actor_in_player_layer(&link,451,params,id)==MOD_ERROR);
    assert(currentLayer==&otherLayer && pending.empty());
    throwInService=true;
    try { create_test_actor_in_player_layer(&link,451,params,id); assert(false); }
    catch (const std::runtime_error&) {}
    assert(currentLayer==&otherLayer);
    throwInService=false; createResult=MOD_OK;

    int oldCalls=calls;
    assert(create_test_actor_in_player_layer(nullptr,451,params,id)==MOD_UNAVAILABLE);
    nextStage=true;
    assert(create_test_actor_in_player_layer(&link,451,params,id)==MOD_UNAVAILABLE);
    nextStage=false; playLayer.deleting=true;
    assert(create_test_actor_in_player_layer(&link,451,params,id)==MOD_UNAVAILABLE);
    playLayer.deleting=false; link.layer_tag.layer=nullptr;
    assert(create_test_actor_in_player_layer(&link,451,params,id)==MOD_UNAVAILABLE);
    link.layer_tag.layer=&rootLayer;
    assert(create_test_actor_in_player_layer(&link,451,params,id)==MOD_UNAVAILABLE);
    assert(calls==oldCalls && currentLayer==&otherLayer && pending.empty());
}
'''

with tempfile.TemporaryDirectory() as temporary:
    folder = Path(temporary)
    cpp = folder / "spawner_scene.cpp"
    binary = folder / "spawner_scene"
    cpp.write_text(stubs + function + cases)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Enemy spawner scene ownership checks passed")
