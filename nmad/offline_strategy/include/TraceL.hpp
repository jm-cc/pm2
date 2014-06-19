#ifndef TRACE_HPP
#define TRACE_HPP

#include <iostream>
#include <list>
typedef int8_t nm_trk_id_t;

extern "C"
{
#include <tbx_fast_list.h>
#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_core.h>
#include <nm_core_interface.h>
#include <nm_session_private.h>
#include <nm_session_interface.h>
#include <nm_minidriver.h>
#include <nm_trk_cap.h>
#include <nm_drv.h>
#include <nm_sampling.h>
}

using namespace std;

class EventL;

class TraceL {
private:
  list<EventL> trace_list;

 
public:
  TraceL();
  ~TraceL();

  void addEvent(EventL e);
  void flush(struct nm_drv* p_drv, string f_name);
  EventL* getCurrentEvent();
  void finish(struct nm_drv* p_drv, string f_name);
  void to_string();
};

#endif // TRACE_HPP

