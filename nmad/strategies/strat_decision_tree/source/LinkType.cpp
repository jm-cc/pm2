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
#include <list>
#include <map>
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/ContainerType.hpp"
#include "../include/trace/EntityType.hpp"
#include "../include/trace/LinkType.hpp"
/* -- */
using namespace std;

LinkType::LinkType(Name name, ContainerType *container_type, ContainerType *source_type, ContainerType *destination_type, map<std::string, Value *> opt):
    EntityType(name, container_type, opt), _source_type(source_type), _destination_type(destination_type) {
    
}

LinkType::LinkType()
    {
    
}

LinkType::~LinkType(){
    // All the ContainerType are deleted by the trace itself (recursively)
    // We just set the pointers to NULL
    _source_type = NULL;
    _destination_type = NULL;
}


    const ContainerType * LinkType::get_source_type() const{
        return _source_type;        
    }
    const ContainerType * LinkType::get_destination_type() const{
        return _destination_type;        
    }
    void LinkType::set_source_type(ContainerType * source){
        _source_type=source;        
    }
    void LinkType::set_destination_type(ContainerType * dest){
        _destination_type=dest;        
    }
    
    
