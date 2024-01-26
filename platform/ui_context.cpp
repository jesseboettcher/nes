#include "platform/ui_context.hpp"

void UIContext::configure_registers_window()
{
    QRect main_rect = main_window->geometry();
    registers_window->setX(main_rect.x() + main_rect.width() + 20);
    registers_window->setY(main_rect.y());
}

void UIContext::configure_memory_window()
{
    QRect main_rect = main_window->geometry();
    memory_window->setX(main_rect.x() + main_rect.width() + 20);
    memory_window->setY(main_rect.y() + main_rect.height() - memory_window->geometry().height());
}
