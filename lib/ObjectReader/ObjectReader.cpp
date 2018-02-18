#include <fstream>
#include <string>
#include <vector>

namespace pof {

class FileHeader {
};


class Symbol {
};

class Relocation {
};

class Segment {

};

class ObjectFile {
  std::string FileName;
  FileHeader FH;
  std::vector<Symbol> Symbols;
  std::vector<Relocation> Relocations;
  std::vector<Segment> Segments;

  ObjectFile(std::string FileName) :FileName{FileName} {
    std::ifstream ifs{FileName};
    

  }
};


}
