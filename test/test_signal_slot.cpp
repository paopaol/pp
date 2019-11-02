#include <iostream>
#include <object/signals.h>
#include <thread>

using namespace std;

class MyButton : public object {
public:
  MyButton(object *parent = nullptr) : object(parent) {}
  signal<void(int)> valueChanged;

public:
  void clickButton(int value) { valueChanged.emit(value); }
};

class MyLabel : public object {
public:
  MyLabel(object *parent = nullptr) : object(parent) {}
  void setValue(int value) { printf("value is %d\n", value); }
};

int main(int argc, char *argv[]) {
  object parent;

  MyButton btn(&parent);
  MyButton *btn2;
  MyLabel label;

  std::thread t([&]() { btn2 = new MyButton; });

  t.detach();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  object::connect(btn2, btn.valueChanged, &label, &MyLabel::setValue);
  btn.clickButton(10);
  btn.clickButton(40);
  btn.clickButton(60);
  return 0;
}
