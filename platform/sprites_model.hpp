#pragma once

#include "lib/magic_enum.hpp"
#include "processor/nes_ppu.hpp"

#include <QAbstractListModel>
#include <QStringList>

#include <sstream>

class SpriteImageProvider : public QQuickImageProvider
{
public:
    SpriteImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        std::string idstr = id.toStdString();

        // The id is provided in the format "<image index>_<unique number>"
        size_t pos = idstr.find('+');
        std::string image_num = idstr.substr(0, pos);

        return sprite_images_[std::stoi(image_num)];
    }

    std::vector<QImage> last_sprite_images_;
    std::vector<QImage> sprite_images_;
    NesPPU::Sprite::Canvas dummy_canvas_;
};

class SpritesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit SpritesModel(QObject *parent = nullptr) {}

    enum class DataRoles
    {
        SpriteIndex = Qt::UserRole + 1,
        SpritePosition,
        SpriteAttributes,
        SpriteTileIndex,
        SpriteTileImage,

        LAST_ROLE_INDEX
    };

    void update(const std::vector<NesPPU::Sprite>& sprite_data);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return sprite_data_.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

    void addData(const QString &text);

    SpriteImageProvider* image_provider() { return &image_provider_; }

private:
    QStringList dataList;
    std::vector<NesPPU::Sprite> sprite_data_;
    std::vector<QImage> sprite_images_;
    SpriteImageProvider image_provider_;

    int32_t update_count_;
};
