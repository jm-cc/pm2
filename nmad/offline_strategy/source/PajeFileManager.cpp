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

#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
/* -- */
#include "../include/PajeFileManager.hpp"
/* -- */
using namespace std;


PajeFileManager::PajeFileManager() : _filename(""), _filesize(-1), _lineid(0), _nbtks(0) {
    _tokens = new char*[_PAJE_NBMAXTKS];
}

PajeFileManager::PajeFileManager( const char * filename, ios_base::openmode mode ) :
    _filename(filename), _filesize(-1), _lineid(0), _nbtks(0)
{
    _tokens = new char*[_PAJE_NBMAXTKS];
    open(filename, mode);
}

PajeFileManager::~PajeFileManager() {
    close();
    delete [] _tokens;
}

void PajeFileManager::open ( const char * filename, ios_base::openmode mode ) {

    _filename = filename;
    ifstream::open(filename, mode);

    if (fail())
        throw "Fail to open file";

    // get length of file:
    seekg (0, ios::end);
    _filesize = tellg();
    seekg (0, ios::beg);

    // For windows, clear the eof byte for ept files after reading the first file
    clear();
}

void PajeFileManager::close() {
    ifstream::close();
    _filesize = -1;
}

long long PajeFileManager::get_filesize() const {
    return _filesize;
}

long long PajeFileManager::get_size_loaded() {
    return tellg();
}

float PajeFileManager::get_percent_loaded() {
    if(_filesize != -1) {
        return (float)tellg() / (float)_filesize;
    }
    else {
        return 0.;
    }
}

int PajeFileManager::get_line(PajeLine *lineptr) {
    unsigned int  i    = 0;
    int  itks = 0;
    char c;

    // We set all the tokens to NULL
    for (i=0; i<_PAJE_NBMAXTKS; i++) {
        _tokens[i] = NULL;
    }

    std::getline(*this, _line);
    if (rdstate() != ifstream::goodbit) {
        setstate(ifstream::eofbit); // We stop the reading if something happens to the state of the file.
        lineptr->_nbtks = 0;
        return _lineid;
    }
    _lineid++;

    // Set the adress of the first token
    c = _line[0];
    _tokens[itks] = &_line[0];

    for(i = 0; i<_line.size() /*&& ((c != '\n') &&(c != '\r\n')&&(c != '\r') && (c != '\0'))*/ && (itks < _PAJE_NBMAXTKS); i++) {
        c = _line[i];

        switch (c) {
            // It's the end of the line, we just add the end caractere
            // for the last token and increase the counter
        case '\0' :
        case '\n' :
            {
                _line[i] = '\0';
                itks++;
                break;
            }
        case '%' :
            {
                // In these case, we just have two tokens, one with the % and one without
                itks++;
                _tokens[itks] = &_line[i+1];
                break;
            }
        case '\'' :
            {
                // Check if we are at the beginnning of a new one or not
                if ( _tokens[itks][0] == '\'' ) {
                    _line[i] ='\0';
                    _tokens[itks]++;
                } else {
                    _line[i] ='\0';
                    itks++;
                    _tokens[itks] = &_line[i+1];
                }

                // Start a long token
                while ( ( i < _filesize) && (_line[i] != '\'')) {
                    i++;
                }
                if((i==_filesize)){
                    setstate(ifstream::eofbit);
                    throw "Overflow";
                    return -1;
                }
                // We finish the token by replacing the \' by \0
                _line[i] = '\0';
                if(i+1<_line.size()){//begin a new token if we are not at the end of the line
                    itks++;
                    _tokens[itks] = &_line[i+1];
                }
                break;
            }
        case '"'  :
            {
                // Check if we are at the beginnning of a new one or not
                if ( _tokens[itks][0] == '"' ) {
                    _line[i] ='\0';
                    _tokens[itks]++;
                } else {
                    _line[i] ='\0';
                    itks++;
                    _tokens[itks] = &_line[i+1];
                }

                // Start a long token (-1 to have place or the null caractere)
                while (( i < _filesize) && (_line[i] != '"')) {
                    i++;
                }

                // We finish the token by replacing the " by \0
                _line[i] = '\0';
                if(i+1<_line.size()){//begin a new token if we are not at the end of the line
                    itks++;
                    _tokens[itks] = &_line[i+1];
                }
                if((i==_filesize)){
                    setstate(ifstream::eofbit);
                    throw "Overflow";
                    return -1;
                }
                break;
            }
        case ' '  :
        case '\t' :
            {
                // Skip all spaces
                while (( i < _filesize) && (i<_line.size()) && ((_line[i] == ' ') || (_line[i] == '\t')) ) {
                    _line[i] = '\0';
                    i++;
                }
                if((i==_filesize)){
                    setstate(ifstream::eofbit);
                    throw "Overflow";
                    return -1;
                }
                // We store a new token if it's a new space
                if(i!=_line.size()){
                    if (_tokens[itks]!=NULL && _tokens[itks][0] != '\0')
                        itks++;
                    _tokens[itks] = &_line[i];
                    i--;
                }
                break;
            }
        default :
            break;
        }
    }
    itks++; //add a token to avoid bugs, because we don't meet \n with std strings
    // We remove the last token if it is empty
    if ((itks > 0) && (_tokens[itks-1][0] == '\0'))
        itks--;

    _nbtks = itks;
    lineptr->_id     = _lineid;
    lineptr->_nbtks  = _nbtks;
    lineptr->_tokens = _tokens;

    return _lineid;
}

void PajeFileManager::print_line() {
    int i;

    cout << "==================" << _lineid << "=====================" << endl;
    for(i=0; i < _nbtks; i++) {
        cout << i << " : " << _tokens[i] << endl;
    }
    cout << "===========================================" << endl;

}
