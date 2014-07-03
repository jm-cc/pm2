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
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <queue>
/* -- */
#include "../include/PajeFileManager.hpp" // temporary
#include "../include/PajeDefinition.hpp"
#include "../include/ParserDefinitionPaje.hpp"
/* -- */
using namespace std;

#ifdef WIN32
#define sscanf sscanf_s
#define sprintf sprintf_s
#endif

#define _OUT_A_DEFINITION 0
#define _IN_A_DEFINITION  1

#define INSERT_FNAME(name, allowed)					\
    {									\
        _FieldNames[_PajeFN_##name]._name    = #name;			\
        _FieldNames[_PajeFN_##name]._id      = _PajeFN_##name;		\
        _FieldNames[_PajeFN_##name]._code    = CODE(_PajeFN_##name);	\
        _FieldNames[_PajeFN_##name]._allowed = allowed;			\
        i++;								\
    }

#define INSERT_EVENT(name, required)			\
    {							\
        _EventDefs[_##name]._name       = #name;        \
        _EventDefs[_##name]._id         = _##name;	\
        _EventDefs[_##name]._trid       = -1;		\
        _EventDefs[_##name]._fdpresent  = 0;		\
        _EventDefs[_##name]._fdrequired = required;	\
        i++;						\
    }


std::map<std::string, int> ParserDefinitionPaje::_EvDefStr2Id;
std::map<std::string, int> ParserDefinitionPaje::_FTypeStr2Code;
bool                       ParserDefinitionPaje::_initialized = false;
int                        ParserDefinitionPaje::_nbparsers   = 0;

ParserDefinitionPaje::ParserDefinitionPaje() {
    int i;

    _state = _OUT_A_DEFINITION;
    _defid = -1;
    _nbparsers++;

    if ( !_initialized ) {
        _initialized = true;

        // Initialize the list of EventDef's names
        _EvDefStr2Id.insert(pair<string, int>("PajeDefineContainerType", _PajeDefineContainerType ));
        _EvDefStr2Id.insert(pair<string, int>("PajeDefineEventType"    , _PajeDefineEventType     ));
        _EvDefStr2Id.insert(pair<string, int>("PajeDefineStateType"    , _PajeDefineStateType     ));
        _EvDefStr2Id.insert(pair<string, int>("PajeDefineVariableType" , _PajeDefineVariableType  ));
        _EvDefStr2Id.insert(pair<string, int>("PajeDefineLinkType"     , _PajeDefineLinkType      ));
        _EvDefStr2Id.insert(pair<string, int>("PajeDefineEntityValue"  , _PajeDefineEntityValue   ));
        _EvDefStr2Id.insert(pair<string, int>("PajeCreateContainer"    , _PajeCreateContainer     ));
        _EvDefStr2Id.insert(pair<string, int>("PajeDestroyContainer"   , _PajeDestroyContainer    ));
        _EvDefStr2Id.insert(pair<string, int>("PajeNewEvent"           , _PajeNewEvent            ));
        _EvDefStr2Id.insert(pair<string, int>("PajeSetState"           , _PajeSetState            ));
        _EvDefStr2Id.insert(pair<string, int>("PajePushState"          , _PajePushState           ));
        _EvDefStr2Id.insert(pair<string, int>("PajePopState"           , _PajePopState            ));
        _EvDefStr2Id.insert(pair<string, int>("PajeResetState"         , _PajeResetState          ));
        _EvDefStr2Id.insert(pair<string, int>("PajeSetVariable"        , _PajeSetVariable         ));
        _EvDefStr2Id.insert(pair<string, int>("PajeAddVariable"        , _PajeAddVariable         ));
        _EvDefStr2Id.insert(pair<string, int>("PajeSubVariable"        , _PajeSubVariable         ));
        _EvDefStr2Id.insert(pair<string, int>("PajeStartLink"          , _PajeStartLink           ));
        _EvDefStr2Id.insert(pair<string, int>("PajeEndLink"            , _PajeEndLink             ));

        // Initialize the list of types available for field in EventDef declaration
        _FTypeStr2Code.insert(pair<string, int>("int"   ,_FieldType_Int    ));
        _FTypeStr2Code.insert(pair<string, int>("hex"   ,_FieldType_Hex    ));
        _FTypeStr2Code.insert(pair<string, int>("date"  ,_FieldType_Date   ));
        _FTypeStr2Code.insert(pair<string, int>("double",_FieldType_Double ));
        _FTypeStr2Code.insert(pair<string, int>("string",_FieldType_String ));
        _FTypeStr2Code.insert(pair<string, int>("color" ,_FieldType_Color  ));
    }


    // Initialize the list of Field's names for EventDef declaration
    i = 0;
    _FieldNames.resize( FIELDNAME_SIZEMAX );
    INSERT_FNAME(Time,                _FieldType_Date                    );
    INSERT_FNAME(Name,                _FieldType_Int | _FieldType_String );
    INSERT_FNAME(Alias,               _FieldType_Int | _FieldType_String );
    INSERT_FNAME(Type,                _FieldType_Int | _FieldType_String );
    INSERT_FNAME(Container,           _FieldType_Int | _FieldType_String );
    INSERT_FNAME(StartContainerType,  _FieldType_Int | _FieldType_String );
    INSERT_FNAME(EndContainerType,    _FieldType_Int | _FieldType_String );
    INSERT_FNAME(StartContainer,      _FieldType_Int | _FieldType_String );
    INSERT_FNAME(EndContainer,        _FieldType_Int | _FieldType_String );
    INSERT_FNAME(Color,               _FieldType_Color );
    INSERT_FNAME(Value,               _FieldType_Int | _FieldType_String | _FieldType_Double);
    INSERT_FNAME(Key,                 _FieldType_Int | _FieldType_String );
    INSERT_FNAME(File,                _FieldType_String );
    INSERT_FNAME(Line,                _FieldType_Int | _FieldType_String );

    // Initialize the map between the field names and the associated id
    _nbFieldNames = FIELDNAME_SIZE;
    for(i=0; i<FIELDNAME_SIZE; i++) {
        _FNameStr2Id.insert(pair<string, int>(_FieldNames[i]._name, i));
    }

    // Former types no longer used in Paje format
    _FNameStr2Id.insert(pair<string, int>("ContainerType"      , _PajeFN_ContainerType       ));
    _FNameStr2Id.insert(pair<string, int>("EntityType"         , _PajeFN_EntityType          ));
    _FNameStr2Id.insert(pair<string, int>("SourceContainerType", _PajeFN_SourceContainerType ));
    _FNameStr2Id.insert(pair<string, int>("DestContainerType"  , _PajeFN_DestContainerType   ));
    _FNameStr2Id.insert(pair<string, int>("SourceContainer"    , _PajeFN_SourceContainer     ));
    _FNameStr2Id.insert(pair<string, int>("DestContainer"      , _PajeFN_DestContainer       ));

    // Initialize the list of Events available
    i = 0;
    _EventDefs.resize(PAJE_EVENTDEF_SIZE);
    INSERT_EVENT(PajeDefineContainerType, CODE2(Name) | CODE2(Alias) | CODE2(Type) );
    INSERT_EVENT(PajeDefineEventType    , CODE2(Name) | CODE2(Alias) | CODE2(Type) );
    INSERT_EVENT(PajeDefineStateType    , CODE2(Name) | CODE2(Alias) | CODE2(Type) );
    INSERT_EVENT(PajeDefineVariableType , CODE2(Name) | CODE2(Alias) | CODE2(Type) );
    INSERT_EVENT(PajeDefineEntityValue  , CODE2(Name) | CODE2(Alias) | CODE2(Type)    );
    INSERT_EVENT(PajeDestroyContainer   , CODE2(Name) | CODE2(Alias) | CODE2(Time) | CODE2(Type) );
    INSERT_EVENT(PajeCreateContainer    , CODE2(Name) | CODE2(Alias) | CODE2(Time) | CODE2(Type) | CODE2(Container) );
    INSERT_EVENT(PajeDefineLinkType     , CODE2(Name) | CODE2(Alias) | CODE2(Type) | CODE2(StartContainerType) | CODE2(EndContainerType) );
    INSERT_EVENT(PajeSetState           , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) );
    INSERT_EVENT(PajePushState          , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) );
    INSERT_EVENT(PajePopState           , CODE2(Time) | CODE2(Type)  | CODE2(Container) );
    INSERT_EVENT(PajeResetState         , CODE2(Time) | CODE2(Type)  | CODE2(Container) );
    INSERT_EVENT(PajeNewEvent           , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) );
    INSERT_EVENT(PajeSetVariable        , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) );
    INSERT_EVENT(PajeAddVariable        , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) );
    INSERT_EVENT(PajeSubVariable        , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) );
    INSERT_EVENT(PajeStartLink          , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) | CODE2(Key) | CODE2(StartContainer) );
    INSERT_EVENT(PajeEndLink            , CODE2(Time) | CODE2(Type)  | CODE2(Container) | CODE2(Value) | CODE2(Key) | CODE2(EndContainer) );
}

ParserDefinitionPaje::~ParserDefinitionPaje() {
    int i;

    // Free the string to store extra names
    for( i=FIELDNAME_SIZE ; i<_nbFieldNames; i++) {
        delete[] _FieldNames[i]._name;
    }
    _FieldNames.clear();
    _EventDefs.clear();
    _FNameStr2Id.clear();
    _EvDefTrId2Id.clear();

    _nbparsers--;
    if ( (_nbparsers == 0) && _initialized ) {
        _FTypeStr2Code.clear();
        _EvDefStr2Id.clear();
        _initialized = false;
    }
}

void ParserDefinitionPaje::enter_definition(const PajeLine_t *line){
    int   defid, trid;
    char *defname = line->_tokens[2];
    char *tridstr = line->_tokens[3];

    // The name is missing
    if(defname == NULL) {
      //Error::set(Error::VITE_ERR_EXPECT_NAME_DEF, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // The id is missing
    if(tridstr == NULL) {
      //Error::set(Error::VITE_ERR_EXPECT_ID_DEF, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // If we are already trying to define an event,
    // we close it before starting a new one
    if(_state == _IN_A_DEFINITION){
      //Error::set(Error::VITE_ERR_EXPECT_END_DEF, line->_id, Error::VITE_ERRCODE_WARNING);
        leave_definition(line);
    }

    // Get the trid of the definition in the trace (trid)
    if(sscanf(tridstr, "%u", &trid) != 1){
      //Error::set(Error::VITE_ERR_EXPECT_ID_DEF, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // Try to find the name
    if ( _EvDefStr2Id.find(defname) == _EvDefStr2Id.end() ) {
      //Error::set(Error::VITE_ERR_UNKNOWN_EVENT_DEF, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    defid = _EvDefStr2Id[defname];

    // Check if it is the first time we see this Event
    if ( _EventDefs[defid]._trid != -1 ) {
      //Error::set(Error::VITE_ERR_EVENT_ALREADY_DEF, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // Extra tokens are not supported
    if(line->_nbtks > 4){
      //Error::set(Error::VITE_ERR_EXTRA_TOKEN, line->_id, Error::VITE_ERRCODE_WARNING);
    }

    // Everything is ok, we start to store the event
    _EventDefs[defid]._trid = trid;
    _defid = defid;
    _state = _IN_A_DEFINITION;
    _EvDefTrId2Id.insert(pair<int, int>(trid, defid));

    return;
}

void ParserDefinitionPaje::leave_definition(const PajeLine_t *line){

    // We can't end a definition if we are not in one !!!
    if(_state != _IN_A_DEFINITION){
      //Error::set(Error::VITE_ERR_EXPECT_EVENT_DEF, line->_id, Error::VITE_ERRCODE_WARNING);
        return;
    }

    _state = _OUT_A_DEFINITION;

    // Check that the event has been correctly defined
    if( !(PajeDef::check_definition(&_EventDefs[_defid])) ) {
      /* Error::set(Error::VITE_ERR_EVENT_NOT_CORRECT
                   + PajeDef::print_string(&_FieldNames, &_EventDefs[_defid]),
                   line->_id, Error::VITE_ERRCODE_ERROR); */

        // We remove the definition
        _EvDefTrId2Id.erase(_EventDefs[_defid]._trid);
        _EventDefs[_defid]._fields.clear();
        _EventDefs[_defid]._trid = -1;
        return;
    }

#ifdef DBG_PARSER_PAJE
    PajeDef::print(&_FieldNames,&_EventDefs[_defid]);
#endif
    _defid = -1;
}

void ParserDefinitionPaje::add_field_to_definition(const PajeLine_t *line){
    char *fieldname = line->_tokens[1];
    char *fieldtype = line->_tokens[2];
    int   idname;
    int   idtype;
    Field fd;

    // Check we are currently defining an event
    if (_state == _OUT_A_DEFINITION) {
      //Error::set(Error::VITE_ERR_EXPECT_EVENT_DEF,   line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // If fieldtype is not defined
    if (fieldname == NULL) {
      // Error::set(Error::VITE_ERR_FIELD_NAME_MISSING, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // If fieldtype is not defined
    if (fieldtype == NULL) {
      //Error::set(Error::VITE_ERR_FIELD_TYPE_MISSING, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    idtype = _FTypeStr2Code[fieldtype];

    // Type unknown
    if ( idtype  == 0 ) {
      // Error::set(Error::VITE_ERR_FIELD_TYPE_UNKNOWN + fieldtype, line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // Name unknown, we have to store it
    if ( _FNameStr2Id.find(fieldname) == _FNameStr2Id.end() ) {
        int   size  = strlen(fieldname)+1;
        char *newfn = new char[size];
#ifndef WIN32
        strcpy(newfn, fieldname);
#else
        strcpy_s(newfn, size, fieldname);
#endif

        _FieldNames[_nbFieldNames]._name    = newfn;
        _FieldNames[_nbFieldNames]._id      = _nbFieldNames;
        _FieldNames[_nbFieldNames]._code    = (1 << _nbFieldNames);
        _FieldNames[_nbFieldNames]._allowed = idtype;
        idname = _nbFieldNames;
        _nbFieldNames++;
    } else {
        idname =_FNameStr2Id[fieldname];
    }

    // check if type is allowed
    if ( !( idtype & _FieldNames[idname]._allowed ) ) {
      // Error::set(Error::VITE_ERR_FIELD_NOT_ALLOWED + "(" + fieldname + "," + fieldtype + ")", line->_id, Error::VITE_ERRCODE_ERROR);
        return;
    }

    // Extra tokens are not supported
    if(line->_nbtks > 3){
      // Error::set(Error::VITE_ERR_EXTRA_TOKEN, line->_id, Error::VITE_ERRCODE_WARNING);
    }

    // We had the field and his type to the definition
    fd._idname = idname;
    fd._idtype = idtype;
    _EventDefs[_defid]._fields.push_back( fd );
    _EventDefs[_defid]._fdpresent |= _FieldNames[idname]._code;
}


void ParserDefinitionPaje::store_definition(const PajeLine_t *line){
    string token = line->_tokens[1];

    // We need almost two tokens '%' and "[End]EventDef"
    if(line->_nbtks < 2){
      //Error::set(Error::VITE_ERR_EMPTY_DEF, line->_id, Error::VITE_ERRCODE_WARNING);
        return;
    }

    // We start a definition
    if (token.compare("EventDef") == 0){
        enter_definition(line);
    }
    // We stop a definition
    else if (token.compare("EndEventDef") == 0) {
        leave_definition(line);
        if(line->_nbtks > 2){
	  // Error::set(Error::VITE_ERR_EXTRA_TOKEN, line->_id, Error::VITE_ERRCODE_WARNING);
        }
    }
    // Add a field to the definition
    else{
        add_field_to_definition(line);
    }

}

PajeDefinition *ParserDefinitionPaje::getDefFromTrid(int trid) {
    if ( _EvDefTrId2Id.find(trid) == _EvDefTrId2Id.end() ) {
        return NULL;
    } else {
        return &(_EventDefs[_EvDefTrId2Id[trid]]);
    }
}

void ParserDefinitionPaje::print_definitions() {
    map<int, int>::iterator it     = _EvDefTrId2Id.begin();
    map<int, int>::iterator it_end = _EvDefTrId2Id.end();

    for (; it != it_end ; ++ it){

        cout << "###### " << (*it).first << endl;
        // (*it).second represents the current definition
        PajeDef::print(&_FieldNames, &_EventDefs[(*it).second]);
    }
}

int ParserDefinitionPaje::definitions_number() const {
    return _EvDefTrId2Id.size();
}

const vector<PajeFieldName> *ParserDefinitionPaje::get_FieldNames() const {
    return &_FieldNames;
}
