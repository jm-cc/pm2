#define typeof __typeof__
#undef NMAD_TRACE

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <queue>
#include <list>

/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/EntityTypes.hpp"
#include "../include/trace/Entitys.hpp"
/* -- */
#include "../include/ParserPaje.hpp"
#include "../include/TraceL.hpp"

typedef int8_t nm_trk_id_t;
 
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



int main(int argc, char**argv){

  nm_session_t   p_session = NULL;
  int rank;
  nm_launcher_init(&argc, argv); /* Here */

  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  nm_core_t p_core = p_session->p_core;
  nm_drv_t p_drv = NULL;
  nm_drv_t p_drv_ib = NULL;
  string file_path;
  string prog_name;
  string file_name;


  if (rank != 0) {
    tbx_fast_list_for_each_entry(p_drv, &(p_core)->driver_list, _link)
      {
	string str = string(p_drv->driver->name);
	if (str == "minidriver")
	  p_drv_ib = p_drv;
      }
    file_name = string(argv[1]);
    file_path = "../trace/" + string(argv[1]);
    unsigned pos = file_name.find(".trace"); 
    prog_name = file_name.substr(0, pos) + ".csv";
    cout << file_name << endl;
    ParserPaje* parser = new ParserPaje(file_path);
    TraceL* trace = new TraceL();
    parser->parse(trace);
    trace->finish(p_drv_ib, prog_name);
  }

  nm_launcher_exit();
  return 0;
}
