#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>

#include <ctype.h>

#ifdef DEFINE_WHEREAMI_MACRO
 #ifndef __WHEREAMI__
  #define __WHEREAMI__ __FILE__ + ::helpers::toString(__LINE__)
 #endif
#endif

namespace helpers {
    ///Convert string to lowercase (via \c tolower).
    /** \param[in,out] str inout string for conversion.
      * \return str (modified).
      * \see sLower
      * \see sToUpper
      **/
    inline std::string& sToLower(std::string& str)
    {
        for(int i = 0; i < str.size(); i++)
            str.replace(i, 1,1, tolower(str[i]));
        return str;
    }

    ///Get lowercased string (using \c tolower).
    /** \param[in] str const string to convert.
      * \return str in lower case.
      * \see sToLower
      * \see sUpper
      **/
    inline std::string sLower(const std::string& str)
    {
        std::string result(str);
        sToLower(result);
        return result;
    }
    
    inline void cToUpper(char& _) { _ = toupper(_); }
    ///Convert string to uppercase (via \c toupper).
    /** \param[in,out] str inout string for conversion.
      * \return str (modified).
      * \see sUpper
      * \see sToLower
      **/
    inline std::string& sToUpper(std::string& str) {
        std::for_each(str.begin(), str.end(), cToUpper);
        return str;
    }

    ///Convert string to upper case (using \c toupper).
    /** \param[in] str const string to convert.
      * \return str in upper case.
      * \see sToUpper
      * \see sLower
      **/
    inline std::string sUpper(const std::string& str)
    {
        std::string result(str);
        sToUpper(result);
        return result;
    }

    template <typename T>
    inline std::string toString(const T& _) {
        std::stringstream ss;
        ss << _;
        return ss.str();
    }

namespace cli {
    /**
     * @brief Read user responce to the question
     *
     * @param[in] _prompt question
     * @param[in] _yes letters considered as yes
     * @param[in] _no letters considered as no
     * @param[in] _default_ans default answer or 0 if no default
     * @return char, either _yes[0] or _no[0]
     * 
     * if the \c default_ans is NULL, user is forced to give some non-empty answer that could be found either in \c _yes or in \c _no.
     * if one of the args is empty, any letter except the one listed in another arg fits.
     * if both \c _yes and \c _no args are empty, std::logic_error is thrown.
     * all arguments are case-insensitive
     * if the letter _default_ans was not found in neither \c _yes nor \c _no (case-insensitive), \c std::logic_error is thrown;
    **/
    char ReadYesNoChar (const std::string& _prompt,
                        const std::string& _yes,
                        const std::string& _no,
                        const char _default_ans
                       );
}
}
