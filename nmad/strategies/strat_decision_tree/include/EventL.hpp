#ifndef EVENT_HPP
#define EVENT_HPP

#include <string>
#include <map>
#include <list>
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/EntityValue.hpp"
#include "../include/trace/EntityTypes.hpp"
#include "../include/trace/Entitys.hpp"

using namespace std; 

class EventL {
private:
  /* Common attributes for both event */
  bool is_event_try_and_co;
  Date time;
  String event_id;
  bool flush_null;

  /* Attributes for try and commit event */
  Double outlist_nb_pw;
  Double outlist_size;
  Double next_pw_size;
  Double next_pw_remaining_data_area;
  
  /* Attributes for the submited pw event */
  Double pw_submited_size;
  Double gdrv_latency;
  Double gdrv_bandwidth;

public:
  EventL();
  EventL(bool _event_type, Date _time, String _event_id);
  ~EventL();
  bool is_null();
  void set_flush_null(bool);
  bool is_event_try_and_commit();
  Date get_time();
  String get_event_id();
  Double get_outlist_nb_pw();
  Double get_outlist_size();
  Double get_next_pw_size();
  Double get_next_pw_remaining_data_area();
  Double get_pw_submited_size();
  Double get_gdrv_latency();
  Double get_gdrv_bandwidth();
  void set_outlist_size (Double _outlist_size);
  void set_next_pw_size (Double _next_pw_size) ;
  void set_next_pw_remaining_data_area (Double _next_pw_remaining_data_area);
  void set_outlist_nb_pw (Double _outlist_nb_pw) ;
  void set_pw_submited_size (Double _pw_submited_size) ;
  void set_gdrv_latency (Double _gdrv_latency);
  void set_gdrv_bandwidth (Double _gdrv_bandwidth);
  bool operator==(EventL &e1);
  EventL& operator=(EventL e);
};

#endif // EVENT_HPP


