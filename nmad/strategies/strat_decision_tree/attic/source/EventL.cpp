#include<iostream>

#include "../include/EventL.hpp"


using namespace std;

EventL::EventL() {}

EventL::EventL(bool _event_type, Date _time, String _event_id) {
  this->event_id = _event_id;
  this->time = _time;
  this->is_event_try_and_co = _event_type;
  this->flush_null = true;
}


EventL::~EventL() {}

bool EventL::is_null() {
  return this->flush_null;
}

void EventL::set_flush_null(bool b) {
  this->flush_null = b;
}

Date EventL::get_time() {
  return this->time;
}

String EventL::get_event_id() {
  return this->event_id;
}

Double EventL::get_outlist_nb_pw() {
  return this->outlist_nb_pw;
}

Double EventL:: get_outlist_size() {
  return this->outlist_size;
}

Double EventL::get_next_pw_size() {
  return this->next_pw_size;
}

Double EventL::get_next_pw_remaining_data_area() {
  return this->next_pw_remaining_data_area;
}


Double EventL::get_pw_submited_size() {
  return this->pw_submited_size;
}


Double EventL::get_gdrv_latency() {
  return this->gdrv_latency;
}

Double EventL::get_gdrv_bandwidth() {
  return this->gdrv_bandwidth;
}

bool EventL::is_event_try_and_commit() {
  return this->is_event_try_and_co;
}
  
void EventL::set_outlist_size (Double _outlist_size) {
  this->outlist_size = _outlist_size;
}

void EventL::set_next_pw_size (Double _next_pw_size) {
  this->next_pw_size = _next_pw_size;
}

void EventL::set_next_pw_remaining_data_area (Double _next_pw_remaining_data_area) {
  this->next_pw_remaining_data_area = _next_pw_remaining_data_area;
}

void EventL::set_outlist_nb_pw (Double _outlist_nb_pw) {
  this->outlist_nb_pw = _outlist_nb_pw;
}

void EventL::set_pw_submited_size (Double _pw_submited_size) {
  this->pw_submited_size = _pw_submited_size;
}

void EventL::set_gdrv_latency (Double _gdrv_latency) {
  this->gdrv_latency = _gdrv_latency;
}


void EventL::set_gdrv_bandwidth (Double _gdrv_bandwidth) {
  this->gdrv_bandwidth = _gdrv_bandwidth;
}


bool EventL::operator==(EventL &e1) {
      if (e1.is_event_try_and_commit()){
	if (this->is_event_try_and_commit()){ 
	  return ( (e1.get_outlist_nb_pw().get_value() == this->get_outlist_nb_pw().get_value()) &&
		   (e1.get_outlist_size().get_value()  == this->get_outlist_size().get_value() ) &&
		   (e1.get_next_pw_size().get_value()  == this->get_next_pw_size().get_value() ) &&
		   (e1.get_next_pw_remaining_data_area().get_value() ==
		    this->get_next_pw_remaining_data_area().get_value()) );
	}
	else {
	  return false;
	}
      }
}

EventL& EventL::operator=(EventL e) {
  is_event_try_and_co = e.is_event_try_and_commit();
  time = e.get_time();
  event_id = e.get_event_id();
  flush_null = e.is_null();
  if (is_event_try_and_co) {
    outlist_nb_pw = e.get_outlist_nb_pw();
    outlist_size = e.get_outlist_size();
    next_pw_size = e.get_next_pw_size();
    next_pw_remaining_data_area = e.get_next_pw_remaining_data_area();
  }
  else {
    pw_submited_size = e.get_pw_submited_size();
    gdrv_latency = e.get_gdrv_latency();
    gdrv_bandwidth = e.get_gdrv_bandwidth();
  }

  return *this;
}
