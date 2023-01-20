#include <string>

using namespace std;

class Student {
public:
    string get_name();
    void set_name(string _name);
    int get_age();
    void set_age(int _age);

private:
    string name;
    int age;
};

class Korean {
public:
    string get_name();
    void set_name(string _name);
    int get_age();
    void set_age(int _age);

    string korean_name;
    int korean_age;
};