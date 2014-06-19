#include <iostream>
#include <fstream>

#include "../include/TraceL.hpp"
#include "../include/EventL.hpp"



TraceL::TraceL() {
}

TraceL::~TraceL() {}

void TraceL::addEvent(EventL e) {
  trace_list.push_back(e);
}

EventL* TraceL::getCurrentEvent() {
  return &(trace_list.back());
}


void TraceL::flush(struct nm_drv* p_drv, string f_name) {
  EventL last_try_and_commit;
  EventL last_pw_submited;
  bool is_last_event_try_and_commit = false;
  EventL e;
  std::ofstream myfile;
  string file_path = "../csv/" + f_name;
  myfile.open(file_path);

  /*------------------------------------------------*/
  /*------------------Print header------------------*/
  /*------------------------------------------------*/
  myfile << "outlist_nb_pw_old\toutlist_size_old\tnext_pw_size_old\tnext_pw_remaining_data_area_old\toutlist_nb_pw\toutlist_size\tnext_pw_size\tnext_pw_remaining_data_area\tpw_submited_size\tgdrv_latency\tgdrv_bandwidth\taggreg(yes/no)" << endl;
  
  for (list<EventL>::const_iterator it = this->trace_list.begin(), end = this->trace_list.end(); it != end; ++it) {
    e = (*it);
    if (e.is_event_try_and_commit()) {
      /*
      /* ------------------------------- */
      /* Print last try and commit value */
      /* ------------------------------- */
      
      if (!last_try_and_commit.is_null()) {
	myfile << last_try_and_commit.get_outlist_nb_pw().to_string() << "\t" << last_try_and_commit.get_outlist_size().to_string() << "\t"
	       << last_try_and_commit.get_next_pw_size().to_string() << "\t" << last_try_and_commit.get_next_pw_remaining_data_area().to_string() 
	       << "\t" ;
      
      /* ---------------------------------- */
      /* Print Current Try and commit value */
      /* ---------------------------------- */
      
      myfile <<  e.get_outlist_nb_pw().to_string() << "\t" << e.get_outlist_size().to_string() << "\t"
	     << e.get_next_pw_size().to_string() << "\t" << e.get_next_pw_remaining_data_area().to_string() << "\t" ;
      
      /* ---------------------------- */
      /* Print last Pw Submited Value */
      /* ---------------------------- */
      
      if (last_pw_submited.is_null())
	myfile << "\t-1\t-1\t-1\t" ;
      else {
	myfile << last_pw_submited.get_pw_submited_size().to_string() << "\t" << last_pw_submited.get_gdrv_latency().to_string() << "\t"
	       << last_pw_submited.get_gdrv_bandwidth().to_string() << "\t" ;
      }
      
      /* --------------------- */
      /* Print Aggreg Decision */
      /* --------------------- */
      /* TODO */
      if (
	  ( nm_ns_evaluate_transfer_time(p_drv, e.get_next_pw_size().get_value()) 
	    + nm_ns_evaluate_transfer_time(p_drv, last_try_and_commit.get_next_pw_size().get_value()) )
	  <=
	  ( nm_ns_evaluate_transfer_time(p_drv, e.get_next_pw_size().get_value() + last_try_and_commit.get_next_pw_size().get_value()) )
	  )
	myfile << "no" << '\n' ;
      else
	myfile << "yes" << '\n';

      last_try_and_commit = e;
      last_try_and_commit.set_flush_null(false);
      is_last_event_try_and_commit = true;
    }
    else {
      if (last_try_and_commit.get_outlist_nb_pw().get_value() == 1)
	last_try_and_commit.set_flush_null(true); 
      is_last_event_try_and_commit = false;
      last_pw_submited = e;
      last_pw_submited.set_flush_null(false);
    }
  }
}




void TraceL::finish(struct nm_drv* p_drv, string f_name) {
  this->trace_list.unique();
  this->flush(p_drv, f_name);
}

void TraceL::to_string() {
  std::cout << "Nb Event: " << this->trace_list.size() << endl;
  std::cout << "List contains:" << endl;
  EventL e;
  
  for (list<EventL>::const_iterator it = this->trace_list.begin(), end = this->trace_list.end(); it != end; ++it) {
    e = (*it);
    if (e.is_event_try_and_commit()) {
      cout << "Try And Commit " << e.get_event_id().to_string() << ": Time " << e.get_time().to_string() << " | Outlist_nb_pw " 
	   << e.get_outlist_nb_pw().to_string() << " | Outlist_pw_size " << e.get_outlist_size().to_string()
	   << " | Next_pw_size " << e.get_next_pw_size().to_string()
	   << " [ Next_pw_remaining.. " << e.get_next_pw_remaining_data_area().to_string() << endl;
    }
    else {
      cout << "Pw Submited " << e.get_event_id().to_string() << ": Time " << e.get_time().to_string() 
	   << " |  Pw_submited_size " << e.get_pw_submited_size().to_string()  << " | Gdrv_latency " 
	   << e.get_gdrv_latency().to_string() << " | Gdrv_bandwidth " << e.get_gdrv_bandwidth().to_string() << endl;
    }
  }
}
