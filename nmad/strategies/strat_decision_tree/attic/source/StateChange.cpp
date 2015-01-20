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
/* -- */
using namespace std;

StateChange::StateChange() {

}

StateChange::StateChange(Date time): _time(time) {

}

StateChange::StateChange(Date time, State *left, State *right):
    _time(time), _left(left), _right(right) {
    
}

void StateChange::set_right_state(State *right) {
    _right = right;
}
    
Date StateChange::get_time() const {
    return _time;
}

State *StateChange::get_left_state() {
    return _left;
}

State *StateChange::get_right_state() {
    return _right;
}

const State *StateChange::get_left_state() const {
    return _left;
}

const State *StateChange::get_right_state() const {
    return _right;
}

StateChange::~StateChange(){
    delete _right;
    _right = NULL;
}
void StateChange::clear(){
        if(_left!=NULL){
            _left->clear();
            //free(_left_state);
         //  Alloc<State2>::Instance().free(_left_state);
        }
           
}
