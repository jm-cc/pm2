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
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/ContainerType.hpp"
/* -- */
using namespace std;

ContainerType::ContainerType(Name &name, ContainerType *parent): _name(name), _parent(parent), _children() {

}

ContainerType::ContainerType() {

}

ContainerType::~ContainerType() {
    // Delete children
    while (!_children.empty()){
        delete _children.front();
        _children.pop_front();
    }
}

void ContainerType::add_child(ContainerType* child) {
    _children.push_back(child);
}

const Name ContainerType::get_name() const {
    return _name;
}

const ContainerType *ContainerType::get_parent() const {
    return _parent;
}

const list<ContainerType *> *ContainerType::get_children() const {
    return &_children;
}

