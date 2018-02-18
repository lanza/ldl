

#include <iostream>
#include <fstream>
#include <libelf/libelf.h>

class ElfReader {



};

class ElfBinary {
  std::string FileName;

  ElfBinary(std::string FileName) :FileName{FileName} { }

  void ReadFile() {
    std::ifstream ifs{FileName, std::ios::binary | std::ios::ate};
    int size = ifs.tellg();
  }


};
