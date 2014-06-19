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
 * \file PajeDefinition.hpp
 * \brief This file contains the definition during decoding the trace file.
 */

#ifndef DEFINITION_HPP
#define DEFINITION_HPP

/*!
 * \brief Paje EventDef Names
 * This names are the names allowed to defined the events in Paje
 * format. This list can't be extended in the trace.
 */
typedef enum paje_eventdef_e {
    _PajeDefineContainerType,
    _PajeDefineEventType,
    _PajeDefineStateType,
    _PajeDefineVariableType,
    _PajeDefineLinkType,
    _PajeDefineEntityValue,
    _PajeCreateContainer,
    _PajeDestroyContainer,
    _PajeNewEvent,
    _PajeSetState,
    _PajePushState,
    _PajePopState,
    _PajeResetState,
    _PajeSetVariable,
    _PajeAddVariable,
    _PajeSubVariable,
    _PajeStartLink,
    _PajeEndLink,
    PAJE_EVENTDEF_SIZE
} paje_eventdef_t;

/*!
 * \brief Names of the fields describing events
 *        This is a non echaustive list of the names which can be used
 *        to described the fields of each events. This list conatins
 *        only the field's name required by some events. It can be
 *        automatically extended inside the trace to be adfapted to
 *        the need of the user.
 */
#define FIELDNAME_SIZEMAX           32
#define FIELDNAME_SIZE              14
#define _PajeFN_Time                 0
#define _PajeFN_Name                 1
#define _PajeFN_Alias                2
#define _PajeFN_Type                 3
#define _PajeFN_Container            4
#define _PajeFN_StartContainerType   5
#define _PajeFN_EndContainerType     6
#define _PajeFN_StartContainer       7
#define _PajeFN_EndContainer         8
#define _PajeFN_Color                9
#define _PajeFN_Value               10
#define _PajeFN_Key                 11
#define _PajeFN_File                12
#define _PajeFN_Line                13

/*
 * Former type that are no longer used in Paje Format.
 * Kept here for compatibility
 */
#define _PajeFN_ContainerType        3
#define _PajeFN_EntityType           3
#define _PajeFN_SourceContainerType  5
#define _PajeFN_DestContainerType    6
#define _PajeFN_SourceContainer      7
#define _PajeFN_DestContainer        8

/*!
 * \brief Types for the fields
 *        This is the list of the type allowed tyo store the different
 *        fields of each event. This list can't be extended.
 */
#define FIELDTYPE_SIZE               6
#define _FieldType_Int            1<<0
#define _FieldType_Hex            1<<1
#define _FieldType_Date           1<<2
#define _FieldType_Double         1<<3
#define _FieldType_String         1<<4
#define _FieldType_Color          1<<5

#define CODE(name) ( 1 << name )
#define CODE2(name) ( 1 << _PajeFN_##name )

struct Field {
    int _idname;
    int _idtype;
};

struct PajeFieldName {
    const char *_name;
    int   _id;
    int   _code;
    int   _allowed;
};

struct PajeDefinition {
    const char *_name;
    int   _id;
    int   _trid;
    int   _fdpresent;
    int   _fdrequired;
    std::vector< Field > _fields;
};

namespace PajeDef {
    void        print           (std::vector<PajeFieldName>* fields, PajeDefinition *def);
    std::string print_string    (std::vector<PajeFieldName>* fields, PajeDefinition *def);
    bool        check_definition(PajeDefinition *def);
};

#endif // DEFINITION_HPP
