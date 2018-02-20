
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>


#include <ObjectReader/ObjectReader.h>

std::string TestName = "/Users/lanza/Projects/ldl/scrap/sample.pof";

using namespace testing;

TEST(ObjectFile, IsCreatable) {
  std::unique_ptr<ldl::ObjectFile> OF = std::make_unique<ldl::ObjectFile>(); 
  ASSERT_THAT(OF, NotNull());
}

TEST(ObjectReader, ReadsFileHeader) {
  ldl::ObjectReader OR{TestName};
  EXPECT_THAT(OR.ReadFileHeader(), Eq(true));

  ldl::FileHeader FH = OR.FH;

  EXPECT_THAT(FH.Magic, Eq("LINK"));
  EXPECT_THAT(FH.NumberOfSegments, Eq(3));
  EXPECT_THAT(FH.NumberOfSymbols, Eq(2));
  EXPECT_THAT(FH.NumberOfRelocations, Eq(4));
}

TEST(ObjectReader, ReadsASegmentHeader) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  EXPECT_THAT(OR.ReadSegmentHeader(), Eq(true));

  EXPECT_THAT(OR.Segments.size(), Eq(1));

  ldl::Segment S = OR.Segments[0];

  EXPECT_THAT(S.Name, Eq(".text"));
  EXPECT_THAT(S.Address, Eq(0x1000));
  EXPECT_THAT(S.Length, Eq(0x3c));
  EXPECT_THAT(S.Code, Eq("RP"));
}

TEST(ObjectReader, ReadsAllSegmentHeaders) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  EXPECT_THAT(OR.ReadSegmentHeaders(), Eq(true));

  EXPECT_THAT(OR.Segments.size(), Eq(3));
}

TEST(ObjectReader, ReadsASymbolTableEntry) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  OR.ReadSegmentHeaders();
  EXPECT_THAT(OR.ReadSymbolTableEntry(), Eq(true));

  EXPECT_THAT(OR.Symbols.size(), Eq(1));

  ldl::Symbol S = OR.Symbols[0];
  EXPECT_THAT(S.Name, Eq("main"));
  EXPECT_THAT(S.Value, Eq(0x5d12));
  EXPECT_THAT(S.SegmentNumber, Eq(1));
  EXPECT_THAT(S.Type, Eq("D"));
}

TEST(ObjectReader, ReadsSymbolTable) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  OR.ReadSegmentHeaders();
  EXPECT_THAT(OR.ReadSymbolTable(), Eq(true));

  EXPECT_THAT(OR.Symbols.size(), Eq(2));
}

TEST(ObjectReader, ReadsARelocationEntry) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  OR.ReadSegmentHeaders();
  OR.ReadSymbolTable();
  EXPECT_THAT(OR.ReadRelocationEntry(), Eq(true));

  EXPECT_THAT(OR.Relocations.size(), Eq(1));

  ldl::Relocation R = OR.Relocations[0];
  EXPECT_THAT(R.Location, Eq(0x1221));
  EXPECT_THAT(R.SegmentNumber, Eq(1));
  EXPECT_THAT(R.Ref, Eq(3));
  EXPECT_THAT(R.Type, Eq("A4"));
}

TEST(ObjectReader, ReadsRelocations) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  OR.ReadSegmentHeaders();
  OR.ReadSymbolTable();
  EXPECT_THAT(OR.ReadRelocations(), Eq(true));

  EXPECT_THAT(OR.Relocations.size(), Eq(4));
}

TEST(ObjectReader, ReadsDataForSegmentOne) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  OR.ReadSegmentHeaders();
  OR.ReadSymbolTable();
  OR.ReadRelocations();
  ldl::Segment& S = OR.Segments[0];
  EXPECT_THAT(OR.ReadDataForSegment(S), Eq(true));

  EXPECT_THAT(S.Data.size() / 2, Eq(S.Length));
}

TEST(ObjectReader, ReadsSegmentData) {
  ldl::ObjectReader OR{TestName};
  OR.ReadFileHeader();
  OR.ReadSegmentHeaders();
  OR.ReadSymbolTable();
  OR.ReadRelocations();
  EXPECT_THAT(OR.ReadSegmentData(), Eq(true));

  for (auto& S : OR.Segments)
    EXPECT_THAT(S.Data.size(), Gt(30));
}

TEST(ObjectReader, ReadFilePasses) {
  ldl::ObjectReader OR{TestName};
  EXPECT_THAT(OR.ReadFile(), Eq(true));
}
using ObjectFilePtr = std::unique_ptr<ldl::ObjectFile>;
TEST(ObjectReader, CreatesObjectFile) {
  ldl::ObjectReader OR{TestName};
  ObjectFilePtr OFPtr = OR.GetObjectFile();

  EXPECT_THAT(OFPtr->FH.Magic, Eq("LINK"));
  EXPECT_THAT(OFPtr->Segments.size(), Eq(3));
}

TEST(ObjectFile, GeneratesEquivalentFile) {
  std::ifstream IFS{TestName};
  std::string FileStringRepresentation(
    (std::istreambuf_iterator<char>(IFS)),
    (std::istreambuf_iterator<char>()));

  ldl::ObjectReader OR{TestName};
  ObjectFilePtr OFPtr = OR.GetObjectFile();
  std::string ObjectFileStringRepresentation = OFPtr->GenerateTextRepresentation();

  EXPECT_THAT(ObjectFileStringRepresentation, Eq(FileStringRepresentation));
}

//
