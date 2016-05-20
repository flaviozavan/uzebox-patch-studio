#include <wx/treectrl.h>
#include "structdata.h"

StructData::StructData() {
};

StructData::StructData(const StructData *s) :
  data(s->data) {
}
