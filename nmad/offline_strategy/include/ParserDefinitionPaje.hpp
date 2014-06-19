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
 *\file ParserDefinitionPaje.hpp
 */

#ifndef PARSERDEFINITIONPAJE_HPP
#define PARSERDEFINITIONPAJE_HPP

struct PajeDefinition;
struct PajeFieldName;

/*!
 * \class ParserDefinitionPaje
 * \brief Parse the header of the trace and store the information
 *        about event description.
 *
 * \param _EvDefStr2Id
 *        It is a hashtable to find quickly a match for
 *        each word found in definition section. It actually also
 *        define the keyword which are allowed in the definition
 *        section of a trace. This list is static, we initialize it
 *        only once for all traces. The second term of each term is
 *        the id of the event in the vector _EventDefs.
 *
 * \param _EvDefTrId2Id
 *        It is a hashtable to find quickly a match for
 *        each trace id used in the file to match an event.
 *        This hashtable is specific to each trace.
 *
 * \param _FTypeStr2Code
 *        It is an hashtable to find quickly a match for the keyword
 *        describing a field type. This hashtable describe only the
 *        string allowed and can't be extended. This list is static,
 *        we initialize it only once for all traces. And the seond
 *        term of each pair is a unique code based on one binary
 *        digit.
 *
 * \param _FNameStr2Id
 *        It is an hashtable to find quickly a match for
 *        the keyword naming a definition's field. This hashtable
 *        can be complete by some new field added in the trace
 *        file. The second term of each pair is the id of the Name in
 *        the vector _FieldNames.
 *
 */

class ParserDefinitionPaje {

    /**
     * Reads line to find events definition
     *
     */

private:

    static std::map<std::string, int> _EvDefStr2Id;
    static std::map<std::string, int> _FTypeStr2Code;
    static bool                       _initialized;
    static int                        _nbparsers;

    std::map<int, int>                _EvDefTrId2Id;
    std::map<std::string, int>        _FNameStr2Id;

    std::vector<PajeDefinition>       _EventDefs;
    std::vector<PajeFieldName>        _FieldNames;
    int                               _nbFieldNames;

    int                               _state;
    int                               _defid;

    /*!
     *  \fn enter_definition(const PajeLine_t *line)
     *  \param PajeLine line
     *  \brief Put the parser in definition mode
     */
    void enter_definition(const PajeLine_t *line);

    /*!
     *  \fn leave_definition(const PajeLine_t *line)
     *  \param line
     */
    void leave_definition(const PajeLine_t *line);

    /*!
     *  \fn add_field_to_definition(const PajeLine_t *line)
     *  \param line : the structure of data to fill
     */
    void add_field_to_definition(const PajeLine_t *line);

public:
    /*!
     * \fn ParserDefinitionPaje
     * \brief constructor
     */
    ParserDefinitionPaje();
    ~ParserDefinitionPaje();

    /*!
     *  \fn store_definition(const PajeLine_t *line)
     *  \param line the line to store.
     */
    void store_definition(const PajeLine_t *line);

    /*!
     *  \fn getDefFromTrid(int trid)
     *  \param i : the unsigned integer matching the definition we want
     *  \return : the i-th definition
     */
    PajeDefinition *getDefFromTrid(int trid);

    /*! \fn print_definitions()
     * \brief Print all the definitions. Useful for debug.
     */
    void print_definitions();

    /*!
     *  \fn definitions_number() const
     *  \return : the number of definitions in the map
     */
    int definitions_number() const;

    const std::vector<PajeFieldName> * get_FieldNames() const;

};

#endif // PARSERDEFINITIONPAJE_HPP
