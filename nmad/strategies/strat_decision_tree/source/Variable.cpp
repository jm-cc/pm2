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
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <vector>
#include <stack>
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/EntityTypes.hpp"
#include "../include/trace/Entitys.hpp"
/* -- */
using namespace std;

Variable::Variable(Container *container, VariableType *type): Entity(container, map<std::string, Value *>()), _min(0.0), _max(0.0), _type(type) {
   
}

Variable::~Variable() {
    _values.clear();
    _type = NULL;
}

void Variable::add_value(Date time, Double value) {
    if (_values.empty())
        _min = _max = value;
    else if (value > _max)
        _max = value;
    else if (value < _min)
        _min = value;
    _values.push_back(pair<Date, Double>(time, value));
}

Double Variable::get_last_value() const {
    return _values.back().second;
}

const list<pair<Date, Double> > *Variable::get_values() const {
    return &_values;
}

const double Variable::get_value_at(double d) const {
    list<pair<Date, Double> >::const_iterator it= _values.begin();
    const list<pair<Date, Double> >::const_iterator it_end= _values.end();
    if(it==it_end)return 0.0;
    const Date* previous=&((*it).first);
    const Double * val = &((*it).second);
    it++;
    while ((it!=it_end)){
        if(((*it).first.get_value())>d)break;
        previous=&((*it).first);
        val = &((*it).second);
        it++;
    }
    return val->get_value();
}

Double Variable::get_min() const {
    return _min;
}

Double Variable::get_max() const {
    return _max;
}

VariableType *Variable::get_type() const {
    return _type;
}

