#include <io/io_event_loop.h>
#include <object/signals.h>

using namespace pp;

class Label : public object {
public:
  Label() {}

  void updateValue(int value) {
    // valueChanged.emit(value);
    emit(valueChanged, value);
    // valueChanged.emit(value);
    // valueChanged2.emit(value, "asassas");
  }
  signal<void(int)> valueChanged;
  // def_signal(public, valueChanged, int value);
  // def_signal(public, valueChanged2, int value, const char* property);
};
class LineEdit : public object {
public:
  LineEdit() {}
  void setValue(int a) { printf("%d\n", a); }
};

void fff(int a) { printf("%d\n", a); }

int main(int argc, char *argv[]) {
  io::event_loop loop;
  Label label;
  LineEdit edit;

  object::connect(&label, label.valueChanged, &edit, &LineEdit::setValue);
  // object::connect<void(int)>(&label, &label.valueChanged, fff);
  // object::connect<void(int)>(&label, label.valueChanged, fff);
  label.updateValue(10);
  return 0;
}
