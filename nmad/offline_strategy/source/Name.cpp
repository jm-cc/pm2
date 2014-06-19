/*
** This file is part of the ViTE project.
**
** This software is governed by the CeCILL-A license under French law
** and abiding by the rules of distribution of free software. You can
** use, modify and/or redistribute the software under the terms of the
** CeCILL-A license as circulated by CEA, CNRS and INRIA at the following
** URL: "http://www.cecill.info".
** 
** As a counterpart to the access to the source code and rights to copy,
** modify and redistribute granted by the license, users are provided
** only with a limited warranty and the software's author, the holder of
** the economic rights, and the successive licensors have only limited
** liability.
** 
** In this respect, the user's attention is drawn to the risks associated
** with loading, using, modifying and/or developing or reproducing the
** software by the user in light of its specific status of free software,
** that may mean that it is complicated to manipulate, and that also
** therefore means that it is reserved for developers and experienced
** professionals having in-depth computer knowledge. Users are therefore
** encouraged to load and test the software's suitability as regards
** their requirements in conditions enabling the security of their
** systems and/or data to be ensured and, more generally, to use and
** operate it in the same conditions as regards security.
** 
** The fact that you are presently reading this means that you have had
** knowledge of the CeCILL-A license and that you accept its terms.
**
**
** ViTE developers are (for version 0.* to 1.0):
**
**        - COULOMB Kevin
**        - FAVERGE Mathieu
**        - JAZEIX Johnny
**        - LAGRASSE Olivier
**        - MARCOUEILLE Jule
**        - NOISETTE Pascal
**        - REDONDY Arthur
**        - VUCHENER Clément 
**
*/
#include <string>
/* -- */
#include "../include/trace/values/Value.hpp"
#include "../include/trace/values/String.hpp"
#include <cstdio>
#include "../include/trace/values/Name.hpp"


Name::Name() : _alias(""), _name("") {
    _is_correct = true;
}


Name::Name(std::string name, std::string alias) : _alias(alias), _name(name) {
    _is_correct = true;
}

Name::Name(String name) {
    _alias=name.to_string();
    _name=name.to_string();
    _is_correct = true;
}

std::string Name::get_name() const{
    return _name;
}

std::string Name::get_alias() const{
    return _alias;
}

void Name::set_name(std::string name) {
    _name = name;
}

void Name::set_alias(std::string alias) {
    _alias = alias;
}

String Name::to_String() const{
if (!_name.empty())
    return String(_name);
 else
     return String(_alias);
}

std::string Name::to_string() const {
    if (!_name.empty())
        return _name;
    else
        return _alias;
}

bool Name::operator== (Name &s) const {
    if(!_name.empty() && !_alias.empty() && !s.get_alias().empty() && !s.get_name().empty()){
        return (s.get_name() == _name || s.get_name() == _alias);
    }else if (!_name.empty()){
        return s.get_name() == _name;
    }else if (!_alias.empty()){
        return s.get_alias() == _alias;
    }
    return false;
}

bool Name::operator== (String &s) const {
    return s == _alias || s == _name;
}

bool Name::operator== (std::string &s) const {
    return s == _alias || s == _name;
}

bool Name::operator< (std::string &s) const {
    return _name < s ;
}

bool Name::operator< (const Name &s) const {
    if (!_alias.empty())return _alias < s.get_alias();
  
   if (!_alias.empty()&&! s.get_alias().empty())return _alias < s.get_alias();
     else  if  (!_name.empty()&&! s.get_name().empty()) return _name < s.get_name() ;
     return false;
}

