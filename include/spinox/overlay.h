#pragma once
namespace spinox::overlay {
void init();
void toggle();
bool isOpen();
void setTouch(int action, float x, float y);
}
