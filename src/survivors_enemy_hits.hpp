#pragma once
#include <memory>

namespace dawnlight::survivors {
// A cast can own several colliders, projectiles or riders and outlive its
// caster. Keep its contact budget with the cast, never with a frame or timer.
struct EnemyAttackUse { bool contacted = false; unsigned contactTick = 0; };
using EnemyAttackUsePtr = std::shared_ptr<EnemyAttackUse>;
inline EnemyAttackUsePtr new_enemy_attack_use() { return std::make_shared<EnemyAttackUse>(); }
struct EnemyAttackCycle {
    int key = 0, action = 0, mode = 0;
    EnemyAttackUsePtr use;
    void observe(int nextKey, int nextAction, int nextMode) {
        if (nextKey && (nextKey != key ||
            (nextAction == action && nextMode == 0 && mode > 0)))
            use = new_enemy_attack_use();
        key = nextKey; action = nextAction; mode = nextMode;
    }
    EnemyAttackUsePtr current() {
        if (!use) use = new_enemy_attack_use();
        return use;
    }
};
}
