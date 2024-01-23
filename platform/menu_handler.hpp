#include "system/nes.hpp"

#include <QObject>

#include <filesystem>

class MenuHandler : public QObject
{
    Q_OBJECT

public:
    MenuHandler(std::shared_ptr<Nes>& nes) : QObject(nullptr), nes_(nes) {}

public slots:
    void load_rom();

    void run();
    void command();

private:
	void start_nes(std::filesystem::path path);

	std::shared_ptr<Nes> nes_;
};
