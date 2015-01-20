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
/*!
 * \file ParserPaje.hpp
 * \brief The implementation of Parser for Paje traces.
 */

#ifndef PARSERPAJE_HPP
#define PARSERPAJE_HPP

#include <cstdio>


class ParserDefinitionPaje;
class ParserEventPaje;
class PajeFileManager;
class TraceL;

/*!
 *
 * \class ParserPaje
 * \brief parse the input data format of Paje.
 *
 */
class ParserPaje {
private:
    ParserDefinitionPaje *_ParserDefinition;
    ParserEventPaje      *_ParserEvent;
    PajeFileManager      *_file;

    bool _is_finished;
    bool _is_canceled;
    
    std::string  _file_to_parse;
public:

    /*!
     *  \fn ParserPaje()
     */
    ParserPaje();
    ParserPaje(const std::string &filename);

    /*!
     *  \fn ~ParserPaje()
     */
    ~ParserPaje();

    /*!
     *  \fn parse(Trace &trace, bool finish_trace_after_parse = true)
     *  \param trace : the structure of data to fill
     *  \param finish_trace_after_parse boolean set if we do not have to finish the trace after parsing
     */
    void parse(TraceL *trace);
    
    /*!
     *  \fn get_percent_loaded() const
     *  \brief return the size of the file already read.
     *  \return the scale of the size already loaded of the file by the parser. (between 0 and 1)
     */
    float get_percent_loaded() const;

    /*!
     * Return the parser of definitions.
     *
     */
    ParserDefinitionPaje *get_parser_def() const;
};

#endif // PARSERPAJE_HPP
