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
#include <map>
#include <list>
#include <vector>
#include <stack>
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/EntityTypes.hpp"
#include "../include/trace/Entitys.hpp"
#include "../include/trace/Entity.hpp"
/* -- */
using namespace std;

Entity::Entity(Container *container, map<std::string, Value *> opt): _container(container)/*, _extra_fields(opt)*/ {
 if(opt.empty())_extra_fields=NULL;
	else{ _extra_fields=new map<std::string, Value *>(opt);
	opt.clear();
	}
			

}

Entity::Entity():_extra_fields(NULL){};

const Container *Entity::get_container() const {
    return _container;
}

const map<std::string, Value *> *Entity::get_extra_fields() const {
    return _extra_fields;
}


Entity::~Entity(){
//     for(map<std::string, Value *>::iterator it = _extra_fields.begin();
//         it != _extra_fields.end();
//         it++){
//         delete (*it).second;
//         (*it).second = NULL;
//     }
}

void Entity::clear(){
	if(_extra_fields !=NULL)delete _extra_fields;     
	_extra_fields=NULL;
}
