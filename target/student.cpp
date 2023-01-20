#include "student.h"

string Student::get_name(){
    return name;
}

void Student::set_name(string _name){
    name = _name;
}

int Student::get_age(){
    return age;
}

void Student::set_age(int _age){
    age = _age;
}

string Korean::get_name(){
    return korean_name;
}

void Korean::set_name(string _name){
    korean_name = _name;
}

int Korean::get_age(){
    return korean_age;
}

void Korean::set_age(int _age){
    korean_age = _age;
}