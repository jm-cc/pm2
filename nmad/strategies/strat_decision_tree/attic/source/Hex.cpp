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
#include <sstream>
#include <cstdio>
#include <iomanip> // For std::setprecision
/* -- */
#include "../include/trace/values/Value.hpp"
#include "../include/trace/values/Hex.hpp"

#ifdef WIN32
#define sscanf sscanf_s
#endif

Hex::Hex() : _value(0) {
    _is_correct = true;
}

Hex::Hex(unsigned int n) : _value(n) {
    _is_correct = true;
}

Hex::Hex(const std::string &in) {
    unsigned int n;
    if(sscanf(in.c_str(), "%X", &n) != 1) {
        _is_correct = false;
    }
    else {
        _value = n;
    }
}

std::string Hex::to_string() const {
    std::ostringstream oss;
    oss.flags(std::ios::hex);
    oss << std::setprecision(Value::_PRECISION) << _value;
    return oss.str();    
}
