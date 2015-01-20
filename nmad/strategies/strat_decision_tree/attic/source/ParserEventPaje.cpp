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
#include <set>
#include <queue>
#include <list>
#include <cstdlib>
/* -- */
#include "../include/TraceL.hpp"
#include "../include/EventL.hpp"
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/EntityValue.hpp"
#include "../include/trace/EntityTypes.hpp"
#include "../include/trace/Entitys.hpp"
/* -- */
#include <fstream>
#include <sstream>
#include "../include/PajeFileManager.hpp" // temporary
#include "../include/PajeDefinition.hpp"
#include "../include/ParserDefinitionPaje.hpp"
#include "../include/ParserEventPaje.hpp"
/* -- */
using namespace std;
#ifdef WIN32
#define sscanf sscanf_s
#endif

ParserEventPaje::ParserEventPaje(ParserDefinitionPaje *defs) {
    _Definitions = defs;
}

ParserEventPaje::~ParserEventPaje(){
  //_containers.clear();
}

void ParserEventPaje::store_event(const PajeLine *line,
                                  TraceL          *trace) {
    string fvalue;
    string alias;
    string name;
    String type;
    String start_container_type;
    String end_container_type;
    Date   time;
    String container;
    String value_string;
    Double value_double;
    String start_container;
    String end_container;
    String key;
    Name   alias_name;


    map<std::string, Value *> extra_fields;

    const vector<PajeFieldName> *FNames = _Definitions->get_FieldNames();
    vector< Field >       *fields;
    PajeDefinition        *def;
    int                    i, trid;
    int                    defsize;
    int                    idname, idtype;

    // We check if we have an event identifier
    if(sscanf(line->_tokens[0], "%u", &trid) != 1){
      //Error::set(Error::VITE_ERR_EXPECT_ID_DEF, line->_id, Error::VITE_ERRCODE_WARNING);
        return;
    }

    // We check if the trid is available
    def = _Definitions->getDefFromTrid(trid);
    if ( def == NULL ) {
        stringstream s;
        //s << Error::VITE_ERR_UNKNOWN_ID_DEF << trid;
        //Error::set(s.str(), line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    fields  = &(def->_fields);
    defsize = fields->size();

    // We check if we have enough data for this event
    if ( defsize > (line->_nbtks - 1) ) {
      //Error::set(Error::VITE_ERR_LINE_TOO_SHORT_EVENT, line->_id,
      //           Error::VITE_ERRCODE_WARNING);
        return;
    }

    // Warning if we have extra data
    if ( defsize < (line->_nbtks - 1) ) {
      //Error::set(Error::VITE_ERR_EXTRA_TOKEN, line->_id, Error::VITE_ERRCODE_WARNING);
    }

    // Dispatch the tokens in the good fields
    for(i=0; i < defsize; i++) 
      {

        fvalue = line->_tokens[i+1];
        idname = (*fields)[i]._idname;
        idtype = (*fields)[i]._idtype;

        // Store the fvalue in the correct field
        switch( idname ) {
        case _PajeFN_Alias :
            alias = fvalue;
            break;

        case _PajeFN_Name :
            name  = fvalue;
            break;

        case _PajeFN_Type :
            type = fvalue;
            break;

        case _PajeFN_StartContainerType :            start_container_type = fvalue;
            break;

        case _PajeFN_EndContainerType :
            end_container_type = fvalue;
            break;

        case _PajeFN_Time :
            time = fvalue;
            if(!time.is_correct()) {
	      /* Error::set(Error::VITE_ERR_INCOMPATIBLE_VALUE_IN_EVENT +
                           fvalue + " (expecting a \"date\")",
                           line->_id,
                           Error::VITE_ERRCODE_WARNING); */
                return;
            }
            break;

        case _PajeFN_Container :
            container = fvalue;
            break;

        case _PajeFN_Value :
            if( idtype == _FieldType_Double ) {
	      value_double = Double(fvalue);
	      if(!value_double.is_correct()) {
		/* Error::set(Error::VITE_ERR_INCOMPATIBLE_VALUE_IN_EVENT + fvalue + " (expecting a \"double\")",
		   line->_id, Error::VITE_ERRCODE_WARNING); */
		return;
	      }
            }
            else {
                value_string = fvalue;
            }
            break;

        case _PajeFN_StartContainer :
            start_container = fvalue;
            break;

        case _PajeFN_EndContainer :
            end_container = fvalue;
            break;

        case _PajeFN_Key :
            key = fvalue;
            break;

        default :
            Value *value = NULL;
            switch( idtype ) {
            case _FieldType_String :
                value = new String(fvalue);
                break;

            case _FieldType_Double :
                value = new Double(fvalue);
                break;

            case _FieldType_Hex :
                value = new Hex(fvalue);
                break;

            case _FieldType_Date :
                value = new Date(fvalue);
                break;

            case _FieldType_Int :
                value = new Integer(fvalue);
                break;

            case _FieldType_Color :
                value = new Color(fvalue);
                break;

            default:
	      // Error::set(Error::VITE_ERR_FIELD_TYPE_UNKNOWN, line->_id, Error::VITE_ERRCODE_WARNING);
                return;
            }

            //          if(!value->is_correct()) { // Check if the value is correct or not
            //              Error::set(Error::VITE_ERR_INCOMPATIBLE_VALUE_IN_EVENT + fvalue + " (expecting a \"" + ftype + "\")",
            //                         line->_id, Error::VITE_ERRCODE_WARNING);
            //              return;
            //          }

            extra_fields[(*FNames)[idname]._name] = value;
        }
    }

    if ( (alias != "") && (name == "" ) ){
        name = alias;
    }
    if ( (name != "") && (alias == "") ) {
        alias = name;
    }
    // alias_name.set_alias(alias);
    // alias_name.set_name(name);

    switch( def->_id ) {
    case _PajeNewEvent :
    {
      
      if (type.to_string() == "Event_Try_And_Commit") {
	EventL e = EventL(true , time, value_string);
	trace->addEvent(e);
      }
      else if (type.to_string() == "Event_Pw_Submited") {
	EventL e = EventL(false, time, value_string);
	trace->addEvent(e);
      }
    }
    break;
    /* TODO */
    case _PajeSetVariable :
      {
	String type_str = type.to_string();
	EventL* e = trace->getCurrentEvent();
	
	if (type_str == "Var_Outlist_Nb_Pw") {
	  e->set_outlist_nb_pw(value_double);	  
	}
	else if (type_str == "Var_Outlist_Pw_Size") {
	  e->set_outlist_size(value_double);
	}
       	else if (type_str == "Var_Next_Pw_Size") {
	  e->set_next_pw_size(value_double);
	}
	else if (type_str == "Var_Next_Pw_Remaining_Data_Area") {
	  e->set_next_pw_remaining_data_area(value_double);
       	}
	else if (type_str == "Var_Pw_Submited_Size") {
	  e->set_pw_submited_size(value_double);
	}
	else if (type_str == "Var_Gdrv_Profile_Latency") {
	  e->set_gdrv_latency(value_double);
	}
	else if (type_str == "Var_Gdrv_Profile_bandwidth") {
	  e->set_gdrv_bandwidth(value_double);
       	}
      }
      break;
    default:
      //Error::set(Error::VITE_ERR_UNKNOWN_EVENT_DEF + def->_name, line->_id, Error::VITE_ERRCODE_WARNING);
        return;
    }
}
