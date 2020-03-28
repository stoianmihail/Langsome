#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <regex>
#include <cassert>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "bk_tree.hpp"

using namespace std;

// #define DEBUG

#define MIN_LEN 5
#define SEARCH_PRECISION 1
#define DE_REGEX_MODE 1
#define EN_REGEX_MODE 0

struct levenshteinDistance {
  private:
  // Efficient implementation of Levenshtein Distance 
  uint32_t impl(const string& source, const string& target) {
    if (source.size() > target.size())
      return impl(target, source);

    const unsigned min_size = source.size(), max_size = target.size();
    vector<unsigned> levDist(min_size + 1);

    levDist[0] = 0;
    for (unsigned i = 1; i <= min_size; ++i)
      levDist[i] = i;

    for (unsigned j = 1; j <= max_size; ++j) {
      unsigned previous_diagonal = levDist[0], previous_diagonal_save;
      levDist[0] += 1;

      for (unsigned i = 1; i <= min_size; ++i) {
        previous_diagonal_save = levDist[i];
        
        if (source[i - 1] == target[j - 1]) {
          levDist[i] = previous_diagonal;
        } else {
          levDist[i] = min(min(levDist[i - 1] + 1, levDist[i] + 1), previous_diagonal + 1);
        }
        previous_diagonal = previous_diagonal_save;
      }
    }
    return levDist[min_size];
  }
  
  public:
  uint32_t operator()(const string &source, const string &target) {
    return impl(source, target);
  }
};

// The typedefs
typedef vector<string> VS;
typedef vector<uint32_t> VI;
typedef unordered_map<string, VI> hashTable;
typedef unordered_set<string> SoS;
typedef unordered_set<uint32_t> SoI;
typedef unordered_map<uint32_t, uint32_t> freqTable;
typedef unordered_map<uint32_t, string> medicineIndex;
typedef storage::bktree<string, uint32_t, levenshteinDistance> metricTree;

// Left trim
string ltrim(const string& s) {
	return regex_replace(s, regex("^[\\s-,']+"), string(""));
}

// Right trim
string rtrim(const string& s) {
	return regex_replace(s, regex("[\\s-,']+$"), string(""));
}

// Both trim
string trim(const string& s) {
	return ltrim(rtrim(s));
}

// Cast the string to lowercase
string strToLower(const string& str) {
  string ret;
  for (unsigned index = 0, limit = str.size(); index != limit; ++index)
    ret.push_back(tolower(str[index]));
  return ret;
}

static vector<string> invalid = {
  // Empty word
  "",
  // Related to medicins
  "g", "mg", "mcg", "ml", "mikrogramm", "unit"
  // Related to grammar
  "zur", "zum", "in", "eine", "einer", "ohne"
};

// Check if 'str' appears in 'invalid'
bool isInvalid(const string& str) {
  if (str.size() == 1)
    return true;
  auto casted = strToLower(str);
  return any_of(invalid.begin(), invalid.end(), [&casted](string& elem){ return casted == elem; });
}

bool hasOnlyDigits(const string& str) {
  return all_of(str.begin(), str.end(), ::isdigit);  
}

// Split 'str' into parts
enum class SplitMode {DE, EN, EN_PRICE};
vector<string> splitUpWithRegex(const string& str, SplitMode mode) {
  string ret;
  switch (mode) {
    // Do not change these options! (unless you are sure about that)
    case SplitMode::DE: ret = "[-,]+"; break; // supporting german references
    case SplitMode::EN: ret = "[\\s-,/'`]+"; break; // supporting drugbank synonyms
    case SplitMode::EN_PRICE: ret = "[\\s-,/'`=.%]+"; break; // supporting drugbank prices
  }
  regex rgx(ret);
  sregex_token_iterator iter(str.begin(), str.end(), rgx, -1), end;
  vector<string> result;
  for (; iter != end; ++iter) {
    string curr(*iter);
    if (curr.empty())
      continue;
    if ((mode == SplitMode::DE) or (mode == SplitMode::EN_PRICE)) {
      if (hasOnlyDigits(curr))
        continue;
      curr.erase(std::remove_if(std::begin(curr), std::end(curr), ::isdigit), curr.end());
      if (isInvalid(curr))
        continue;
    }
    result.push_back(curr);
  }
  return result;
}

// Get rid of any type of paranthesis. Note that if 'str' has not been correctly bracketed, the empty string is returned
string cleanUp(string& str) {
  auto isOpen = [](char c) -> bool {
    return (c == '(') || (c == '[') || (c == '{');
  };
  
  auto isClose = [](char c) -> bool {
    return (c == ')') || (c == ']') || (c == '}');
  };
  
  // Analyze the string with a stack of paranthesis
  string ret, empty;
  vector<char> stack;
  for (auto c : str) {
    if (isOpen(c)) {
      stack.push_back(c);
    } else if (isClose(c)) {
      if ((!stack.empty()) && (stack.back() == ((c == ')') ? '(' : ((c == ']') ? '[' : '{'))))
        stack.pop_back();
      else
        return empty;
    } else if (stack.empty()) {
      ret += c;
    }
  }
  ret = trim(ret);
  return stack.empty() ? ret : empty;
}

void dissolveMeds(string language, hashTable& table, medicineIndex& medIndex) {
  // Check the language (only German by now)
  if (language != "de") {
    cerr << "Language " << language << " not supported yet!" << endl;
    return;
  }
  
  // Open the input file
  string completePath = string("../meds/") + language + string("_meds.csv");
  ifstream input(completePath);
  
  unsigned medRow = 0;
  for (string medicine; input >> medicine; medRow++) {
    // Store the current medicine
    medIndex[medRow] = medicine;
    
    // Split the current medicine (since we only have the reference to it)
    vector<string> parts = splitUpWithRegex(medicine, SplitMode::DE);
    if (parts.empty())
      continue;
    
    // Do not consider the last part, since that is the unique id of the medicine
    for (unsigned index = 0, limit = parts.size() - 1; index != limit; ++index) {
      auto word = strToLower(parts[index]);
      // It could be the case that a word occurs twice in the same medicine, e.g. "alfa"
      if ((table[word].empty()) || (table[word].back() != medRow)) 
        table[word].push_back(medRow);
    }
  }
  input.close();
}

// Build up the metric tree
void buildStorage(const hashTable& table, metricTree& container) {
  for (auto elem : table)
    container.insert(elem.first);
}

// Analyze the line and parse the common name along with its synonyms, which are not chemical formulas
pair<string, pair<VS, VS>> analyzeLine(string line) {
  // Check for empty line
  if (line.empty()) {
    vector<string> empty;
    return make_pair(line, make_pair(empty, empty));
  }

  // Lambda-expression to check if the current char is a paranthesis
  // Words which contain at least a complex paranthesis are chemical formulas, thus, we are not interested in them
  auto isComplexParanthesis = [](char c) -> bool {
    return (c == '[') || (c == ']') || (c == '{') || (c == '}');
  };
  // There could be the case that a simple paranthesis only marks an explanation, e.g. Insuline (human) 
  auto isSimpleParanthesis = [](char c) -> bool {
    return (c == '(') || (c == ')');
  };
  
  // Extract the common name
  string commonName;
  unsigned index = 0;
  bool hadSimpleParanthesis = false, hadComplexParanthesis = false;
  for (bool activate = false; index != line.size(); ++index) {
    if (line[index] == '"') {
      activate = !activate;
    } else {
      if ((!activate) && (line[index] == ','))
        break;
      hadSimpleParanthesis |= isSimpleParanthesis(line[index]);
      hadComplexParanthesis |= isComplexParanthesis(line[index]);
      commonName += line[index];
    }
  }
  // If common name contained a complex paranthesis, that's for sure a chemical formula
  if (hadComplexParanthesis) {
    commonName.clear();
  } else if (hadSimpleParanthesis) {
    // Otherwise, it could be only an explanation, which is not that harmful
    commonName = cleanUp(commonName);
  }
  
  // Can we deploy the result earlier?
  vector<string> synonyms, prices;
  if ((commonName.empty()) || (index == line.size()))
    return make_pair(commonName, make_pair(synonyms, prices));
  
  // Append an element (synonym or price) to its corresponding list, depending on 'beginOfPrices'
  bool hasSimpleParanthesis = false, hasComplexParanthesis = false, beginOfPrices = false;
  auto appendElem = [&synonyms, &prices, &hasSimpleParanthesis, &hasComplexParanthesis, &beginOfPrices](string& elem) -> void {
    if (elem.empty())
      return;
    if (hasComplexParanthesis)
      return;
    if (hasSimpleParanthesis)
      elem = cleanUp(elem);
    if (beginOfPrices)
      prices.push_back(trim(elem));
    else
      synonyms.push_back(trim(elem));
    hasSimpleParanthesis = hasComplexParanthesis = false;
    elem.clear();
  };
  
  string curr;
  bool activate = false;
  unsigned distance = 1;
  for (++index; index != line.size(); ++index) {
    if (line[index] == '"') {
      activate = !activate;
    } else {
      // End of synonym?
      if (line[index] == '|') {
        appendElem(curr);
        distance = 0;
      } else {
        // Update the word and its status
        hasSimpleParanthesis |= isSimpleParanthesis(line[index]);
        hasComplexParanthesis |= isComplexParanthesis(line[index]);
        
        // If the medicine starts exactly after a bar, that is where the prices begin to appear
        if ((!distance) && (line[index] != ' '))
          beginOfPrices = true;
        curr += line[index];
        ++distance;
      }
    }
  }
  // And append the last synonym, only if did not contain any complex paranthesis
  appendElem(curr);
  
  // And deploy
  return make_pair(commonName, make_pair(synonyms, prices));
}

int main(int argc, char** argv) {
  // The program receives the .csv file of Drugbank database, which has been already parsed (en_meds.csv)
  // Example: ./match ../meds/en_meds.csv
  if (argc < 2)
    exit(0);
  
  // Check for file
  string databankFileName(argv[1]);
  if (databankFileName.empty()) {
    cerr << "empty database file name" << endl;
    exit(1);
  }
  
  // 'word2index' saves the indexes in file for each part of medicine
  hashTable word2index;
  
  // 'medIndex' tells us which medicine is to be found at a certain index (row)
  medicineIndex medIndex;
  
  // Split up the medicines to which we translate 
  dissolveMeds("de", word2index, medIndex);

  // Filter out the parts which are way too small
  VS mayBeEliminated;
  for (auto elem : word2index)
    if (elem.first.length() < MIN_LEN)
      mayBeEliminated.push_back(elem.first);
  for (auto elem : mayBeEliminated)
    word2index.erase(elem);
  
  // Save the parts into a BK-Tree.
  metricTree container;
  buildStorage(word2index, container);
  
  auto printIndex = [&medIndex](const VI& v) -> void {
    for (auto elem : v) {
      cout << "(" << elem << " -> " << medIndex[elem] << "), ";
    }
    cout << endl;
  };
  
  auto solveSplittedCase = [&word2index, &container](VS& splitted, string optional = "") -> VI {
    // Sum up the Levenshtein distances of the edges
    freqTable indexCloseness;
    
    // Count how many times the index has been used
    freqTable indexCount;
    
    // Each part, if not directly found in 'word2index', can have many similar parts in the file
    // Thus, we do not want to repeat an index, if it should appear at 2 different parts
    SoI rowBitMap;
    
    // Pick only the unique parts in 'splitted'
    SoS unique(splitted.begin(), splitted.end());
   
    VS acceptedParts;
    for (auto part : unique) {
      if (part.length() < MIN_LEN)
        continue;
      if (hasOnlyDigits(part))
        continue;
      
      // Check if the part can be directly found in 'word2index'
      auto iter = word2index.find(part);
      if (iter != word2index.end()) {
        // If so, the Levenshtein distance is 0, so only increase the count of the index
        acceptedParts.push_back(part);
        for (auto index : word2index[part])
          indexCount[index]++;
      } else {
        // Search for similar parts
        auto devs = container.find_within(part, SEARCH_PRECISION);
        if (!devs.empty()) {
          acceptedParts.push_back(part);
          rowBitMap.clear();
          for (auto dev : devs) {
            auto selectedWord = dev.first;
            auto levDistance = dev.second;
            
            // And update with the indexes of the word
            for (auto index : word2index[selectedWord]) {
              // First check if the index has not yet appeared for 'part'
              if (rowBitMap.find(index) == rowBitMap.end()) {
                indexCloseness[index] += levDistance;
                indexCount[index]++;
                rowBitMap.insert(index);
              }
            }
          }
        }
      }
    }
    
    // Custom function to determine how many times an index should appear
    auto analyzeAcceptedParts = [&splitted, &acceptedParts]() -> uint32_t {
      // The threshold for the soft-max function
      static constexpr double threshold = 1.0 / exp(1);
      
      // The lower-bound of the number of accepted parts
      auto lowerBound = static_cast<uint32_t>(splitted.size() / 2);
      
      // We are very restrictive
      if ((splitted.size() % 2) or (splitted.size() == 2))
        return lowerBound + 1;
    
      // Compute the soft-max of the lengths
      auto computeSoftMax = [&splitted, &acceptedParts]() -> double {
        auto addExp = [](const VS& parts) -> double {
          return accumulate(parts.begin(), parts.end(), 0, [](double acc, const string& part) {
            return acc + exp(part.length());
          });
        };
        
        // And return the ratio
        return addExp(acceptedParts) / addExp(splitted);
      };
      
      // Compute the lower-bound, after having analyzed the lengths of the accepted parts
      uint32_t offset = (acceptedParts.size() == lowerBound) ? (computeSoftMax() < threshold) : 1;
      return lowerBound + offset;
    };

    // First check if the lower bound has been respected
    VI bestIndexes;
    auto currentAcceptedSize = analyzeAcceptedParts();
    if (acceptedParts.size() < currentAcceptedSize)
      return bestIndexes;
    
    // And find the best indexes, where the medicines can be matched with the current medicine (its parts are in splitted)
    uint32_t maxIndexCount = 0, minCloseness = numeric_limits<uint32_t>::max();
    for (auto [index, count] : indexCount) {
      if (count < currentAcceptedSize)
        continue;
      if (count > maxIndexCount) {
        maxIndexCount = count;
        minCloseness = indexCloseness[index];
        bestIndexes.clear();
        bestIndexes.push_back(index);
      } else if ((count == maxIndexCount) && (indexCloseness[index] < minCloseness)) {
        minCloseness = indexCloseness[index];
        bestIndexes.push_back(index);
      }
    }
    
    // And return the best indexes
    return bestIndexes;
  };
  
  // Open the input file
  cerr << "Start parsing the english medicines.." << endl;
  ifstream in(databankFileName);
  if (!in.is_open()) {
    cerr << "file \"" << databankFileName << "\" could not open" << endl;
    exit(1);
  }
  
  // Open the output file
  string outputFile = "graph.matched";
  ofstream out(outputFile);
  
  // Analyze the common name
  auto analyzeCommonName = [&out, &word2index, &container, &solveSplittedCase, &printIndex](const uint32_t rowIndex, const string& commonName) -> bool {
    // Check if the medicine has the same name in the other language
    static constexpr bool commonNameSolved = true;
    auto castedName = strToLower(commonName);
    auto iter = word2index.find(castedName);
    
    // Is the medicine similar in German?
    if (iter != word2index.end()) {
      // Save the matching
      out << (rowIndex - 1);
      for (auto index : iter->second)
        out << " " << index;
      out << endl;
#ifdef DEBUG
      cout << "en = de: " << commonName << endl;
#endif  
      return commonNameSolved;
    }
    
    // Find possible deviations from the common name
    auto splittedName = splitUpWithRegex(castedName, SplitMode::EN);
    if (splittedName.empty())
      return commonNameSolved;
    
    if (splittedName.size() == 1) {
      auto single = splittedName.front();
      iter = word2index.find(single);
      if (iter != word2index.end()) {
        out << (rowIndex - 1);
        for (auto index : iter->second)
          out << " " << index;
        out << endl;
#ifdef DEBUG
        cout << "en ~ de: " << commonName << endl;
#endif
        return commonNameSolved;
      }
      auto deviated = container.find_within(single, SEARCH_PRECISION);
      if (!deviated.empty()) {
        // Save the matching
        SoI rowBitMap;
        out << (rowIndex - 1);
        for (auto part : deviated) {
          for (auto index : word2index[part.first]) {
            if (rowBitMap.find(index) == rowBitMap.end()) {
              rowBitMap.insert(index);
              out << " " << index; 
            }
          }
        }
        out << endl;
        rowBitMap.clear();
#ifdef DEBUG
        cout << "Deviated: " << commonName << ": ";
        for (auto part : deviated)
          cout << "part=" << part.first << ", ";
        cout << endl;
#endif
        return commonNameSolved;
      }
    } else {
      VI bestIndexes = solveSplittedCase(splittedName, commonName);
      if (!bestIndexes.empty()) {
        // Save the matching
        out << (rowIndex - 1);
        for (auto index : bestIndexes)
          out << " " << index;
        out << endl;
#ifdef DEBUG
        cout << "%%%: " << commonName;
        printIndex(bestIndexes);
#endif          
        return commonNameSolved;
      }
    }
    return !commonNameSolved;
  };
  
  // Analyze a type of list, either synonyms or prices
  auto analyzeResemblances = [&out, &word2index, &container, &solveSplittedCase, &printIndex](const uint32_t rowIndex, const string& commonName, const VS& list, const string resemblanceType, const SplitMode mode) -> bool {
    static constexpr bool resemblanceListSolved = true;
    if (list.empty())
      return !resemblanceListSolved;
    
    // Check the list
    for (auto elem : list) {
      auto castedElem = strToLower(elem);
      auto splittedElem = splitUpWithRegex(castedElem, mode);
      if (splittedElem.empty())
        continue;
      
      // If 'elem' consists of only one part
      if (splittedElem.size() == 1) {
        auto single = splittedElem.front();
        
        // Check if 'single' can be directly found
        auto castedSingle = strToLower(single);
        auto iter = word2index.find(castedSingle);
        if (iter != word2index.end()) {
          // Save the matching
          out << (rowIndex - 1);
          for (auto index : iter->second)
            out << " " << index;
          out << endl;
#ifdef DEBUG
          cout << "en (" << resemblanceType << ") de: " << commonName << " -> " << single << endl;
#endif
          return resemblanceListSolved; 
        } else {
          auto similarGermanParts = container.find_within(castedSingle, SEARCH_PRECISION);
          if (!similarGermanParts.empty()) {
            // Save the matching
            SoI rowBitMap;
            out << (rowIndex - 1);
            for (auto part : similarGermanParts) {
              for (auto index : word2index[part.first]) {
                if (rowBitMap.find(index) == rowBitMap.end()) {
                  rowBitMap.insert(index);
                  out << " " << index;
                }
              }
            }
            out << endl;
            rowBitMap.clear();
#ifdef DEBUG
            cout << "Found in list " << single << ": ";
            for (auto part : similarGermanParts)
              cout << "commonName=" << commonName << " -> " << part.first << ", ";
            cout << endl;
#endif
            return resemblanceListSolved;
          }
        }
      } else {
        // Analyze 'castedElem' when there are many more parts
        VI bestIndexes = solveSplittedCase(splittedElem, elem);
        if (!bestIndexes.empty()) {
          // Save the matching
          out << (rowIndex - 1);
          for (auto index : bestIndexes)
            out << " " << index;
          out << endl;            
#ifdef DEBUG
          cout << "*** Multiple : common=" << commonName << " " << resemblanceType << "=" << elem;
          printIndex(bestIndexes);
#endif
          return resemblanceListSolved;
        }
      }
    }
    return !resemblanceListSolved;
  };
  
  // Analyze each row of the database .csv file
  string line;
  unsigned rowIndex = 0;
  nextMatch : {
    // Get the next line, if any
    line.clear();
    if (getline(in, line)) {
      rowIndex++;
      // Analyze the line, i.e extract the common name and its list of synonyms
      pair<string, pair<VS, VS>> curr = analyzeLine(line);
      
      // Check for an empty common nome
      string commonName = curr.first;
      if (commonName.empty())
        goto nextMatch;
      
      if (analyzeCommonName(rowIndex, commonName))
        goto nextMatch;
      
      // Analyze the synonyms
      VS synonyms = curr.second.first;
      if (analyzeResemblances(rowIndex, commonName, synonyms, "synonym", SplitMode::EN))
        goto nextMatch;
      
      // Analyze the prices
      VS prices = curr.second.second;
      if (analyzeResemblances(rowIndex, commonName, prices, "price", SplitMode::EN_PRICE))
        goto nextMatch;
      
      // Continue the loop
      goto nextMatch;
    }
  }
  return 0;
}
