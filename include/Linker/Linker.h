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
    using Permissions = std::string;
    using SegmentName = std::string;
    std::map<Permissions, std::map<SegmentName, std::vector<Segment>>> SegmentDataStructure;


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
      GenerateOutputFileSymbolTable();
      GenerateOutputFileSegments();
      ObjectFilePtr OFPtr = std::make_unique<ObjectFile>();
      OFPtr->FH = FileHeader{
        "LINK",
        static_cast<int>(RPSegments.size() + RWPSegments.size() + RWSegments.size()),
        static_cast<int>(MergedSymbols.size()),
        static_cast<int>(MergedRelocation.size())
      };
      std::vector<Segment> Ss;
      Ss.insert(Ss.end(), RPSegments.begin(), RPSegments.end());
      Ss.insert(Ss.end(), RWPSegments.begin(), RWPSegments.end());
      Ss.insert(Ss.end(), RWSegments.begin(), RWSegments.end());
      OFPtr->Segments = std::move(Ss);

      return OFPtr;
    }

    void MergeSegmentsIntoDataStructure() {
      for (auto& OFPtr : ObjectFiles) {
        for (auto& S : OFPtr->Segments) {
          SegmentDataStructure[S.Code][S.Name].push_back(S);
        }
      }
    }

    OutSegment* TextSegment;
    std::vector<OutSegment> RPSegments;
    OutSegment* DataSegment;
    std::vector<OutSegment> RWPSegments;
    OutSegment* BSSSegment;
    Segment CommonSegment{"NA", ".common", 0x0, 0x0, "RW"};
    std::vector<OutSegment> RWSegments;


    void GenerateOutputFileSymbolTable() {
      MergeSymbolsIntoOneVector();
      GatherCommonSymbols();
      CombineCommonSymbolsIntoCommonSegment();
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
      MergeSegmentsIntoDataStructure();

      MergeRPSegments();
      MergeRWPSegments();
      MergeRWSegments();
    }

    void MergeRPSegments() {
      auto& RPSegmentsDataStructure = SegmentDataStructure["RP"];
      auto TextSegments = RPSegmentsDataStructure[".text"];
      RPSegmentsDataStructure.erase(".text");
      RPSegments.push_back(OutSegment{"a.out", ".text", 0x1000, 0x0, "RP"});
      TextSegment = &RPSegments.front();

      for (auto& S : TextSegments)
        AppendTextSegment(S);


      for (auto& NameVectorPair : RPSegmentsDataStructure) {
        OutSegment RPSegment{"a.out", NameVectorPair.first, 0x0, 0x0, "RP"};
        InitializeSegmentForSamePage(RPSegment, RPSegments.back());
        for (auto& S : NameVectorPair.second)
          AppendSegmentToOutSegment(S, RPSegment);
        RPSegments.push_back(RPSegment);
      }
    }

    void MergeRWPSegments() {
      auto& RWPSegmentsDataStructure = SegmentDataStructure["RWP"];
      auto DataSegments = RWPSegmentsDataStructure[".data"];
      RWPSegmentsDataStructure.erase(".data");
      RWPSegments.push_back(OutSegment{"a.out", ".data", 0x0, 0x0, "RWP"});
      DataSegment = &RWPSegments.front();

      InitializeSegmentForNewPage(*DataSegment, RPSegments.back());
      for (auto& S : DataSegments)
        AppendDataSegment(S);

      for (auto& NameVectorPair : RWPSegmentsDataStructure) {
        OutSegment RWPSegment{"a.out", NameVectorPair.first, 0x0, 0x0, "RWP"};
        InitializeSegmentForSamePage(RWPSegment, RWPSegments.back());
        for (auto& S : NameVectorPair.second)
          AppendSegmentToOutSegment(S, RWPSegment);
        RWPSegments.push_back(RWPSegment);
      }
    }

    void MergeRWSegments() {
      auto& RWSegmentsDataStructure = SegmentDataStructure["RW"];
      auto BSSSegments = RWSegmentsDataStructure[".bss"];
      RWSegmentsDataStructure.erase(".bss");
      RWSegments.push_back(OutSegment{"a.out", ".bss", 0x0, 0x0, "RW"});
      BSSSegment = &RWSegments.front();

      InitializeSegmentForSamePage(*BSSSegment, RWPSegments.back());
      for (auto& S : BSSSegments)
        AppendBSSSegment(S);

      AppendBSSSegment(CommonSegment);

      for (auto& NameVectorPair : RWSegmentsDataStructure) {
        OutSegment RWSegment{"a.out", NameVectorPair.first, 0x0, 0x0, "RW"};
        InitializeSegmentForSamePage(RWSegment, RWSegments.back());
        for (auto& S : NameVectorPair.second)
          AppendSegmentToOutSegment(S, RWSegment);
        RWSegments.push_back(RWSegment);
      }
    }

    void InitializeSegmentForSamePage(OutSegment& Next, OutSegment& Previous) {
      int ModuloFour = Previous.Length % 4;
      int BlankSpaceSize = (4 - ModuloFour) % 4;

      Previous.Length += BlankSpaceSize;
      for (int i = 0; i < BlankSpaceSize; i++)
        Previous.Data += "00";
      Next.Address = Previous.Address + Previous.Length;
    }
    void InitializeSegmentForNewPage(OutSegment& Next, OutSegment& Previous) {
      Next.Address = ((Previous.Address + Previous.Length) & 0xF000) + 0x1000;
    }

    void AppendTextSegment(Segment S) {
      AppendSegmentToOutSegment(S, *TextSegment);
    }
    void AppendDataSegment(Segment S) {
      AppendSegmentToOutSegment(S, *DataSegment);
    }
    void AppendBSSSegment(Segment S) {
      AppendSegmentToOutSegment(S, *BSSSegment);
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
  };
}

#endif
