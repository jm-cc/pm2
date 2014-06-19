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
#include <list>
#include <map>
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/ContainerType.hpp"
#include "../include/trace/EntityValue.hpp"
#include "../include/trace/EntityType.hpp"
/* -- */
using namespace std;

EntityType::EntityType(Name name, ContainerType *container_type, map<std::string, Value *>& opt):
    _name(name), _container_type(container_type) {
	if(opt.empty())_extra_fields=NULL;
	else{ _extra_fields=new map<std::string, Value *>(opt);
	opt.clear();
	}

}

EntityType::EntityType():_extra_fields(NULL)
  {}


void EntityType::add_value(EntityValue *value) {
    _values.insert ( pair<Name , EntityValue*>(value->get_name(),value)); 
}

const Name EntityType::get_name() const {
    return _name;
}

const ContainerType *EntityType::get_container_type() const {
    return _container_type;
}

const std::map<Name, EntityValue* > *EntityType::get_values() const {
    return &_values;
}

void EntityType::set_name(Name name){
    _name=name;
    }
void EntityType::set_container_type(ContainerType* container_type){
    _container_type=container_type;
}
void EntityType::set_extra_fields(std::map<std::string, Value *>* extra_fields){
    _extra_fields=extra_fields;
}

void EntityType::set_values(std::map<Name, EntityValue* > values){
    _values=values;
    }
     

EntityType::~EntityType(){
    // Destruction of the list _values
    // As long as everything has not been cleaned
    std::map<Name, EntityValue* >::iterator cur1 = _values.end();
   for(std::map<Name, EntityValue* >::iterator it1 = _values.begin();
        it1 != cur1;
        it1++){
        delete ((*it1).second);
    }
   _values.clear();

    _container_type = NULL;
	if(_extra_fields!=NULL){
		map<std::string , Value * >::iterator cur = _extra_fields->end();
	   for(map<std::string, Value *>::iterator it = _extra_fields->begin();
			it != cur;
			it++){
			delete (*it).second;
		}
	   _extra_fields->clear();
	}
}

map<std::string, Value *> *EntityType::get_extra_fields() const {
    return _extra_fields;
}

