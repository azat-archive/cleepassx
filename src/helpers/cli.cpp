#define DEFINE_WHEREAMI_MACRO
#include "cli.h"

using namespace std;

namespace helpers {
namespace cli {
char ReadYesNoChar(const std::string& _prompt,
                   const std::string& _yes,
                   const std::string& _no,
                   const char _default_ans) {
    using namespace std;
    string yes = sLower(_yes);
    string no = sLower(_no);
    if (yes.empty()) {
        if (no.empty()) throw logic_error(__WHEREAMI__ + "> both „yes“ and „no“ parameters are empty, it's a programmer's bug");
        yes += "y";
    }
    string ans_tip = string(" [") + _yes[0] + "/" + _no[0] + "] ";
    char default_ans = 0;
    if (_default_ans != 0) {
        default_ans = tolower(_default_ans);
        if (yes.find(default_ans) != yes.npos)
            ans_tip.replace(2, 1, 1, toupper(_default_ans));
        else if (no.find(default_ans) != no.npos)
            ans_tip.replace(4, 1, 1, toupper(_default_ans));
        else
            throw logic_error(__WHEREAMI__ + "> Inappropriate default arg was passed to ReadYesNoChar function, it's a programmer's bug.");
    }
    string prompt = _prompt + ans_tip;

    do {
        cout << prompt;
        char response[65536];
        cin.getline(response, sizeof(response) / sizeof(response[0]));
        string l_response = sLower(response);
        if (l_response.empty()) {
            if (default_ans != 0) return default_ans;
            else continue;
        }
        if (yes.find(response[0]) != yes.npos || yes.empty())
            return _yes[0];
        if (no.find(response[0]) != no.npos || no.empty())
            return _no[0];
        // neither yes nor no are empty here
        cout << "Please input any of the chars: «" << yes << "» as «yes» or «" << no << "» as «no» in either upper or lower case and then press enter." << endl;
    } while (!cin.eof());
    if (default_ans != 0) return default_ans;
    else throw runtime_error (__WHEREAMI__ + "> No answer provided, when required.");
}
}
}
