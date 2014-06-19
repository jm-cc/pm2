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
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <queue>
#include <list>
/* -- */
/* -- */
#include "../include/trace/values/Values.hpp"
#include "../include/trace/EntityTypes.hpp"
#include "../include/trace/Entitys.hpp"
/* -- */
#include "../include/TraceL.hpp"
#include "../include/EventL.hpp"
/* -- */
#include "../include/PajeFileManager.hpp"
#include "../include/PajeDefinition.hpp"
#include "../include/ParserDefinitionPaje.hpp"
#include "../include/ParserEventPaje.hpp"
#include "../include/ParserPaje.hpp"
/* -- */
using namespace std;

ParserPaje::ParserPaje() : _ParserDefinition(new ParserDefinitionPaje()),
                           _ParserEvent(new ParserEventPaje(_ParserDefinition)), _file(NULL){}

ParserPaje::ParserPaje(const string &filename) : _file_to_parse(filename),
                                                 _ParserDefinition(new ParserDefinitionPaje()),
                                                 _ParserEvent(new ParserEventPaje(_ParserDefinition)), _file(NULL) {}

ParserPaje::~ParserPaje() {
    delete _ParserDefinition;
    delete _ParserEvent;
    if (_file != NULL)
        delete _file;
}

void ParserPaje::parse(TraceL* trace) {

    static const string PERCENT = "%";
    PajeLine_t     line;
#ifdef DBG_PARSER_PAJE
    int lineid = 0;
#endif

    // Open the trace
    try {
        _file = new PajeFileManager(_file_to_parse.c_str());
    } catch (const char *) {
        delete _file;
        _file = NULL;
        _is_canceled = true;
        //finish();
        //trace.finish();
        std::cerr <<  "Cannot open file " <<  _file_to_parse.c_str() << std::endl;
        //Error::set(Error::VITE_ERR_OPEN, 0, Error::VITE_ERRCODE_WARNING);
        return;
    }

    while( (!(_file->eof())) && !(_is_canceled) ) {

        try {
#ifdef DBG_PARSER_PAJE
            if ( (lineid+1) ==  _file->get_line(&line) )
            {
                _file->print_line();
                lineid++;
            }
#else
            _file->get_line(&line);
#endif
        }
        catch(char *){
	  //Error::set(Error::VITE_ERR_EXPECT_ID_DEF, 0, Error::VITE_ERRCODE_ERROR);
            continue;
        }

        // If it's an empty line
        if (line._nbtks == 0) {
            continue;
        }
        // The line starts by a '%' : it's a definition
        else if(line._tokens[0][0] == '%') {
            _ParserDefinition->store_definition(&line);
        }
        // It's an event
        else {
            _ParserEvent->store_event(&line, trace);
        }
    }
    
    delete _file;
    _file = NULL;
}


float ParserPaje::get_percent_loaded() const {
    if (_file != NULL)
        return _file->get_percent_loaded();
    else
        return 0.;
}

ParserDefinitionPaje *ParserPaje::get_parser_def() const {
    return _ParserDefinition;
}
