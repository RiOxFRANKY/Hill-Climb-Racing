// ============================================================================
// ASSET SPECIFICATION & DIMENSION STANDARDS
// ============================================================================
// File Path: assets/collectibles/coin.png
// Native Resolution: 24 x 24 px | Aspect Ratio: 1:1
// Description: Rotating golden coin with shine highlight.
//
// File Path: assets/collectibles/fuel_can.png
// Native Resolution: 24 x 30 px | Aspect Ratio: 4:5
// Description: Classic red military jerrycan with carry handle and white label.
//
// WARNING: Any replacement PNG for these collectibles MUST strictly match
// the exact pixel dimensions and aspect ratios specified above to ensure
// precise pickup collision radius and HUD alignment.
// ============================================================================

#pragma once

#include "physics/PhysicsTypes.h"

namespace Core {

enum class GameState {
    Playing,
    Paused,
    GameOver_OutOfFuel,
    GameOver_HeadCrash,
    LevelWon
};

enum class CollectibleType {
    Coin,
    FuelCan
};

struct Collectible {
    Physics::Vec2 position{0.0f, 0.0f};
    CollectibleType type{CollectibleType::Coin};
    int   value{5};            // Coins grant 5 or 25; fuel cans grant 100% fuel refill
    bool  collected{false};
    float bobTimer{0.0f};      // Floating sine bob animation offset
    float pickupRadius{26.0f}; // Collision threshold with vehicle center
};

} // namespace Core
