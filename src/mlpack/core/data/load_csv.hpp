/**
 * @file load_csv.hpp
 * @author ThamNgapWei
 *
 * This is a csv parsers which use to parse the csv file format
 */
#ifndef MLPACK_CORE_DATA_LOAD_CSV_HPP
#define MLPACK_CORE_DATA_LOAD_CSV_HPP

#include<boost/spirit/include/qi.hpp>

#include <mlpack/core/util/log.hpp>
#include <mlpack/core/arma_extend/arma_extend.hpp> // Includes Armadillo.
#include <string>

#include "format.hpp"
#include "dataset_info.hpp"

namespace mlpack {
namespace data /** Functions to load and save matrices and models. */ {

namespace details /** Implementation details, please do not use them in production codes */{

//put the implementation details do not depend on template params
//out of template class, this could reduce duplicate binary codes
//if the compiler/linker not smart enough

}

/**
 *Load the csv file.This class use boost::spirit
 *to implement the parser, please refer to following link
 *http://theboostcpplibraries.com/boost.spirit for quick review.
 */
class LoadCSV
{      
public:
  explicit LoadCSV(std::string const &file) : inFile(file)
  {
    if(!inFile.is_open())
    {
      throw std::runtime_error("LoadCSV can not open file");
    }
    inFile.unsetf(std::ios::skipws);
  }

  template<typename T>
  void Load(arma::Mat<T> &inout, DatasetInfo &infoSet, bool transpose = true)
  {
    //please refer to the comments of ColSize if you do not familiar
    //with boost::spirit yet
    if(transpose)
    {
      TranposeParse(inout, infoSet);
    }
    else
    {
      NonTranposeParse(inout, infoSet);
    }
  }

  size_t ColSize()
  {
    //boost tokenizer or strtok can do the same thing, I use
    //spirit at here because I think this is a nice example
    using namespace boost::spirit;
    using bsi_type = boost::spirit::istream_iterator;
    using iter_type = boost::iterator_range<bsi_type>;

    inFile.clear();
    inFile.seekg(0, std::ios::beg);
    //spirit::qi requires iterators to be atleast forward iterators,
    //but std::istream_iterator is input iteraotr, so we use
    //boost::spirit::istream_iterator to overcome this problem
    bsi_type begin(inFile);
    bsi_type end;
    size_t col = 0;

    //the parser of boost spirit can work with "actions"(functor)
    //when the parser find match target, this functor will be executed
    auto findColSize = [&col](iter_type){ ++col; };

    //qi::char_ bite an character
    //qi::char_(",\r\n") only bite a "," or "\r" or "\n" character
    //* means the parser(ex : qi::char_) can bite [0, any size] of characters
    //~ means negate, so ~qi::char_(",\r\n") means I want to bite anything except of ",\r\n"
    //parse % "," means you want to parse string like "1,2,3,apple"(noticed it without last comma)

    //qi::raw restrict the automatic conversion of boost::spirit, without it, spirit parser
    //will try to convert the string to std::string, this would cause memory allocation
    //After we wrap the parser with qi::raw, the attribute(the data accepted by functor) will
    //become boost::iterator_range, this could save a tons of memory allocations
    qi::parse(begin, end, qi::raw[*~qi::char_(",\r\n")][findColSize] % ",");

    return col;
  }

  size_t RowSize()
  {
    inFile.clear();
    inFile.seekg(0, std::ios::beg);
    size_t row = 0;
    std::string line;
    while(std::getline(inFile, line))
    {
      ++row;
    }

    return row;
  }

private:
  using iter_type = boost::iterator_range<std::string::iterator>;

  struct ElemParser
  {
    //return int_parser if the type of T is_integral
    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value,
    boost::spirit::qi::int_parser<T>>::type
    Parser()
    {
      return boost::spirit::qi::int_parser<T>();
    }

    //return real_parser if T is floating_point
    template<typename T>
    static typename std::enable_if<std::is_floating_point<T>::value,
    boost::spirit::qi::real_parser<T>>::type
    Parser()
    {
      return boost::spirit::qi::real_parser<T>();
    }
  };

  template<typename T>
  void NonTranposeParse(arma::Mat<T> &inout, DatasetInfo &infoSet)
  {
    using namespace boost::spirit;

    size_t row = 0;
    size_t col = 0;
    infoSet = DatasetInfo(RowSize());
    std::string line;
    inout.set_size(infoSet.Dimensionality(), ColSize());
    inFile.clear();
    inFile.seekg(0, std::ios::beg);

    auto setNum = [&](T val)
    {
      inout(row, col++) = val;
    };
    auto setCharClass = [&](iter_type const &iter)
    {
      inout(row, col++) =
          static_cast<T>(infoSet.MapString(std::string(iter.begin(), iter.end()),
                                           row));
    };

    qi::rule<std::string::iterator, T()> numRule = CreateNumRule<T>();
    qi::rule<std::string::iterator, iter_type()> charRule = CreateCharRule();
    while(std::getline(inFile, line))
    {
      auto begin = line.begin();
      const bool allNumber =
          qi::parse(begin, line.end(), numRule[setNum] % ",");
      if(!allNumber)
      {
        begin = line.begin();
        col = 0;
        const bool canParse = qi::parse(begin, line.end(),
                                        charRule[setCharClass] % ",");
        if(!canParse)
        {
          throw std::runtime_error("LoadCSV cannot parse categories");
          break;
        }
      }
      ++row; col = 0;
    }
  }

  template<typename T>
  void TranposeParse(arma::Mat<T> &inout, DatasetInfo &infoSet)
  {
    infoSet = DatasetInfo(ColSize());
    inout.set_size(infoSet.Dimensionality(), RowSize());
    while(!TranposeParseImpl(inout, infoSet))
    {

    }
  }

  template<typename T>
  bool TranposeParseImpl(arma::Mat<T> &inout, DatasetInfo &infoSet)
  {
    using namespace boost::spirit;

    size_t row = 0;
    size_t col = 0;
    std::string line;
    inFile.clear();
    inFile.seekg(0, std::ios::beg);

    auto setNum = [&](T val)
    {
      inout(row++, col) = val;
    };
    auto setCharClass = [&](iter_type const &iter)
    {
      inout(row++, col) =
          static_cast<T>(infoSet.MapString(std::string(iter.begin(), iter.end()),
                                           col));
    };

    qi::rule<std::string::iterator, T()> numRule = CreateNumRule<T>();
    qi::rule<std::string::iterator, iter_type()> charRule = CreateCharRule();
    while(std::getline(inFile, line))
    {
      auto begin = line.begin();
      const bool allNumber =
          qi::parse(begin, line.end(), numRule[setNum] % ",");
      if(!allNumber)
      {
        begin = line.begin();
        const size_t dimension = infoSet.NumMappings(col);
        if((dimension == 0 && row == 0) || dimension != 0)
        {
          row = 0;
          const bool canParse = qi::parse(begin, line.end(),
                                          charRule[setCharClass] % ",");
          if(!canParse)
          {
            throw std::runtime_error("LoadCSV cannot parse categories");
          }
        }
        else
        {
          return false;
        }
      }
      row = 0; ++col;
    }

    return true;
  }

  template<typename T>
  boost::spirit::qi::rule<std::string::iterator, T()> CreateNumRule() const
  {
    using namespace boost::spirit;

    //elemParser will generate integer or real parser based on T
    auto elemParser = ElemParser::Parser<T>();
    //qi::skip can specify which characters you want to skip,
    //in this example, elemParser will parse int or double value,
    //but we do not want space to intefere it, so we skip it by qi::skip

    //qi::omit can omit the attributes of spirit, every parser of spirit
    //has attribute(the type will pass into actions(functor))
    //if you do not omit it, the attribute combine with attribute may
    //change the attribute

    //"-" means one or zero(same as "-" of EBNF)
    return qi::skip(qi::char_(" "))[elemParser] >> -qi::omit[*qi::char_(" ")];
  }

  boost::spirit::qi::rule<std::string::iterator, iter_type()> CreateCharRule() const
  {
    using namespace boost::spirit;
    return -qi::omit[*qi::char_(" ")] >> qi::raw[*~qi::char_(" ,\r\n")]
        >> -qi::omit[*qi::char_(" ")];
  }

  std::ifstream inFile;
};

} // namespace data
} // namespace mlpack

#endif
