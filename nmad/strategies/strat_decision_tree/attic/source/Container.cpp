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
#include <iostream>
#include <cstring>
#include <string>
#include "stdio.h"
#include <map>
#include <list>
#include <vector>
#include <stack>
#include <algorithm>
/* -- */
#include "../include/common/common.hpp"
#include "../include/common/Info.hpp"
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/EntityTypes.hpp"
#include "../include/trace/Entitys.hpp"
#include "../include/trace/tree/Interval.hpp"
#include "../include/trace/tree/Node.hpp"
#include "../include/trace/tree/BinaryTree.hpp"
/* -- */

#include "../include/statistics/Statistic.hpp"
/* -- */
#include "../include/trace/Container.hpp"
/* -- */

using namespace std;


Container::Container():
    _name(),
    _creation_time(0.0),
    _destruction_time(0.0),
    _type(NULL),
    _parent(NULL),
    _n_states(0),
    _state_tree(NULL),
    _n_events(0),
#ifdef USE_ITC
    _events(NULL),
#endif
    _event_tree(NULL), _n_variables(0)
{}

Container::Container(Name name,
                     Date creation_time,
                     ContainerType *type,
                     Container *parent,
                     map<string, Value *> &opt):
    _name(name), _creation_time(creation_time),
    _destruction_time(0.0), _type(type),
    _parent(parent), _n_states(0), _state_tree(NULL),
    _n_events(0), _event_tree(NULL), _n_variables(0),
    _extra_fields(opt)
{
    if (parent) {
        _depth = parent->_depth+1;
    }
    else {
        _depth = 0;
    }

#ifdef USE_ITC
    IntervalOfContainer * itc= new IntervalOfContainer();
    _intervalsOfContainer.push_back(itc);
    _events=NULL;
#endif
}

template <class T>
void MyDelete(T *ptr){
    delete ptr;
};

Container::~Container()
{
    std::for_each(_children.begin(),
                  _children.end(),
                  MyDelete<Container>);
    _children.clear();
    _children.resize(0);
#ifdef USE_ITC
    std::for_each(_intervalsOfContainer.begin(),
                  _intervalsOfContainer.end(),
                  MyDelete<IntervalOfContainer>);
    _intervalsOfContainer.clear();
#endif

    // Delete states
    // delete _state_tree;
    _state_tree = NULL;
    // Delete events
    //delete _event_tree;
    _event_tree = NULL;
#ifndef USE_ITC
    // Delete links
    while (!_links.empty()){
        delete _links.front();
        _links.pop_front();
    }
    // Delete variables
    for(map<VariableType *, Variable *>::iterator it = _variables.begin();
        it != _variables.end();
        it++){
        delete (*it).second;
    }
    _variables.clear();
#endif
}

void Container::add_child(Container *child) {
    if(child != NULL){
        _children.push_back(child);
    }
}

void Container::add_view_child(Container *child) {
    if(child != NULL){
        _view_children.push_back(child);
    }
}

void Container::add_current_state(Date end) {
#ifdef USE_ITC
    State* new_state=_intervalsOfContainer.back()->add_state(
        _current_states.top().start,
        end,
        _current_states.top().type,
        _current_states.top().value,
        this,
        _current_states.top().opt);

    if (_n_states!=0)
        (_intervalsOfContainer.back()->_statechanges[_intervalsOfContainer.back()->_n_statechanges-1]).set_right_state(new_state);
    else {
        _n_states++;
        _intervalsOfContainer.back()->add_statechange(new_state->get_start_time(), NULL, new_state);
    }
    if(_intervalsOfContainer.back()->add_statechange(end, new_state,0)==false){
#ifdef BOOST_SERIALIZE
        if(Info::Splitter::split)dump_last_itc();
#endif
        //this interval is full, create a new one
        IntervalOfContainer* itc=new IntervalOfContainer();

        State* first_state=itc->add_state(
            _current_states.top().start,
            end,
            _current_states.top().type,
            _current_states.top().value,
            this,
            _current_states.top().opt);

        _intervalsOfContainer.push_back(itc);
        //add the state to the new intervalOfContainer
        itc->add_statechange(end, first_state,0);
    }

#else
    State *new_state = new State(
        _current_states.top().start,
        end,
        _current_states.top().type,
        this,
        _current_states.top().value,
        _current_states.top().opt);

    // Set the change to the new state
    if (!_states.empty())
        _states.back()->set_right_state(new_state);
    else {
        _n_states++;
        _states.push_back(new StateChange(new_state->get_start_time(), NULL, new_state));
    }

    // Set the change from the new state
    _states.push_back(new StateChange(end, new_state));
#endif
    _n_states++;
}

void Container::set_state(Date time, StateType *type, EntityValue *value, map<string, Value *> &opt) {


    if (!_current_states.empty()) {
        add_current_state(time);
        _current_states.pop();
    }

    current_state_t t(time, type, value, opt);
    _current_states.push(t);
}

void Container::push_state(Date time, StateType *type, EntityValue *value, map<string, Value *> &opt) {
    if (!_current_states.empty())
        add_current_state(time);

    current_state_t t(time, type, value, opt);
    _current_states.push(t);
}

void Container::pop_state(Date time) {
    if (!_current_states.empty()) {
        add_current_state(time);
        _current_states.pop();
    }

    if (!_current_states.empty()) {
        _current_states.top().start = time;
    }
}

void Container::reset_state(Date time) {
    if (!_current_states.empty()) {
        add_current_state(time);
        while( !_current_states.empty() ){
            _current_states.pop();
        }
    }
}

void Container::new_event(Date time, EventType *type, EntityValue *value, map<string, Value *> &opt) {
#ifdef USE_ITC
    _intervalsOfContainer.back()->add_event(time,  type,this, value, opt);
#else
    _events.push_back(new Event(time, type, this, value, opt));
    _n_events++;
#endif
}


void Container::start_link(Date time, LinkType *type, Container *source,
                           EntityValue *value, String key, map<std::string, Value *> &opt)
{
    map<String, std::list<current_link_t>, String::less_than>::iterator i = _current_received_links.find(key);

    if (i == _current_received_links.end())
    {
        //no message is to be ended, just push the new one in the list
        current_link_t t(time, type, source, value, opt);
        map<String, std::list<current_link_t>, String::less_than>::iterator it = _current_sent_links.find(key);
        if (it == _current_sent_links.end()) {
            //printf ("received a send order for a new key\n");
            _current_sent_links[key] = std::list<current_link_t>();
            _current_sent_links[key].push_back(t);
        }
        else{
            //printf ("received a send order for an existing key\n");
            _current_sent_links[key].push_back(t);
        }

    }
    else
    {
        //if the key exists, we need to end a message, the first one correponnding to the key
        current_link_t* mess = &(*i).second.front();
        for (map<string, Value *>::const_iterator j = opt.begin();
             j != opt.end(); j++)
        {
            mess->opt[(*j).first] = (*j).second;
        }
#ifdef USE_ITC
        if( _intervalsOfContainer.back()->add_link(time,
                                                   mess->start,
                                                   type,
                                                   this,
                                                   source,
                                                   mess->source,
                                                   value,
                                                   mess->opt)==false){
#ifdef BOOST_SERIALIZE
            if(Info::Splitter::split)dump_last_itc();
#endif
            //this interval is full, create a new one
            IntervalOfContainer* itc=new IntervalOfContainer();
            _intervalsOfContainer.push_back(itc);
            //add the state to the new intervalOfContainer
            _intervalsOfContainer.back()->add_link(time,
                                                   mess->start,
                                                   type,
                                                   this,
                                                   source,
                                                   mess->source,
                                                   value,
                                                   mess->opt);
        }
#else
        _links.push_back(new Link(
                             time,
                             mess->start,
                             type,
                             this,
                             source,
                             mess->source,
                             value,
                             mess->opt));
#endif
        //remove the first element from the list for that key
        (*i).second.pop_front();

        if ((*i).second.empty())_current_received_links.erase(i);

    }
}

void Container::end_link(Date time, Container *destination, String key, map<string, Value *> &opt) {

    map<String, std::list<current_link_t>, String::less_than>::iterator i = _current_sent_links.find(key);
    if (i == _current_sent_links.end())
    {
        //no message is to be ended, just push the new one in the list
        current_link_t t(time, NULL, destination, NULL, opt);

        map<String, std::list<current_link_t>, String::less_than>::iterator it = _current_received_links.find(key);
        if (it == _current_received_links.end()) {
            //printf ("received a receive order for a new key\n");
            _current_received_links[key] = std::list<current_link_t>();
            _current_received_links[key].push_back(t);
        }
        else{
            //printf ("received a receive order for an existing key\n");
            _current_received_links[key].push_back(t);
        }
    }
    else
    {

        //if the key exists, we need to end a message, the first one correponnding to the key
        current_link_t* mess = &(*i).second.front();


        for (map<string, Value *>::const_iterator j = opt.begin();
             j != opt.end(); j++)
        {
            mess->opt[(*j).first] = (*j).second;
        }
#ifdef USE_ITC
        if(_intervalsOfContainer.back()->add_link(mess->start,
                                                  time,
                                                  mess->type,
                                                  this,
                                                  mess->source,
                                                  destination,
                                                  mess->value,
                                                  mess->opt)==false){
#ifdef BOOST_SERIALIZE
            if(Info::Splitter::split)dump_last_itc();
#endif
            //this interval is full, create a new one
            IntervalOfContainer* itc=new IntervalOfContainer();
            _intervalsOfContainer.push_back(itc);
            //add the state to the new intervalOfContainer
            _intervalsOfContainer.back()->add_link(mess->start,
                                                   time,
                                                   mess->type,
                                                   this,
                                                   mess->source,
                                                   destination,
                                                   mess->value,
                                                   mess->opt);
        }
#else
        _links.push_back(new Link(
                             mess->start,
                             time,
                             mess->type,
                             this,
                             mess->source,
                             destination,
                             mess->value,
                             mess->opt));
#endif

        //remove the first element from the list for that key
        (*i).second.pop_front();

        if ((*i).second.empty())_current_sent_links.erase(i);
    }
}

void Container::set_variable(Date time, VariableType *type, Double value) {
#ifdef USE_ITC
    int i=0;
    IntervalOfContainer* itc= _intervalsOfContainer.back();
    for(i=0;i< itc->_n_variables && itc->_variables[i].get_type()!=type; i++) ;

    if (i==itc->_n_variables) {
        itc->set_variable(this, type);
        itc->_variables[i].add_value(time, value);
    }
    else {
        itc->_variables[i].add_value(time, value);
    }

#else
    map<VariableType *, Variable *>::iterator i = _variables.find(type);
    if (i == _variables.end()) {
        _variables[type] = new Variable(this, type);
        _variables[type]->add_value(time, value);
        _n_variables++;
    }
    else {
        (*i).second->add_value(time, value);
    }
#endif
}

void Container::add_variable(Date time, VariableType *type, Double value) {
#ifdef USE_ITC
    int i=0;
    IntervalOfContainer* itc= _intervalsOfContainer.back();
    for(i=0;i< itc->_n_variables && itc->_variables[i].get_type()!=type; i++) ;

    if (i==itc->_n_variables) {

        itc->set_variable(this, type);
        itc->_variables[i].add_value(time, value);
    }
    else {
        itc->_variables[i].add_value(time,itc->_variables[i].get_last_value()+ value);
    }
#else
    map<VariableType *, Variable *>::iterator i = _variables.find(type);
    if (i == _variables.end()) {
        _variables[type] = new Variable(this, type);
        _variables[type]->add_value(time, value);
        _n_variables++;
    }
    else {
        (*i).second->add_value(time, (*i).second->get_last_value() + value);
    }
#endif
}

void Container::sub_variable(Date time, VariableType *type, Double value) {
#ifdef USE_ITC
    int i=0;
    IntervalOfContainer* itc= _intervalsOfContainer.back();
    for(i=0;i< itc->_n_variables && itc->_variables[i].get_type()!=type; i++) ;

    if (i==itc->_n_variables) {
        itc->set_variable(this, type);
        itc->_variables[i].add_value(time, value);
    }
    else {
        itc->_variables[i].add_value(time,itc->_variables[i].get_last_value()- value);
    }
#else
    map<VariableType *, Variable *>::iterator i = _variables.find(type);
    if (i == _variables.end()) {
        _variables[type] = new Variable(this, type);
        _variables[type]->add_value(time, -value);
        _n_variables++;
    }
    else {
        (*i).second->add_value(time, (*i).second->get_last_value() - value);
    }
#endif
}

Name Container::get_name() const {
    return _name;
}

const Container *Container::get_parent() const {
    return _parent;
}

const ContainerType *Container::get_type() const {
    return _type;
}

const Container::Vector * Container::get_children() const {
    return &_children;
}

const Container::Vector * Container::get_view_children() const {
    return &_view_children;
}

void Container::clear_children() {
    _children.clear();
}

void Container::clear_view_children() {
    _view_children.clear();
}

Date Container::get_creation_time() const {
    return _creation_time;
}

Date Container::get_destruction_time() const {
    return _destruction_time;
}

StateChange::Tree *Container::get_states() const {
    return _state_tree;
}

Event::Tree *Container::get_events() const {
    return _event_tree;
}

const Link::Vector *Container::get_links() const {
    return &_links;
}

const map<VariableType *, Variable *> *Container::get_variables() const {
    return &_variables;
}

const map<string, Value *> *Container::get_extra_fields() const {
    return &_extra_fields;
}

unsigned int Container::get_variable_number() const {
    return _n_variables;
}

unsigned int Container::get_state_number() const {
    return _n_states;
}

unsigned int Container::get_event_number() const {
    return _n_events;
}

#ifdef USE_ITC


const std::list<IntervalOfContainer* >* Container::get_intervalsOfContainer() const{
    return &_intervalsOfContainer;
}


void Container::set_intervalsOfContainer(std::list<IntervalOfContainer* >* itc) {
    _intervalsOfContainer.assign(itc->begin(),itc->end());

}

#ifdef BOOST_SERIALIZE
/*!
 * \fn dump(std::string path, std::filename) const
 * \brief Dump all IntervalOfContainers to disk
 */
void Container::dump(std::string path, std::string filename,const Date &time){

    if (_destruction_time.get_value() == 0.0)
        destroy(time);

    int _uid=Serializer<Container>::Instance().getUid(this);
    int n=0;

    list<IntervalOfContainer* > ::const_iterator it_end1= _intervalsOfContainer.end();
    for(list<IntervalOfContainer *>::const_iterator it = _intervalsOfContainer.begin() ;
        it != it_end1;
        it++){

        if( ( *it)->_loaded ==true){
            char* name=(char*)malloc(512*sizeof(char*));
            sprintf(name, "%s/%s/%d_%d",path.c_str(),filename.c_str(),_uid ,n);
            (*it)->_loaded=false;
            SerializerDispatcher::Instance().dump(*it, name);

        }
        n++;
    }
}

/*!
 * \fn dump_last_itc() const
 * \brief Dumps the last finished IntervalOfContainer to disk
 */
void Container::dump_last_itc(){
    int _uid=Serializer<Container>::Instance().getUid(this);
    if(_intervalsOfContainer.back()->_loaded==true){
        char* name=(char*)malloc(512*sizeof(char));//free in the dump_on_disk
        sprintf(name, "%s/%s/%d_%d",Info::Splitter::path.c_str(),Info::Splitter::filename.c_str(),_uid ,(int)_intervalsOfContainer.size()-1);
        SerializerDispatcher::Instance().dump(_intervalsOfContainer.back(), name);
    }
}
#endif
#endif

void Container::destroy(const Date &time) {
    if (!_current_states.empty()) {
        add_current_state(time);
        _current_states.pop();
    }
    _destruction_time = time;
}


#ifdef BOOST_SERIALIZE

void Container::loadItcInside(Interval* splitInterval){
    _states.clear();
    _links.clear();
    _variables.clear();
    delete _events;
    _n_states=0;
    _n_events=0;
    _events=NULL;


    bool loading=true;
    int _uid=Serializer<Container>::Instance().getUid(this);
    //Info::Render::_x_max_visible= Info::Render::_x_max_visible==0.0 ? 10000000 : Info::Render::_x_max_visible;

    // printf("min: %lf, max: %lf\n", Info::Render::_x_min_visible, Info::Render::_x_max_visible);

    //test if we are past the last element of the container -> if yes, load its last itc

    int i=0;



    list<IntervalOfContainer* > ::const_iterator it_end1= _intervalsOfContainer.end();
    for(list<IntervalOfContainer *>::const_iterator it = _intervalsOfContainer.begin() ;
        it != it_end1;
        it++){

        if(((*it)->_beginning<=splitInterval->_left && (*it)->_end>=splitInterval->_left) ||
           ((*it)->_beginning>=splitInterval->_left && (*it)->_beginning<=splitInterval->_right)) {

            if((*it)->_loaded==false){
                char* name=(char*)malloc(512*sizeof(char));
                sprintf(name, "%s/%d_%d",Info::Splitter::path.c_str(),_uid ,i);
                //printf("%s : need to load itc beginning at %lf and finishing at %lf, file %s \n", _name.to_string().c_str(), (*it)->_beginning.get_value(), (*it)->_end.get_value(), name);
                SerializerDispatcher::Instance().load(*it, name);
                //loading=(*it)->retrieve(name);
                if(loading==true)(*it)->_loaded=true;
                // else printf("bug de loading\n");

            }

        }else{
            //if the itc is loaded, unload it !
            if((*it)->_loaded==true){
                (*it)->unload();
                (*it)->_loaded=false;
                // printf("%s :unload itc beginning at %lf and finishing at %lf \n", _name.to_string().c_str(), (*it)->_beginning.get_value(), (*it)->_end.get_value());
            }

        }
        i++;



    }

    //  free(name);


}

void Container::loadPreview(){

    _n_states=0;
    _n_events=0;
    _events=NULL;
    StateType* type=new StateType(Name(String("Aggregated Data")), _type,  map<std::string, Value *>());
    list<IntervalOfContainer* > ::const_iterator it_end= _intervalsOfContainer.end();
    for(list<IntervalOfContainer *>::const_iterator it = _intervalsOfContainer.begin() ;
        it != it_end;
        it++){
        if((*it)->_states_values_map!=NULL && (*it)->_states_values_map->size()!=0){

            map<EntityValue*, double>::iterator it2;
            double begin = (*it)->_beginning.get_value();
            // show content:

            for ( it2=(*it)->_states_values_map->begin() ; it2 != (*it)->_states_values_map->end(); it2++ ){
                State* t= new State(begin, begin + (*it2).second , type, this,(*it2).first,  map<std::string, Value *>());

                if(_n_states==0){
                    StateChange* sc=new StateChange(begin, NULL, t);
                    _states.push_back(sc);
                    _n_states++;
                }else{
                    _states.back()->set_right_state(t);

                }
                begin += (*it2).second;
                StateChange* sc=new StateChange(begin,t, NULL);

                _states.push_back(sc);
                _n_states++;
            }
        }

    }

    /*   if(_n_events==0)_event_tree=NULL;
     else _event_tree = new BinaryTree<Event>(_events, _n_events);

     if(_n_states==0)_state_tree=NULL;
     else _state_tree = new BinaryTree<StateChange>(_states,_n_states);*/

}

#endif

void Container::finish(const Date &time) {

#ifdef USE_ITC
    //	if( Info::Splitter::split==false && Info::Splitter::preview ==false){

    //boolean to check if the intervalOfcontainer is fully inside the visible interval, in order not to check each state if yes
    //		bool fully_contained=false;

    if(_event_tree)delete _event_tree;
    if(_state_tree!=NULL)delete _state_tree;
    _n_variables=0;
    _event_tree=NULL;
    _state_tree=NULL;

    if(Info::Splitter::load_splitted==false){
        if (_destruction_time.get_value() == 0.0)
            destroy(time);



        _states.clear();
        _links.clear();
        _variables.clear();
        delete _events;
        _n_states=0;
        _n_events=0;
        _n_variables=0;
        _events=NULL;
    }

    //iterates through all intervalOfContainers
    list<IntervalOfContainer* > ::const_iterator it_end= _intervalsOfContainer.end();
    for(list<IntervalOfContainer *>::const_iterator it = _intervalsOfContainer.begin() ;
        it != it_end;
        it++){
        //only work with intervalOfContainers which are loaded in memory
        if(((*it)->_loaded)==true){
            //add all statechanges to the global one
            if((*it)->_n_statechanges!=0){
                for(int i=0; i< (*it)->_n_statechanges; i++){
                    _states.push_back(&((*it)->_statechanges[i]));
                    _n_states++;
                }
            }

            if((*it)->_n_events!=0)
                for(int i=0; i< (*it)->_n_events; i++){
                    if(_events==NULL){
                        _events=(Event**)malloc((_n_events+N)*sizeof(Event*));
                        memset(_events, 0, (_n_events+N)*sizeof(Event*));
                    }
                    _events[_n_events]=(&((*it)->_events[i]));
                    _n_events++;
                    if(_n_events%N==0)_events=(Event**)realloc(_events,(_n_events+N)*sizeof(Event*));
                }

            if((*it)->_n_links!=0)
                for(int i=0; i< (*it)->_n_links; i++){
                    _links.push_back(&((*it)->_links[i]));
                }

            if((*it)->_n_variables!=0)
                for(int i=0; i< (*it)->_n_variables; i++){
                    VariableType* type=((*it)->_variables[i]).get_type();

                    if (_variables.find(type)==_variables.end()){
                        _variables.insert(std::pair<VariableType*, Variable*>(type,&((*it)->_variables[i])));

                        //printf("%s inserting variable %s %d\n", this->get_name().to_string().c_str(),((*it)->_variables[i]).get_type()->get_name().to_string().c_str(), ((*it)->_variables[i]).get_values()->size());
                        _n_variables++;
                    }else{
                        if(((*it)->_variables[i]).get_values()!=NULL){
                            /* int nb_items=0;
                             for(nb_items=0; nb_items<((*it)->_variables[i]).get_values()->size(); nb_items++){
                             pair<Date, Double> t= ((*it)->_variables[i]).get_values()->[nb_items];
                             printf("add first %lf value %lf\n", t.first.get_value(), t.second.get_value());
                             _variables[type]->add_value(t.first, t.second);

                             }*/
                            std::list<std::pair<Date, Double> >::const_iterator  it_end2=((*it)->_variables[i]).get_values()->end();
                            std::list<std::pair<Date, Double> >::const_iterator  it_val = ((*it)->_variables[i]).get_values()->begin();
                            for(;
                                it_val!=it_end2 ;
                                it_val++){
                                pair<Date, Double> t= *it_val;

                                _variables[type]->add_value(t.first, t.second);
                            }
                        }
                    }
                }
        }


        //  }

        /*if(_n_states!=0 && (_states.back()->get_right_state()!=NULL)) {
         //add a new state at the end if not already presently
         StateChange* end = new StateChange(_destruction_time,_states.back()->get_right_state(), NULL );
         _states.push_back(end);
         _n_states++;

         }*/

    }
    if(_n_events==0)_event_tree=NULL;
    else _event_tree = new BinaryTree<Event>(_events, _n_events);

    if(_n_states==0)_state_tree=NULL;
    else _state_tree = new BinaryTree<StateChange>(_states,_n_states);
#else
    if (_destruction_time.get_value() == 0.0)
        destroy(time);
    _event_tree = new Event::Tree(_events, _n_events);
    _state_tree = new StateChange::Tree(_states,_n_states);
#endif
}



void Container::fill_stat( Statistic * stat, Interval I){

    // If the container is a proc -> no child container
    if(this->get_children()->empty()) {
        browse_stat_link(this,stat,I);
    }
    Node<StateChange> * sauv = NULL;
    Node<StateChange> ** prev = &sauv;
    Node<StateChange> * node=NULL;
    if(this->get_states()!=NULL)node = this->get_states()->get_root();
    double tmp;
    browse_stat_state(node,stat,I,prev);
    // To add the first partial state of the interval
    if(sauv &&
       sauv->get_element() &&
       sauv->get_element()->get_right_state()){
        if(sauv->get_element()->get_right_state()->get_end_time() >
           I._left){
            if( sauv->get_element()->get_right_state()->get_end_time() > I._right)
                tmp = I._right - I._left;
            else
                tmp = sauv->get_element()->get_right_state()->get_end_time() - I._left;

            stat->add_state(sauv->get_element()->get_right_state()->get_value(),tmp);
        }
    }

    stat->set_nb_event(this->get_event_number());
}


void browse_stat_state(Node<StateChange> * node, Statistic * stats, Interval I, Node<StateChange>** prev){

    if( ! node ||
        ! node->get_element())
        return;

    // If the node is in the interval
    if(node->get_element()->get_time() <= I._right &&
       node->get_element()->get_time() >= I._left){

        if(node->get_left_child())
            browse_stat_state(node->get_left_child(),stats,I,prev);
        if(node->get_right_child())
            browse_stat_state(node->get_right_child(),stats,I,prev);


        //Add the right state of the state change
        if(node->get_element()->get_right_state())
        {
            if(node->get_element()->get_right_state()->get_end_time() < I._right){
                stats->add_state(node->get_element()->get_right_state()->get_value(),node->get_element()->get_right_state()->get_duration());
            }
            else{
                stats->add_state(node->get_element()->get_right_state()->get_value(),I._right - node->get_element()->get_right_state()->get_start_time());
            }
        }
    }

    // Else if after the interval
    else if(node->get_element()->get_time() > I._right ){
        if(node->get_left_child()){
            browse_stat_state(node->get_left_child(),stats,I,prev);
        }
    }

    else{        // Else node is before the interval
        if( !(*prev) ){
            *prev = node;
        }

        if(node->get_element()->get_right_state()){
            if(node->get_element()->get_right_state()->get_start_time() >=
               (*prev)->get_element()->get_right_state()->get_start_time()){
                *prev = node;
            }
        }

        if(node->get_right_child()){
            browse_stat_state(node->get_right_child(),stats,I,prev);
        }
    }
}


void browse_stat_link(const Container * cont, Statistic * S, Interval I){
    if(!cont)
        return;

    Link::VectorIt const  &it_end = cont->get_links()->end();
    for(list<Link *>::const_iterator it = cont->get_links()->begin() ;
        it != it_end;
        it++){
        if( (*it)->get_source() == cont  &&
            (*it)->get_destination() != cont){
            S->add_link(cont);
        }
        else if((*it)->get_source() != cont  &&
                (*it)->get_destination() == cont){
            S->add_link(cont);
        }
    }
    browse_stat_link(cont->get_parent(), S, I);
}

int Container::get_depth(){
    return _depth;
}
