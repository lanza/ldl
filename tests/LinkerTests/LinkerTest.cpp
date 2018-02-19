#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Linker/Linker.h>
#include <ObjectReader/ObjectReader.h>

using namespace ::testing;

class LinkerTest : public Test {
public:
  ldl::Linker L;
protected:
  virtual void SetUp() {
    L.FileNames = {
      "/Users/lanza/Projects/ldl/scrap/linkertest1.pof",
      "/Users/lanza/Projects/ldl/scrap/linkertest1.pof"
    };

    L.ReadFiles();
  }

  virtual void TearDown() {
  }
};

using ObjectFilePtr = std::unique_ptr<ldl::ObjectFile>;

TEST_F(LinkerTest, GathersCommonSymbols) {
  L.MergeSymbolsIntoOneVector();
  EXPECT_THAT(L.MergedSymbols.size(), Eq(4));

  L.GatherCommonSymbols();
  EXPECT_THAT(L.CommonSymbols.size(), Eq(2));
}

TEST_F(LinkerTest, CreatsCommonSegment) {
  L.MergeSymbolsIntoOneVector();
  L.GatherCommonSymbols();
  L.CombineCommonSymbolsIntoCommonSegment();

  EXPECT_THAT(L.CommonSegment.Length, Eq(0x8));
}

TEST_F(LinkerTest, AppendsCommonSegmentOntoBSS) {
  L.GenerateOutputFileSymbolTable();
  L.GenerateOutputFileSegments();

  ldl::OutSegment& B = *L.BSSSegment;

  EXPECT_THAT(B.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3));
  EXPECT_THAT(B.Length, Eq(0x34 + 0x34 + 0x8));
  EXPECT_THAT(B.ContainedSegments.size(), Eq(3));
  EXPECT_THAT(B.InputSegments.size(), Eq(3));

  ldl::Segment Common = B.ContainedSegments[2];

  EXPECT_THAT(Common.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3 + 0x34 + 0x34));
  EXPECT_THAT(Common.Length, Eq(0x8));
}

TEST_F(LinkerTest, ReadsFiles) {
  EXPECT_THAT(L.ObjectFiles.size(), Eq(2));
}


TEST_F(LinkerTest, MergesTextSegments) {
  L.MergeSegmentsIntoDataStructure();
  L.MergeRPSegments();

  ldl::OutSegment& T = *L.TextSegment;

  EXPECT_THAT(T.Address, Eq(0x1000));
  EXPECT_THAT(T.Length, Eq(0x3b + 0x1 + 0x3b));
  EXPECT_THAT(T.ContainedSegments.size(), Eq(2));
  EXPECT_THAT(T.InputSegments.size(), Eq(2));

  ldl::Segment C = T.ContainedSegments[0];
  EXPECT_THAT(C.Address, Eq(0x1000));
  EXPECT_THAT(C.Length, Eq(0x3b));

  ldl::Segment C2 = T.ContainedSegments[1];

  EXPECT_THAT(C2.Address, Eq(0x1000 + 0x3b + 0x1));
  EXPECT_THAT(C2.Length, Eq(0x3b));
}

TEST_F(LinkerTest, MergesDataSegments) {
  L.MergeSegmentsIntoDataStructure();
  L.MergeRPSegments();
  L.MergeRWPSegments();

  ldl::OutSegment& D = *L.DataSegment;

  EXPECT_THAT(D.Address, Eq(0x2000));
  EXPECT_THAT(D.Length, Eq(0x29 + 0x3 + 0x29));
  EXPECT_THAT(D.ContainedSegments.size(), Eq(2));
  EXPECT_THAT(D.InputSegments.size(), Eq(2));

  ldl::Segment C = D.ContainedSegments[0];

  EXPECT_THAT(C.Address, Eq(0x2000));
  EXPECT_THAT(C.Length, Eq(0x29));

  ldl::Segment C2 = D.ContainedSegments[1];

  EXPECT_THAT(C2.Address, Eq(0x2000 + 0x29 + 0x3));
  EXPECT_THAT(C2.Length, Eq(0x29));
}

TEST_F(LinkerTest, FinalizesDataSegmentProperly) {
  L.MergeSegmentsIntoDataStructure();
  L.MergeRPSegments();
  L.MergeRWPSegments();

  ldl::OutSegment& D = *L.DataSegment;

  EXPECT_THAT(D.Length, Eq(0x29 + 0x3 + 0x29));
}

TEST_F(LinkerTest, InitializesBSSandFinalizesDataProperly) {
  L.MergeSegmentsIntoDataStructure();
  L.MergeRPSegments();
  L.MergeRWPSegments();
  L.MergeRWSegments();

  ldl::OutSegment& D = *L.DataSegment;
  ldl::OutSegment& B = *L.BSSSegment;

  EXPECT_THAT(D.Length, Eq(0x29 + 0x3 + 0x29 + 0x3));
  EXPECT_THAT(B.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3));
}

TEST_F(LinkerTest, MergesBSSSegments) {
  L.MergeSegmentsIntoDataStructure();
  L.MergeRPSegments();
  L.MergeRWPSegments();
  L.MergeRWSegments();

  ldl::OutSegment& B = *L.BSSSegment;

  EXPECT_THAT(B.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3));
  EXPECT_THAT(B.Length, Eq(0x34 + 0x34));
  EXPECT_THAT(B.ContainedSegments.size(), Eq(3));
  EXPECT_THAT(B.InputSegments.size(), Eq(3));

  ldl::Segment C = B.ContainedSegments[0];

  EXPECT_THAT(C.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3));
  EXPECT_THAT(C.Length, Eq(0x34));

  ldl::Segment C2 = B.ContainedSegments[1];

  EXPECT_THAT(C2.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3 + 0x34));
  EXPECT_THAT(C2.Length, Eq(0x34));
}

TEST_F(LinkerTest, MergesAllSegments) {
  L.GenerateOutputFileSegments();

  ldl::OutSegment& T = *L.TextSegment;

  EXPECT_THAT(T.Address, Eq(0x1000));
  EXPECT_THAT(T.Length, Eq(0x3b + 0x1 + 0x3b));
  EXPECT_THAT(T.ContainedSegments.size(), Eq(2));
  EXPECT_THAT(T.InputSegments.size(), Eq(2));

  ldl::Segment TC2 = T.ContainedSegments[1];

  EXPECT_THAT(TC2.Address, Eq(0x1000 + 0x3b + 0x1));
  EXPECT_THAT(TC2.Length, Eq(0x3b));

  ldl::OutSegment& D = *L.DataSegment;

  EXPECT_THAT(D.Address, Eq(0x2000));
  EXPECT_THAT(D.Length, Eq(0x29 + 0x3 + 0x29 + 0x3));
  EXPECT_THAT(D.ContainedSegments.size(), Eq(2));
  EXPECT_THAT(D.InputSegments.size(), Eq(2));

  ldl::Segment DC2 = D.ContainedSegments[1];

  EXPECT_THAT(DC2.Address, Eq(0x2000 + 0x29 + 0x3));
  EXPECT_THAT(DC2.Length, Eq(0x29));

  ldl::OutSegment& B = *L.BSSSegment;

  EXPECT_THAT(B.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3));
  EXPECT_THAT(B.Length, Eq(0x34 + 0x34));
  EXPECT_THAT(B.ContainedSegments.size(), Eq(3));
  EXPECT_THAT(B.InputSegments.size(), Eq(3));

  ldl::Segment BC2 = B.ContainedSegments[1];

  EXPECT_THAT(BC2.Address, Eq(0x2000 + 0x29 + 0x3 + 0x29 + 0x3 + 0x34));
  EXPECT_THAT(BC2.Length, Eq(0x34));
}

TEST_F(LinkerTest, GeneratesAnObjectFile) {
  ObjectFilePtr OFPtr = L.GenerateObjectFile();
  ldl::FileHeader FH{"LINK", 3, 0, 0};

  EXPECT_THAT(OFPtr->FH.Magic, Eq(FH.Magic));
  EXPECT_THAT(OFPtr->Segments.size(), Eq(3));
}

class LinkerMatchesBookTest : public Test {
public:
  ldl::Linker L;
  std::string main = "/Users/lanza/Projects/ldl/scrap/main.pof";
  std::string calif = "/Users/lanza/Projects/ldl/scrap/calif.pof";
  std::string mass = "/Users/lanza/Projects/ldl/scrap/mass.pof";
  std::string newyork = "/Users/lanza/Projects/ldl/scrap/newyork.pof";
protected:
  virtual void SetUp() {
    L.FileNames = { main, calif, mass, newyork };

    L.ReadFiles();
  }
};

TEST_F(LinkerMatchesBookTest, FilesAreCorrect) {
  for (auto& OFPtr : L.ObjectFiles) {
    if (OFPtr->FileName.find("main") != std::string::npos) {
      auto& Segments = OFPtr->Segments;
      for (auto& S : Segments) {
        if (S.Name == ".text") {
          EXPECT_THAT(S.Address, Eq(1000));
          EXPECT_THAT(S.Length, Eq(1017));
        } else if (S.Name == ".data") {
          EXPECT_THAT(S.Address, Eq(3000));
          EXPECT_THAT(S.Length, Eq(320));
        } else if (S.Name == ".bss") {
          EXPECT_THAT(S.Address, Eq(3320));
          EXPECT_THAT(S.Length, Eq(50));
        }
        EXPECT_THAT(S.Data.size() / 2, Eq(S.Length));
      }
    } else if (OFPtr->FileName.find("calif") != std::string::npos) {
      auto& Segments = OFPtr->Segments;
      for (auto& S : Segments) {
        if (S.Name == ".text") {
          EXPECT_THAT(S.Address, Eq(1000));
          EXPECT_THAT(S.Length, Eq(290));
        } else if (S.Name == ".data") {
          EXPECT_THAT(S.Address, Eq(2000));
          EXPECT_THAT(S.Length, Eq(127));
        } else if (S.Name == ".bss") {
          EXPECT_THAT(S.Address, Eq(2218));
          EXPECT_THAT(S.Length, Eq(100));
        }
        EXPECT_THAT(S.Data.size() / 2, Eq(S.Length));
      }
    } else if (OFPtr->FileName.find("mass") != std::string::npos) {
      auto& Segments = OFPtr->Segments;
      for (auto& S : Segments) {
        if (S.Name == ".text") {
          EXPECT_THAT(S.Address, Eq(1000));
          EXPECT_THAT(S.Length, Eq(615));
        } else if (S.Name == ".data") {
          EXPECT_THAT(S.Address, Eq(2000));
          EXPECT_THAT(S.Length, Eq(300));
        } else if (S.Name == ".bss") {
          EXPECT_THAT(S.Address, Eq(2300));
          EXPECT_THAT(S.Length, Eq(840));
        }
        EXPECT_THAT(S.Data.size() / 2, Eq(S.Length));
      }
    } else if (OFPtr->FileName.find("newyork") != std::string::npos) {
      auto& Segments = OFPtr->Segments;
      for (auto& S : Segments) {
        if (S.Name == ".text") {
          EXPECT_THAT(S.Address, Eq(1000));
          EXPECT_THAT(S.Length, Eq(1390));
        } else if (S.Name == ".data") {
          EXPECT_THAT(S.Address, Eq(3000));
          EXPECT_THAT(S.Length, Eq(1213));
        } else if (S.Name == ".bss") {
          EXPECT_THAT(S.Address, Eq(4214));
          EXPECT_THAT(S.Length, Eq(1400));
        }
        EXPECT_THAT(S.Data.size() / 2, Eq(S.Length));
      }
    }
  }
}

TEST_F(LinkerMatchesBookTest, GeneratesCorrectObjectFile) {
    auto OFPtr = L.GenerateObjectFile();

    auto& M = L.Mappings;

    auto& Main = M[main];
    auto& MainText = Main[".text"];
    auto& MainData = Main[".data"];
    auto& MainBSS = Main[".bss"];
    auto& Calif = M[calif];
    auto& CalifText = Calif[".text"];
    auto& CalifData = Calif[".data"];
    auto& CalifBSS = Calif[".bss"];
    auto& Mass = M[mass];
    auto& MassText = Mass[".text"];
    auto& MassData = Mass[".data"];
    auto& MassBSS = Mass[".bss"];
    auto& NewYork = M[newyork];
    auto& NewYorkText = NewYork[".text"];
    auto& NewYorkData = NewYork[".data"];
    auto& NewYorkBSS = NewYork[".bss"];


    EXPECT_THAT(MainText.Start, Eq(0x1000));
    EXPECT_THAT(MainText.End, Eq(0x2016));
    EXPECT_THAT(CalifText.Start, Eq(0x2018));
    EXPECT_THAT(CalifText.End, Eq(0x2937));
    EXPECT_THAT(MassText.Start, Eq(0x2938));
    EXPECT_THAT(MassText.End, Eq(0x2f4c));
    EXPECT_THAT(NewYorkText.Start, Eq(0x2f50));
    EXPECT_THAT(NewYorkText.End, Eq(0x42df));

    EXPECT_THAT(MainData.Start, Eq(0x5000));
    EXPECT_THAT(MainData.End, Eq(0x531f));
    EXPECT_THAT(CalifData.Start, Eq(0x5320));
    EXPECT_THAT(CalifData.End, Eq(0x5446));
    EXPECT_THAT(MassData.Start, Eq(0x5448));
    EXPECT_THAT(MassData.End, Eq(0x5747));
    EXPECT_THAT(NewYorkData.Start, Eq(0x5748));
    EXPECT_THAT(NewYorkData.End, Eq(0x695a));

    EXPECT_THAT(MainBSS.Start, Eq(0x695c));
    EXPECT_THAT(MainBSS.End, Eq(0x69ab));
    EXPECT_THAT(CalifBSS.Start, Eq(0x69ac));
    EXPECT_THAT(CalifBSS.End, Eq(0x6aab));
    EXPECT_THAT(MassBSS.Start, Eq(0x6aac));
    EXPECT_THAT(MassBSS.End, Eq(0x72eb));
    EXPECT_THAT(NewYorkBSS.Start, Eq(0x72ec));
    EXPECT_THAT(NewYorkBSS.End, Eq(0x86eb));
}













//
