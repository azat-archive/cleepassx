#define DEFINE_WHEREAMI_MACRO
#include <readline/readline.h>

#include "cli.h"
#include "syntax.h"

using namespace std;

namespace helpers {
namespace cli {
    char** disable_tab_completion(const char* _beginning, int _start, int _end) {
        rl_attempted_completion_over = true;
        return NULL;
    }

    
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

    using namespace helpers::syntax;
    hide_value<rl_completion_func_t*> hv(rl_attempted_completion_function,
                                         disable_tab_completion);
    do {
        cout << prompt;
        auto_free<char> line(readline(_prompt.c_str()));
        if (line == NULL) break;
        if (line[0] == 0) {
            if (default_ans != 0) return default_ans;
            else continue;
        }
        string l_response = sLower(static_cast<const char*>(line));
        cerr << "the l_response is «" << l_response << "»\n";
        if (yes.find(l_response[0]) != yes.npos || yes.empty())
            return _yes[0];
        if (no.find(l_response[0]) != no.npos || no.empty())
            return _no[0];
        // neither yes nor no are empty here
        cout << "Please input any of the chars: «" << yes << "» as «yes» or «" << no << "» as «no» in either upper or lower case and then press enter." << endl;
    } while (true);
    if (default_ans != 0) return default_ans;
    else throw runtime_error (__WHEREAMI__ + "> No answer provided, when required.");
}
}
}
