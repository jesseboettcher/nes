#include "system/nes.hpp"

#include <QObject>
#include <QWindow>

#include <filesystem>

class MenuHandler : public QObject
{
    Q_OBJECT

public:
    MenuHandler(std::shared_ptr<Nes>& nes, QWindow* memory_window);

public slots:
    void load_rom();

    void run();
    void step();
    void stop();

    void goto_memory();
    void command();

private:
	void start_nes(std::filesystem::path path);

	std::shared_ptr<Nes> nes_;
    QQuickItem * scroll_view_{nullptr};
};
