#include <iostream>
#include <ctime>
#include "student.h"

Korean koreanStudent;

int* get_korean_age(int original, int* korean){
    *korean = original + 1;

    return korean;
}

string* get_korean_name(string original, string* korean){
    *korean = "Kimchi " + original;
    
    return korean;
}

int main(int argc, char **argv){
    Student student;

    if(argc < 3){
        cerr << "Wrong usage : target {name} {age}" << endl;
        return 1;
    }

    string argName(argv[1]);
    string argAge(argv[2]);

    student.set_name(argName);
    student.set_age(stoi(argAge));

    int* korean_age = get_korean_age(student.get_age(), &(koreanStudent.korean_age));
    string* korean_name = get_korean_name(student.get_name(), &(koreanStudent.korean_name));

    cout << "original age : " << student.get_age() << endl;
    cout << "original name : " << student.get_name() << endl;
    cout << "korean age : " << *korean_age << endl;
    cout << "korean name : " << *korean_name << endl;
    cout << "korean age(in-class) : " << koreanStudent.get_age() << endl;
    cout << "korean name(in-class) : " << koreanStudent.get_name() << endl;

    time_t now = time(0);
    char *dt = ctime(&now);

    cout << dt << "\n";

    now = time(0);
    dt = ctime(&now);

    cout << dt << "\n";
    
    return 1;
}