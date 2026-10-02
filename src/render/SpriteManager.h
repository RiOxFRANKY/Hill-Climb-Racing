// ============================================================================
// ASSET SPECIFICATION & DIMENSION MASTER INDEX
// ============================================================================
// 1.  assets/vehicle/chassis.png            [96 x 48 px,  Aspect Ratio: 2:1]
// 2.  assets/vehicle/wheel.png              [32 x 32 px,  Aspect Ratio: 1:1]
// 3.  assets/vehicle/driver_head.png        [24 x 24 px,  Aspect Ratio: 1:1]
// 4.  assets/terrain/grass_top.png          [32 x 16 px,  Aspect Ratio: 2:1]
// 5.  assets/terrain/dirt_fill.png          [32 x 32 px,  Aspect Ratio: 1:1]
// 6.  assets/collectibles/coin.png          [24 x 24 px,  Aspect Ratio: 1:1]
// 7.  assets/collectibles/fuel_can.png      [24 x 30 px,  Aspect Ratio: 4:5]
// 8.  assets/ui/pedal_gas_released.png      [36 x 60 px,  Aspect Ratio: 3:5]
// 9.  assets/ui/pedal_gas_pressed.png       [36 x 60 px,  Aspect Ratio: 3:5]
// 10. assets/ui/pedal_brake_released.png    [36 x 60 px,  Aspect Ratio: 3:5]
// 11. assets/ui/pedal_brake_pressed.png     [36 x 60 px,  Aspect Ratio: 3:5]
// 12. assets/ui/meter_dial.png              [64 x 64 px,  Aspect Ratio: 1:1]
// 13. assets/ui/dial_needle.png             [8 x 32 px,   Aspect Ratio: 1:4]
// 14. assets/backgrounds/sky_bg.png         [960 x 540 px, Aspect Ratio: 16:9]
//
// WARNING: Any replacement PNG MUST strictly match these native resolutions.
// Arbitrary downscaling in-game is prohibited by the Uniform Pixel Density Rule.
// ============================================================================

#pragma once

#include <QtGui/QPixmap>
#include <QtCore/QString>
#include <unordered_map>

namespace Render {

enum class AssetId {
    Vehicle_Chassis,
    Vehicle_Wheel,
    Vehicle_DriverHead,
    Terrain_GrassTop,
    Terrain_DirtFill,
    Collectible_Coin,
    Collectible_FuelCan,
    UI_PedalGasReleased,
    UI_PedalGasPressed,
    UI_PedalBrakeReleased,
    UI_PedalBrakePressed,
    UI_MeterDial,
    UI_DialNeedle,
    Background_Sky
};

class SpriteManager {
public:
    // ========================================================================
    // TWEAKABLE PARAMETERS
    // ========================================================================
    struct Config {
        bool enableResourceFallbacks{true};
        bool verboseLogging{false};
    };

    SpriteManager();
    explicit SpriteManager(const Config& config);

    // Loads all 14 game assets from disk or compiled Qt resources
    bool loadAll();

    // Query asset availability
    bool hasAsset(AssetId id) const;

    // Retrieve cached QPixmap (or dummy null pixmap if missing)
    const QPixmap& getPixmap(AssetId id) const;

    Config& config() { return m_config; }
    const Config& config() const { return m_config; }

private:
    bool tryLoadPixmap(AssetId id, const QString& relativePath);
    QString resolveAssetPath(const QString& relativePath) const;

    Config m_config;
    std::unordered_map<AssetId, QPixmap> m_sprites;
    QPixmap m_nullPixmap;
};

} // namespace Render
