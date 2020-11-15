/*
* Copyright (c) 2018 Pawel Drzycimski
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include "cmdline/cmdline.h"
#include "simplelog/simplelog.h"
#include <stdexcept>

#include <fstream>

template<typename T>
struct CloseStream
{
  CloseStream(T &_stream) : stream(_stream) {}
  ~CloseStream() {stream.close();}

  T &stream;
};

struct SONidEntry
{
  std::string wholeEnumEntry;
  std::string enumLeftSide;
};

std::string toUpper(const std::string &str);
std::string getNid(const std::string &nidLine, std::string::size_type nidStartPos);
std::string stripNid(const std::string &nidLine);

std::string formTestSecion(const std::string &nidLine, const size_t pos);
SONidEntry formSoEntry(const std::string &nidLine, const size_t pos);
std::string formEnumLeftSideName(const SONidEntry &entry);
std::string formEnumCheck(const std::vector<std::string> &enumValueNames);

int main(int argc, char *argv[])
{
  cmdline::parser arg;
  arg.add("help", 'h', "Print help.");
  arg.add<std::string>("file", 'f', "Path to obj_mac.h", true);
    
  if(!arg.parse(argc, const_cast<const char* const*>(argv)))
  {
    const auto fullErr = arg.error_full();
    if(!fullErr.empty())
      log << fullErr;
     
    log << arg.usage();
    return 0;
  }
  
  if(arg.exist("help"))
  {
    log << arg.usage();
    return 0;
  } 

  if(!arg.exist("file"))
  {
    log << "--file or -f argument is mandatory!\n";
    log << arg.usage();
    return 0;
  }

  const std::string file = arg.get<std::string>("file");
  std::ifstream in(file); 
  if(!in.is_open())
  {
    log << "Can't open " << file;
    return 0;
  }
  const CloseStream<decltype(in)> inGuard(in);
  
  const std::string testsOutFile = "./nidstestcases.txt";
  std::ofstream testsOut(testsOutFile);
  if(!testsOut.is_open())
  {
    log << "Can't open " << testsOutFile;
    return 0;
  }
  const CloseStream<decltype(testsOut)> testsOutGuard(testsOut);

  const std::string soOutFile = "./sonamespace.txt";
  std::ofstream soOut(soOutFile);
  if(!testsOut.is_open())
  {
    log << "Can't open " << soOutFile;
    return 0;
  }
  const CloseStream<decltype(soOut)> soOutGuard(soOut);

  log << "Writing to " << testsOutFile;
  log << "Writing to " << soOutFile;

  testsOut << "const static NidUTInput NID_VALIDITY_UT_VALUES[] {\n";

  soOut << "enum class Nid : int\n{\n";
  size_t nidsWritten = 0;
  std::vector<std::string> enumLeftSideNames;
  for(std::string line; std::getline(in, line);)
  {
    if(const auto pos = line.find("NID_"); pos != std::string::npos)
    {
      try {
        testsOut << formTestSecion(line, pos);
        const auto soNidEntry = formSoEntry(line, pos);
        soOut << soNidEntry.wholeEnumEntry;
        enumLeftSideNames.push_back(formEnumLeftSideName(soNidEntry));

      }catch(const std::runtime_error &e){
        log << e.what();
        throw;
      }
      ++nidsWritten;
    }
  }
  testsOut << "};\n";
  soOut << "};\n\n";
  soOut << formEnumCheck(enumLeftSideNames) << "\n";
  log << "Written nids " << nidsWritten;

  return 0;
}

std::string formTestSecion(const std::string &nidLine, const size_t pos)
{
  std::string ret = " NidUTInput {\n";
  const auto nid = getNid(nidLine, pos);
  //log << "NID to be added " << nid;
  ret += "  " + nid + ", nid::Nid::" + toUpper(stripNid(nid));
  ret += "\n },\n";

  return ret;
}

SONidEntry formSoEntry(const std::string &nidLine, const size_t pos)
{ 
  const auto nid = getNid(nidLine, pos);
  const auto upperNid = toUpper(stripNid(nid));

  return {
    "  " + upperNid + " = " + nid + ",\n",
    upperNid
  };
}

std::string formEnumLeftSideName(const SONidEntry &entry)
{
  return "::so::nid::Nid::" + entry.enumLeftSide;
}

std::string formEnumCheck(const std::vector<std::string> &enumValueNames)
{
  std::string ret = "using NidEnumTest = EnumCheck<nid::Nid,";
  for(const auto &entry : enumValueNames)
  {
    ret += entry;
    ret += ",";
  }

  ret += ">;";

  return ret;
}

std::string getNid(const std::string &nidLine, std::string::size_type nidStartPos)
{
  //log << "Processing " << nidLine << " nid start " << nidStartPos;
  auto i = nidStartPos;
  while(std::isalnum(nidLine[i]) || nidLine[i] == '_')
      ++i;

  //log << "Nid end " << i;
  return nidLine.substr(nidStartPos, i - nidStartPos);
}


std::string stripNid(const std::string &nid)
{
  const auto pos = nid.find('_');
  if(pos == std::string::npos)
    throw std::runtime_error("No undersore in nid");

  return nid.substr(pos + 1);
}

std::string toUpper(const std::string &str)
{
  std::string ret(str);
  std::transform(ret.begin(), ret.end(), ret.begin(), ::toupper);

  return ret;
}