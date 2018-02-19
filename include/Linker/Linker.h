#ifndef Linker_h
#define Linker_h

#include <string>
#include <vector>
#include <memory>
#include <map>

#include <ObjectReader/ObjectReader.h>

namespace ldl {

using ObjectFilePtr = std::unique_ptr<ObjectFile>;

  class SegmentMapping {
  public:
    std::string SegmentName;
    int Start;
    int End;
    SegmentMapping() {}
    SegmentMapping(std::string SegmentName, int Start, int End)
      :SegmentName{SegmentName},
      Start{Start},
      End{End} {}
    };

  class Linker {
  public:
    using ObjectFileMapping = std::map<std::string, SegmentMapping>;
    std::map<std::string, ObjectFileMapping> Mappings;
    std::vector<std::string> FileNames;
    std::vector<ObjectFilePtr> ObjectFiles;

    std::vector<Symbol> MergedSymbols;
    std::vector<Relocation> MergedRelocation;
    std::vector<Segment> MergedSegments;

    Linker(std::vector<std::string> FileNames) :FileNames{FileNames} { }
    Linker() { }

    void ReadFiles() {
      for (auto& FileName : FileNames) {
        ObjectReader OR{FileName};
        auto OFPtr = OR.GetObjectFile();
        ObjectFiles.push_back(std::move(OFPtr));
      }

      for (auto& OF : ObjectFiles) {
        Mappings[OF->FileName] = ObjectFileMapping{};
      }
    }

    ObjectFilePtr GenerateObjectFile() {
      GenerateOutputFileSegments();
      GenerateOutputFileSymbolTable();
      ObjectFilePtr OFPtr = ObjectFilePtr{new ObjectFile{}};
      OFPtr->FH = FileHeader{"LINK", 3, 0, 0};
      OFPtr->Segments = {TextSegment, DataSegment, BSSSegment};

      return OFPtr;
    }

    void MergeSegmentsIntoOneVector() {
      for (auto& OFPtr : ObjectFiles) {
        MergedSegments.insert(MergedSegments.end(), OFPtr->Segments.begin(), OFPtr->Segments.end());
      }
    }

    OutSegment TextSegment{"a.out", ".text", 0x1000, 0x0, "RP"};
    OutSegment DataSegment{"a.out", ".data", 0x0, 0x0, "RWP"};
    OutSegment BSSSegment{"a.out", ".bss", 0x0, 0x0, "RW"};
    Segment CommonSegment{"NA", ".common", 0x0, 0x0, "RWP"};
    std::vector<OutSegment> OtherSegments;


    void GenerateOutputFileSymbolTable() {
      MergeSymbolsIntoOneVector();
      GatherCommonSymbols();
      CombineCommonSymbolsIntoCommonSegment();
      AppendBSSSegment(CommonSegment);
    }

    void CombineCommonSymbolsIntoCommonSegment() {
      for (auto& CS : CommonSymbols) {
        CommonSegment.Length += CS.Value;
      }
    }

    std::vector<Symbol> CommonSymbols;

    void GatherCommonSymbols() {
      for (auto& S : MergedSymbols) {
        if (S.Type == "U" && S.Value != 0)
          CommonSymbols.push_back(S);
      }
    }

    void MergeSymbolsIntoOneVector() {
      for (auto& OFPtr : ObjectFiles) {
        MergedSymbols.insert(MergedSymbols.end(), OFPtr->Symbols.begin(), OFPtr->Symbols.end());
      }
    }

    void GenerateOutputFileSegments() {
      MergeSegmentsIntoOneVector();
      InitializeTextSegment();
      MergeTextSegments();
      FinalizeTextSegment();
      InitializeDataSegment();
      MergeDataSegments();
      FinalizeDataSegment();
      InitializeBSSSegment();
      MergeBSSSegments();
      FinalizeBSSSegment();
    }

    void MergeTextSegments() {
      for (auto& S : MergedSegments) {
        if (S.Name == ".text")
          AppendTextSegment(S);
      }
    }
    void MergeDataSegments() {
      for (auto& S : MergedSegments) {
        if (S.Name == ".data")
          AppendDataSegment(S);
    }
    }
    void MergeBSSSegments() {
      for (auto& S : MergedSegments) {
        if (S.Name == ".bss")
          AppendBSSSegment(S);
      }
    }


    void AppendTextSegment(Segment S) {
      AppendSegmentToOutSegment(S, TextSegment);
    }
    void AppendDataSegment(Segment S) {
      AppendSegmentToOutSegment(S, DataSegment);
    }
    void AppendBSSSegment(Segment S) {
      AppendSegmentToOutSegment(S, BSSSegment);
    }


    void AppendSegmentToOutSegment(Segment S, OutSegment& O) {
      O.InputSegments.push_back(S);
      Segment ContainedSegment = S;

      int ModuloFour = O.Length % 4;
      int BlankSpaceSize = (4 - ModuloFour) % 4;
      ContainedSegment.Address = O.Address + O.Length + BlankSpaceSize;
      O.Length += BlankSpaceSize + ContainedSegment.Length;
      for (int i = 0; i < BlankSpaceSize; i++)
        O.Data += "00";
      O.Data += ContainedSegment.Data;

      O.ContainedSegments.push_back(ContainedSegment);

      Mappings[S.FileName][S.Name] = SegmentMapping(S.Name, ContainedSegment.Address, ContainedSegment.Address + ContainedSegment.Length -1);
    }

    void InitializeTextSegment() {
    }
    void FinalizeTextSegment() {}
    void InitializeDataSegment() {

      DataSegment.Address = ((TextSegment.Address + TextSegment.Length) & 0xF000) + 0x1000;
    }
    void FinalizeDataSegment() {
      int ModuloFour = DataSegment.Length % 4;
      int BlankSpaceSize = (4 - ModuloFour) % 4;

      DataSegment.Length += BlankSpaceSize;
      for (int i = 0; i < BlankSpaceSize; i++)
        DataSegment.Data += "00";
    }
    void InitializeBSSSegment() {
      BSSSegment.Address = DataSegment.Address + DataSegment.Length;
    }
    void FinalizeBSSSegment() {}
  };
}

#endif
