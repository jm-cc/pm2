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

#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <stdexcept>
#include <map>
/* -- */
#include "../include/PajeDefinition.hpp"
/* -- */
using namespace std;

namespace PajeDef {

    void print(std::vector<PajeFieldName>* fields, PajeDefinition *def) {
        unsigned int i;
        size_t       size = def->_fields.size();

        cout << def->_trid << " : " << def->_name << endl;

        for(i=0 ; i < size ; i ++){
            PajeFieldName p;
            try{
                p =  (*fields).at( def->_fields[i]._idname);
                cout << " " << p._name;
            }catch(out_of_range& e){
                cout << " " << "name not found";
            }
            string s;
            //as this won't change in the future, we can use a switch
            switch(def->_fields[i]._idtype){
            case(1<<0) :
                s="int";
                break;
            case(1<<1) :
                s="hex";
                break;
            case(1<<2) :
                s="date";
                break;
            case(1<<3) :
                s="double";
                break;
            case(1<<4) :
                s="string";
                break;
            case(1<<5) :
                s="color";
                break;
            default:
                s="error : wrong type";
                break;
            }
            cout << " " << s << endl;
        }
    }

    string print_string(std::vector<PajeFieldName>* fields, PajeDefinition *def) {
        unsigned int i;
        size_t       size = def->_fields.size();
        //string       out;
        stringstream outstream;
        outstream  << def->_name;
        outstream << "\n";
        for(i=0 ; i < size ; i ++){

            //try to find the name of the field we want to print

            PajeFieldName p;
            try{
                p =  (*fields).at( def->_fields[i]._idname);
                outstream << " " << p._name;
            }catch(out_of_range& e){
                outstream << " " << "name not found";
            }
            string s;
            //as this won't change in the future, we can use a switch
            switch(def->_fields[i]._idtype){
            case(1<<0) :
                s="int";
                break;
            case(1<<1) :
                s="hex";
                break;
            case(1<<2) :
                s="date";
                break;
            case(1<<3) :
                s="double";
                break;
            case(1<<4) :
                s="string";
                break;
            case(1<<5) :
                s="color";
                break;
            default:
                s="error : wrong type";
                break;
            }
            outstream << " " << s;
            outstream << "\n";
        }

        return outstream.str();
    }

    bool check_definition(PajeDefinition *def) {
        int fdpresent = def->_fdpresent;

        // If the definition requires Alias or Name,
        // and the trace provide only one the both, it's ok
        if ( ((def->_fdrequired & CODE(_PajeFN_Alias)) ||
              (def->_fdrequired & CODE(_PajeFN_Name ))) &&
             ((def->_fdpresent  & CODE(_PajeFN_Alias)) ||
              (def->_fdpresent  & CODE(_PajeFN_Name ))) ) {
            fdpresent = fdpresent | CODE(_PajeFN_Alias);
            fdpresent = fdpresent | CODE(_PajeFN_Name );
        }

        if ( (fdpresent & def->_fdrequired) == def->_fdrequired )
            return true;
        else
            return false;
    }
}
