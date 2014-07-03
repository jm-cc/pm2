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
#include <iomanip> // For std::setprecision
#include <cstdlib>
/* -- */
#include "../include/common/Tools.hpp"
/* -- */
#include "../include/trace/values/Value.hpp"
#include "../include/trace/values/Color.hpp"

using namespace std;

Color::Color() : _r((rand()%256)/256),
                 _g((rand()%256)/256),
                 _b((rand()%256)/256) {
    _is_correct = true;
}


Color::Color(double r, double g, double b) : _r(r), _g(g), _b(b) {
    _is_correct = true;
}

Color::Color(const Color &c) : Value(c), _r(c._r), _g(c._g), _b(c._b) {
    _is_correct = c._is_correct;
}

Color::Color(const std::string &in) {
    string separated_color[3];
    string temp = in;

    for(int i = 0 ; i < 3 ; i ++){
        size_t position_of_space = temp.find(' ');
        separated_color[i] = temp.substr(0, position_of_space);
        temp = temp.substr(position_of_space+1);
    }

    _is_correct = convert_to_double(separated_color[0], &_r) &&
                  convert_to_double(separated_color[1], &_g) &&
                  convert_to_double(separated_color[2], &_b);
}

std::string Color::to_string() const {
    std::ostringstream oss;
    oss << "<font color=\"#";
    oss << std::hex << std::setfill('0');;
    oss << std::setw(2) << (int)(_r*255+0.5)%256
        << std::setw(2) << (int)(_g*255+0.5)%256
        << std::setw(2) << (int)(_b*255+0.5)%256;
    oss << "\">";
    oss << std::dec << std::setprecision(Value::_PRECISION) << _r << " " << _g << " " << _b;
    oss << "</font>";
    return oss.str();
}

double Color::get_red() const {
    return _r;
}

double Color::get_green() const {
    return _g;
}

double Color::get_blue() const {
    return _b;
}
