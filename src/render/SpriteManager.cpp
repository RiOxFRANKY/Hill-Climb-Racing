#include "render/SpriteManager.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>

namespace Render {

SpriteManager::SpriteManager()
    : m_config() {
}

SpriteManager::SpriteManager(const Config& config)
    : m_config(config) {
}

QString SpriteManager::resolveAssetPath(const QString& relativePath) const {
    // 1. Qt compiled binary resources
    QString qrcPath = ":/" + relativePath;
    if (QFileInfo::exists(qrcPath)) {
        return qrcPath;
    }

    // 2. Relative to working directory
    if (QFileInfo::exists(relativePath)) {
        return relativePath;
    }

    // 3. Parent relative directories (useful when running from build/subfolders)
    QString parent1 = "../" + relativePath;
    if (QFileInfo::exists(parent1)) return parent1;

    QString parent2 = "../../" + relativePath;
    if (QFileInfo::exists(parent2)) return parent2;

    // 4. Relative to application executable directory
    QString appDir = QCoreApplication::applicationDirPath();
    QString fromApp = appDir + "/" + relativePath;
    if (QFileInfo::exists(fromApp)) return fromApp;

    QString fromAppParent = appDir + "/../" + relativePath;
    if (QFileInfo::exists(fromAppParent)) return fromAppParent;

    QString fromAppGrandParent = appDir + "/../../" + relativePath;
    if (QFileInfo::exists(fromAppGrandParent)) return fromAppGrandParent;

    return QString();
}

bool SpriteManager::tryLoadPixmap(AssetId id, const QString& relativePath) {
    QString resolved = resolveAssetPath(relativePath);
    if (!resolved.isEmpty()) {
        QPixmap pm;
        if (pm.load(resolved)) {
            m_sprites[id] = pm;
            if (m_config.verboseLogging) {
                qDebug() << "[SpriteManager] Loaded" << relativePath << "from" << resolved;
            }
            return true;
        }
    }
    return false;
}

bool SpriteManager::loadAll() {
    bool allLoaded = true;

    // Vehicle
    allLoaded &= tryLoadPixmap(AssetId::Vehicle_Chassis, "assets/vehicle/chassis.png");
    allLoaded &= tryLoadPixmap(AssetId::Vehicle_Wheel, "assets/vehicle/wheel.png");
    allLoaded &= tryLoadPixmap(AssetId::Vehicle_DriverHead, "assets/vehicle/driver_head.png");

    // Terrain
    allLoaded &= tryLoadPixmap(AssetId::Terrain_GrassTop, "assets/terrain/grass_top.png");
    allLoaded &= tryLoadPixmap(AssetId::Terrain_DirtFill, "assets/terrain/dirt_fill.png");

    // Collectibles
    allLoaded &= tryLoadPixmap(AssetId::Collectible_Coin, "assets/collectibles/coin.png");
    allLoaded &= tryLoadPixmap(AssetId::Collectible_FuelCan, "assets/collectibles/fuel_can.png");

    // UI
    allLoaded &= tryLoadPixmap(AssetId::UI_PedalGasReleased, "assets/ui/pedal_gas_released.png");
    allLoaded &= tryLoadPixmap(AssetId::UI_PedalGasPressed, "assets/ui/pedal_gas_pressed.png");
    allLoaded &= tryLoadPixmap(AssetId::UI_PedalBrakeReleased, "assets/ui/pedal_brake_released.png");
    allLoaded &= tryLoadPixmap(AssetId::UI_PedalBrakePressed, "assets/ui/pedal_brake_pressed.png");
    allLoaded &= tryLoadPixmap(AssetId::UI_MeterDial, "assets/ui/meter_dial.png");
    allLoaded &= tryLoadPixmap(AssetId::UI_DialNeedle, "assets/ui/dial_needle.png");

    // Background
    allLoaded &= tryLoadPixmap(AssetId::Background_Sky, "assets/backgrounds/sky_bg.png");

    return allLoaded;
}

bool SpriteManager::hasAsset(AssetId id) const {
    auto it = m_sprites.find(id);
    return (it != m_sprites.end() && !it->second.isNull());
}

const QPixmap& SpriteManager::getPixmap(AssetId id) const {
    auto it = m_sprites.find(id);
    if (it != m_sprites.end()) {
        return it->second;
    }
    return m_nullPixmap;
}

} // namespace Render
