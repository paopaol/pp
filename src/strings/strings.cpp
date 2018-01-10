#include <strings.h>

#include <functional>
#include <algorithm>
#include <vector>
#include<ctype.h>

namespace pp{
	using namespace std;
	namespace strings{
		bool isSpace(unsigned char c)
		{
			return (bool)isspace((unsigned char)c);
		}


		string TrimRightFunc(const std::string s, std::function<bool (char)> f)
		{
			for (auto i = s.size() - 1; i > 0; i--){
				unsigned char c = s[i];
				if (f(c) == false){
					string	str(s.data(), i + 1);
					return str;
				}
			}

			return "";
		}

		string TrimLeftFunc(const std::string s, std::function<bool (char)> f)
		{
			for (unsigned i = 0; i < s.size(); i++){
				if (f(s[i]) == false){
					string	str(s.data() + i, s.size() - i);
					return str;
				}
			}

			return "";
		}


		string TrimRight(const std::string s, const std::string trim)
		{
				if (s == ""){
					return s;
				}
				return TrimRightFunc(s, isSpace);
		}

		string TrimLeft(const std::string s, const std::string trim)
		{
			if (s == ""){
				return s;
			}
			return TrimLeftFunc(s, isSpace);
		}



		string TrimSpace(const string &str)
		{
			if (str == ""){
				return 0;
			}
			return TrimLeft(TrimRight(str, ""), "");
		}

		string ToUpper(const string &str)
		{
			string	upperString(str);

			transform(upperString.begin(), upperString.end(), upperString.begin(), ::toupper);
			return upperString;
		}

		string ToLower(const string &str)
		{
			string lowerString(str);

			transform(lowerString.begin(), lowerString.end(), lowerString.begin(), ::tolower);
			return lowerString;
		}

		vector<string> &Split(vector<string> &vstr, const string &str, const string &pattern)
		{
			vstr.resize(0);

			if (str == "" || pattern == ""){
				return vstr;
			}

			string s = str + pattern;
			size_t size = s.size();

			for (size_t pos = s.find(pattern); pos != string::npos;){
				string x(s.substr(0, pos));
				vstr.push_back(x);
				s = s.substr(pos + 1, size);
				pos = s.find(pattern);
			}
			return vstr;
		}

		bool Contains(const string &s, const string &substr)
		{
			if (s.find(substr) != string::npos){
				return true;
			}
			return false;
		}

		string Join(const vector<string> &a, const string &sep)
		{
			if (a.size() == 0){
				return "";
			}
			if (a.size() == 1){
				return a[0];
			}
			
			string b = *(a.begin());
			for (auto it = a.begin() + 1; it != a.end(); it++){
				b.append(sep);
				b.append(*it);
			}
			return b;
		}
	}
}
