
#include <strings.h>
#include <fmt.h>

using namespace pp;
using namespace std;


static void test_TrimSpace()
{
	const char	*str = "   sdsdsds sdsdsd sdsd ";
	fmt::Println("[%s]", strings::TrimSpace(str).c_str());

	string	str2 = "sdsds    ";
	fmt::Println("[%s]", strings::TrimSpace(str2).c_str());
}

static void test_ToUpper()
{
	const char *lower1 = "abc";
	fmt::Println("%s", strings::ToUpper(lower1).c_str());
	
	string lower2 = "aBc123@";
	fmt::Println("%s", strings::ToUpper(lower2).c_str());

}

static void test_ToLower()
{
	const char *upper = "ABC@";
	fmt::Println("%s", strings::ToLower(upper).c_str());

	string upper2 = "acbBBB@";
	fmt::Println("%s", strings::ToLower(upper2).c_str());
}


static void printStringSlice(const vector<string> &slice)
{
	for (auto s = slice.begin(); s != slice.end(); s++){
		fmt::Println("%s", s->c_str());
	}
	fmt::Println("");
}

static void test_Split()
{
	const char *s1 = "1,2,3,4,5,6,";
	vector<string> slice;
	printStringSlice(strings::Split(slice, s1, ","));
	printStringSlice(strings::Split(slice, s1, ",2,3,4,"));
	

}


int main()
{
	test_TrimSpace();	
	fmt::Println("********");
	test_ToUpper();
	fmt::Println("********");
	test_ToLower();
	fmt::Println("********");
	test_Split();

}
