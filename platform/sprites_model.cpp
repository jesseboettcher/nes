#include "platform/sprites_model.hpp"

namespace magic_enum::customize
{
    template <>
    struct magic_enum::customize::enum_range<SpritesModel::DataRoles>
    {
        static constexpr int min = Qt::UserRole + 1;
        static constexpr int max = static_cast<int>(SpritesModel::DataRoles::LAST_ROLE_INDEX);
        // (max - min) must be less than UINT16_MAX.
    };
}

SpriteImageProvider::SpriteImageProvider()
 : QQuickImageProvider(QQuickImageProvider::Image)
{
    for (int32_t i = 0;i < 8*8;i++)
    {
        dummy_canvas_[i/8][i%8] = NesDisplay::Color({0, 0, 0, 0});
    }
}

void SpritesModel::update(const std::vector<NesPPU::Sprite>& sprite_data)
{
    beginResetModel();
    sprite_data_ = sprite_data;

    image_provider_.sprite_images_.clear();

    for (auto& s : sprite_data_)
    {
        const uchar* buffer = s.canvas ? (const uchar*)s.canvas.get() : (const uchar*)image_provider_.dummy_canvas_;
        QImage img = QImage(buffer, 8, 8, QImage::Format_RGBA8888);
        image_provider_.sprite_images_.push_back(img);
    }

    endResetModel();
}

QHash<int, QByteArray> SpritesModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    for (auto& r : magic_enum::enum_values<DataRoles>())
    {
        roles[static_cast<int>(r)] = QByteArray(magic_enum::enum_name<DataRoles>(r).data());
    }
    return roles;
}

QVariant SpritesModel::data(const QModelIndex &index, int role) const
{
    std::stringstream strstream;

    switch (static_cast<DataRoles>(role))
    {
        case DataRoles::SpriteIndex:
            strstream << index.row();
            break;

        case DataRoles::SpritePosition:
            strstream << "(" << static_cast<int32_t>(sprite_data_[index.row()].x_pos)
                      << ", " << static_cast<int32_t>(sprite_data_[index.row()].y_pos) << ")";
            break;

        case DataRoles::SpriteAttributes:
            strstream << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int32_t>(sprite_data_[index.row()].attributes);
            break;

        case DataRoles::SpriteTileIndex:
            strstream << static_cast<int32_t>(sprite_data_[index.row()].tile_index);
            break;

        case DataRoles::SpriteTileImage:
            strstream << "image://sprite_image_provider/" << index.row();
            break;

        case DataRoles::LAST_ROLE_INDEX:
            assert(false);
            break;

    }
    return QString::fromStdString(strstream.str());
}
