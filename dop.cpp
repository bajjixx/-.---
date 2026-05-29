#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <climits>
#include <cstring>
#include <sstream>
#include <cctype>

using namespace std;

struct Denom {
    int value;
    int count;
};

struct TestCase {
    vector<Denom> wallet;
    int amount;
    string strategy;
};

struct Result {
    vector<Denom> dispense;
};

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

vector<Denom> parseWallet(const string& json, size_t& pos) {
    vector<Denom> res;
    while (pos < json.length() && json[pos] != '[') pos++;
    if (pos >= json.length()) return res;
    pos++;
    
    while (pos < json.length()) {
        while (pos < json.length() && isspace(json[pos])) pos++;
        if (pos >= json.length()) break;
        if (json[pos] == ']') {
            pos++;
            break;
        }
        if (json[pos] != '[') {
            pos++;
            continue;
        }
        pos++;
        while (pos < json.length() && isspace(json[pos])) pos++;
        int value = 0;
        bool neg = false;
        if (json[pos] == '-') { neg = true; pos++; }
        while (pos < json.length() && isdigit(json[pos])) {
            value = value * 10 + (json[pos] - '0');
            pos++;
        }
        if (neg) value = -value;
        
        while (pos < json.length() && json[pos] != ',' && !isdigit(json[pos]) && json[pos] != '-') pos++;
        if (json[pos] == ',') pos++;
        while (pos < json.length() && isspace(json[pos])) pos++;
        
        int count = 0;
        neg = false;
        if (json[pos] == '-') { neg = true; pos++; }
        while (pos < json.length() && isdigit(json[pos])) {
            count = count * 10 + (json[pos] - '0');
            pos++;
        }
        if (neg) count = -count;
        
        res.push_back({value, count});
        
        while (pos < json.length() && json[pos] != ']' && json[pos] != ',') pos++;
        if (json[pos] == ']') {
            pos++;
            while (pos < json.length() && isspace(json[pos])) pos++;
            if (json[pos] == ',') pos++;
        } else if (json[pos] == ',') {
            pos++;
        }
    }
    return res;
}

TestCase parseTestCase(const string& json, size_t& pos) {
    TestCase tc;
    tc.amount = 0;
    tc.strategy = "";
    
    while (pos < json.length()) {
        size_t found = json.find("\"wallet\"", pos);
        if (found == string::npos) break;
        pos = found + 8;
        while (pos < json.length() && isspace(json[pos])) pos++;
        if (json[pos] == ':') {
            pos++;
            while (pos < json.length() && isspace(json[pos])) pos++;
            tc.wallet = parseWallet(json, pos);
            break;
        }
    }
    
    pos = 0;
    while (pos < json.length()) {
        size_t found = json.find("\"amount\"", pos);
        if (found == string::npos) break;
        pos = found + 8;
        while (pos < json.length() && isspace(json[pos])) pos++;
        if (json[pos] == ':') {
            pos++;
            while (pos < json.length() && isspace(json[pos])) pos++;
            tc.amount = 0;
            while (pos < json.length() && isdigit(json[pos])) {
                tc.amount = tc.amount * 10 + (json[pos] - '0');
                pos++;
            }
            break;
        }
    }
    
    pos = 0;
    while (pos < json.length()) {
        size_t found = json.find("\"strategy\"", pos);
        if (found == string::npos) break;
        pos = found + 10;
        while (pos < json.length() && isspace(json[pos])) pos++;
        if (json[pos] == ':') {
            pos++;
            while (pos < json.length() && isspace(json[pos])) pos++;
            if (json[pos] == '"') {
                pos++;
                string strat;
                while (pos < json.length() && json[pos] != '"') {
                    strat += json[pos];
                    pos++;
                }
                pos++;
                tc.strategy = strat;
            }
            break;
        }
    }
    
    return tc;
}

bool found = false;
vector<Denom> bestSolution;

void searchMax(const vector<Denom>& wallet, int idx, int remaining, vector<int>& curCount, vector<Denom>& best, int& bestTotalCount) {
    if (remaining == 0) {
        bool better = false;
        if (best.empty()) {
            better = true;
        } else {
            for (int i = 0; i < (int)wallet.size(); i++) {
                if (curCount[i] != best[i].count) {
                    if (curCount[i] > best[i].count) better = true;
                    break;
                }
            }
        }
        if (better) {
            best.clear();
            for (int i = 0; i < (int)wallet.size(); i++) {
                if (curCount[i] > 0) {
                    best.push_back({wallet[i].value, curCount[i]});
                }
            }
        }
        return;
    }
    if (idx >= (int)wallet.size()) return;
    
    int maxTake = min(wallet[idx].count, remaining / wallet[idx].value);
    for (int take = maxTake; take >= 0; take--) {
        curCount[idx] = take;
        searchMax(wallet, idx + 1, remaining - take * wallet[idx].value, curCount, best, bestTotalCount);
    }
    curCount[idx] = 0;
}

bool searchMin(const vector<Denom>& wallet, int idx, int remaining, vector<int>& curCount, vector<Denom>& best) {
    if (remaining == 0) {
        best.clear();
        for (int i = 0; i < (int)wallet.size(); i++) {
            if (curCount[i] > 0) {
                best.push_back({wallet[i].value, curCount[i]});
            }
        }
        return true;
    }
    if (idx >= (int)wallet.size()) return false;
    
    int maxTake = min(wallet[idx].count, remaining / wallet[idx].value);
    for (int take = maxTake; take >= 0; take--) {
        curCount[idx] = take;
        if (searchMin(wallet, idx + 1, remaining - take * wallet[idx].value, curCount, best)) {
            return true;
        }
    }
    curCount[idx] = 0;
    return false;
}

void searchUniform(const vector<Denom>& wallet, int idx, int remaining, vector<int>& curCount, vector<Denom>& best, int& bestDiff) {
    if (remaining == 0) {
        int mn = INT_MAX, mx = INT_MIN;
        for (int c : curCount) {
            if (c < mn) mn = c;
            if (c > mx) mx = c;
        }
        int diff = mx - mn;
        if (diff < bestDiff) {
            bestDiff = diff;
            best.clear();
            for (int i = 0; i < (int)wallet.size(); i++) {
                if (curCount[i] > 0) {
                    best.push_back({wallet[i].value, curCount[i]});
                }
            }
        }
        return;
    }
    if (idx >= (int)wallet.size()) return;
    
    int maxTake = min(wallet[idx].count, remaining / wallet[idx].value);
    for (int take = maxTake; take >= 0; take--) {
        curCount[idx] = take;
        searchUniform(wallet, idx + 1, remaining - take * wallet[idx].value, curCount, best, bestDiff);
    }
    curCount[idx] = 0;
}

vector<Denom> solveTestCase(const TestCase& tc) {
    vector<Denom> wallet = tc.wallet;
    sort(wallet.begin(), wallet.end(), [](const Denom& a, const Denom& b) {
        return a.value < b.value;
    });
    
    vector<int> curCount(wallet.size(), 0);
    vector<Denom> result;
    
    if (tc.strategy == "MAX") {
        sort(wallet.begin(), wallet.end(), [](const Denom& a, const Denom& b) {
            return a.value > b.value;
        });
        vector<Denom> best;
        int dummy = 0;
        searchMax(wallet, 0, tc.amount, curCount, best, dummy);
        result = best;
    } 
    else if (tc.strategy == "MIN") {
        sort(wallet.begin(), wallet.end(), [](const Denom& a, const Denom& b) {
            return a.value < b.value;
        });
        vector<Denom> best;
        searchMin(wallet, 0, tc.amount, curCount, best);
        result = best;
    }
    else if (tc.strategy == "UNIFORM") {
        sort(wallet.begin(), wallet.end(), [](const Denom& a, const Denom& b) {
            return a.value < b.value;
        });
        vector<Denom> best;
        int bestDiff = INT_MAX;
        searchUniform(wallet, 0, tc.amount, curCount, best, bestDiff);
        result = best;
    }
    
    return result;
}

string resultToJson(const vector<Denom>& dispense) {
    stringstream ss;
    ss << "{\n\"dispense\": [";
    for (size_t i = 0; i < dispense.size(); i++) {
        if (i > 0) ss << ", ";
        ss << "[" << dispense[i].value << ", " << dispense[i].count << "]";
    }
    ss << "]\n}";
    return ss.str();
}

int main() {
    ifstream inFile("input.json");
    if (!inFile.is_open()) {
        cerr << "Error: cannot open input.json" << endl;
        return 1;
    }
    
    string content((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();
    
    vector<TestCase> testCases;
    size_t pos = 0;
    while (pos < content.length()) {
        size_t start = content.find('{', pos);
        if (start == string::npos) break;
        pos = start;
        int braceCount = 0;
        size_t end = start;
        for (; end < content.length(); end++) {
            if (content[end] == '{') braceCount++;
            else if (content[end] == '}') {
                braceCount--;
                if (braceCount == 0) break;
            }
        }
        if (end >= content.length()) break;
        string objJson = content.substr(start, end - start + 1);
        pos = end + 1;
        
        size_t p = 0;
        TestCase tc = parseTestCase(objJson, p);
        if (tc.amount > 0 && !tc.strategy.empty()) {
            testCases.push_back(tc);
        }
    }
    
    vector<Result> results;
    for (const auto& tc : testCases) {
        vector<Denom> d = solveTestCase(tc);
        results.push_back({d});
    }
    
    ofstream outFile("output.json");
    if (!outFile.is_open()) {
        cerr << "Error: cannot write output.json" << endl;
        return 1;
    }
    
    outFile << "[\n";
    for (size_t i = 0; i < results.size(); i++) {
        if (i > 0) outFile << ",\n";
        outFile << resultToJson(results[i].dispense);
    }
    outFile << "\n]\n";
    outFile.close();
    
    return 0;
}
