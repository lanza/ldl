#ifndef ObjectReader_h
#define ObjectReader_h

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>

namespace ldl {

class ObjectReader;

class FileHeader {
public:
  std::string Magic;
  int NumberOfSegments;
  int NumberOfSymbols;
  int NumberOfRelocations;
};


class Symbol {
public:
  std::string Name;
  int Value;
  int SegmentNumber;
  std::string Type;
};

class Relocation {
public:
  int Location;
  int SegmentNumber;
  int Ref;
  std::string Type;
};

class Segment {
public:
  std::string FileName;
  std::string Name;
  int Address;
  int Length;
  std::string Code;

  std::string Data;

  Segment(std::string FileName, std::string Name, int Address, int Length, std::string Code)
    :FileName{FileName},
    Name{Name},
    Address{Address},
    Length{Length},
    Code{Code} { }
  Segment() {}
};

class OutSegment : public Segment {
public:
  std::vector<Segment> InputSegments;
  std::vector<Segment> ContainedSegments;
  OutSegment(std::string FileName, std::string Name, int Address, int Length, std::string Code)
    :Segment{FileName, Name, Address, Length, Code} { }
};

class ObjectFile {
public:
  std::string FileName;
  FileHeader FH;
  std::vector<Symbol> Symbols;
  std::vector<Relocation> Relocations;
  std::vector<Segment> Segments;

  std::string GenerateTextRepresentation() {
    std::ostringstream OSS;
    OSS << FH.Magic << std::endl;
    OSS << FH.NumberOfSegments << " " << FH.NumberOfSymbols << " " << FH.NumberOfRelocations << std::endl;
    for (auto S : Segments)
      OSS << S.Name << " " << std::hex << S.Address << std::hex << " " << S.Length << " " << S.Code << std::endl;
    for (auto S : Symbols)
      OSS << S.Name << " " << S.Value << " " << S.SegmentNumber << " " << S.Type << std::endl;
    for (auto R : Relocations)
      OSS << R.Location << " " << R.SegmentNumber << " " << R.Ref << " " << R.Type << std::endl;
    for (auto S : Segments)
      OSS << S.Data << std::endl;

    return OSS.str();
  }
};

class ObjectReader {

public:
  std::string FileName;
  std::ifstream IFS;

  FileHeader FH;
  std::vector<Segment> Segments;
  std::vector<Symbol> Symbols;
  std::vector<Relocation> Relocations;

  ObjectReader(std::string FileName)
  : FileName{FileName},
    IFS{FileName}
  {
    IFS.exceptions(IFS.exceptions() | std::ios_base::badbit);
    if (!IFS.good()) {
      throw "IFS for file no good";
    }
  }

  using ObjectFilePtr = std::unique_ptr<ObjectFile>;

  ObjectFilePtr GetObjectFile() {
    bool ReadFileSuccess = ReadFile();
    if (ReadFileSuccess == false) throw "Read file failed";

    ObjectFilePtr OFPtr = std::make_unique<ObjectFile>();
    OFPtr->FH = std::move(FH);
    OFPtr->Segments = std::move(Segments);
    OFPtr->Symbols = std::move(Symbols);
    OFPtr->Relocations = std::move(Relocations);
    return OFPtr;
  }

  bool ReadFile() {
    bool ReadFileHeaderSuccess = ReadFileHeader();
    bool ReadSegmentHeadersSuccess = ReadSegmentHeaders();
    bool ReadSymbolTableSuccess = ReadSymbolTable();
    bool ReadRelocationsSuccess = ReadRelocations();
    bool ReadSegmentDataSuccess = ReadSegmentData();

    if (ReadFileHeaderSuccess
      && ReadSegmentHeadersSuccess
      && ReadSymbolTableSuccess
      && ReadRelocationsSuccess
      && ReadSegmentDataSuccess)
      return true;
    else
      return false;
  }

  bool ReadFileHeader() {
    IFS >> FH.Magic;
    IFS >> FH.NumberOfSegments;
    IFS >> FH.NumberOfSymbols;
    IFS >> FH.NumberOfRelocations;

    if (IFS.fail())
      return false;
    else
      return true;
  }

  bool ReadSegmentHeaders() {
    for (int i = 0; i < FH.NumberOfSegments; i++) {
      bool ReadSegmentHeaderSuccess = ReadSegmentHeader();
      if (ReadSegmentHeaderSuccess == false) return false;
    }
    return true;
  }

  bool ReadSegmentHeader() {
    Segment S;
    S.FileName = FileName;
    IFS >> S.Name;
    IFS >> std::hex >> S.Address;
    IFS >> std::hex >> S.Length;
    IFS >> S.Code;

    Segments.push_back(S);

    if (IFS.fail()) {
      return false;
    } else {
      return true;
    }
  }

  bool ReadSymbolTableEntry() {
    Symbol S;
    IFS >> S.Name;
    IFS >> std::hex >> S.Value;
    IFS >> S.SegmentNumber;
    IFS >> S.Type;

    Symbols.push_back(S);

    if (IFS.fail()) {
      return false;
    } else {
      return true;
    }
  }

  bool ReadSymbolTable() {
    for (int i = 0; i < FH.NumberOfSymbols; i++) {
      bool ReadSymbolTableEntrySuccess = ReadSymbolTableEntry();
      if (ReadSymbolTableEntrySuccess == false) return false;
    }
    return true;
  }

  bool ReadRelocationEntry() {
    Relocation R;
    IFS >> std::hex >> R.Location;
    IFS >> R.SegmentNumber;
    IFS >> R.Ref;
    IFS >> R.Type;

    Relocations.push_back(R);

    if (IFS.fail()) {
      return false;
    } else {
      return true;
    }
  }

  bool ReadRelocations() {
    for (int i = 0; i < FH.NumberOfRelocations; i++) {
      bool ReadRelocationEntrySuccess = ReadRelocationEntry();
      if (ReadRelocationEntrySuccess == false) return false;
    }
    return true;
  }

  bool ReadDataForSegment(Segment& S) {
    IFS >> S.Data;

    if (IFS.fail())
      return false;
    else
      return true;
  }

  bool ReadSegmentData() {
    for (auto& S : Segments) {
      bool ReadDataForSegmentSuccess = ReadDataForSegment(S);
      if (ReadDataForSegmentSuccess == false) return false;
    }
    return true;
  }
};
}

#endif
