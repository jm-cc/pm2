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
#include "../include/trace/EntityValue.hpp"
#include "../include/trace/Entitys.hpp"
#include "../include/trace/Container.hpp"
/* -- */
#include "../include/statistics/Statistic.hpp"
/* -- */
using namespace std;

Statistic::Statistic(){
    _event       = 0;
    _number_link = 0;
}

Statistic::~Statistic(){
    // Delete states
    for (map<const EntityValue *, stats *>::iterator it = _states.begin();
        it != _states.end();
        it ++) {
        delete (*it).second;
    }
}

void Statistic::add_state(EntityValue const* ent, double length){
    map<const EntityValue *, stats*>::iterator i = _states.find(ent);
    // If it does not exist, add a new entry
    if (i == _states.end()) {
        _states[ent] = new stats(length);
    }
    // Else update the state found
    else {
        (*i).second->_total_length += length;
        (*i).second->_cardinal ++;
    }
}

void Statistic::add_link(const Container * cont){
    map<const Container*, int>::iterator i = _link.find(cont);
   // If it does not exist, add a new entry
     if(i == _link.end()) {
         _link[cont] = 1;
     }
   // Else update the link to the container found
    else{
        (*i).second ++;
        _number_link ++;
    }
}

void Statistic::set_nb_event(int n){
    _event = n;
}

int Statistic::get_nb_event() const {
    return _event;
}

map<const EntityValue*, stats*> Statistic::get_states(){
    return _states;
}
