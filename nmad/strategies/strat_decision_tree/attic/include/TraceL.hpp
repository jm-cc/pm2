#ifndef TRACE_HPP
#define TRACE_HPP

#include <iostream>
#include <list>
typedef int8_t nm_trk_id_t;

#define NM_SO_ALIGN_TYPE      uint32_t
#define NM_SO_ALIGN_FRONTIER  sizeof(NM_SO_ALIGN_TYPE)

extern "C"
{
#include <tbx_fast_list.h>
#include <nm_public.h>
#include <nm_launcher_interface.h>
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
  void flush(nm_drv_t p_drv, string f_name);
  EventL* getCurrentEvent();
  void finish(nm_drv_t p_drv, string f_name);
  void to_string();
};

#endif // TRACE_HPP

