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
 *\file info.cpp
 */

#include "../include/common/common.hpp"
#include "../include/common/Info.hpp"
//#include "../include/common/Session.hpp"
#include <string>
using namespace std;

unsigned int Info::Screen::width  = 800;
unsigned int Info::Screen::height = 600;

Element_pos Info::Container::x_min = 0;
Element_pos Info::Container::x_max = 0;
Element_pos Info::Container::y_min = 0;
Element_pos Info::Container::y_max = 0;

Element_pos Info::Entity::x_min = 0;
Element_pos Info::Entity::x_max = 0;
Element_pos Info::Entity::y_min = 0;
Element_pos Info::Entity::y_max = 0;

Element_pos Info::Render::width  = 100; /* 100 OpenGL units for 1 pixel  */
Element_pos Info::Render::height = 100; /* 100 OpenGL units for 1 pixel  */

bool        Info::Render::_key_alt  = false;
bool        Info::Render::_key_ctrl = false;
Element_pos Info::Render::_x_min_visible = 0.0;
Element_pos Info::Render::_x_max_visible = 0.0;
Element_pos Info::Render::_x_min = 0.0;
Element_pos Info::Render::_x_max = 0.0;

Element_pos Info::Render::_info_x = 0.0;
Element_pos Info::Render::_info_y = 0.0;
Element_pos Info::Render::_info_accurate = 0.0;

bool        Info::Render::_no_arrows     = false;
bool        Info::Render::_no_events     = false;

// bool        Info::Render::_shaded_states = Session::getSession().get_shaded_states_setting();
//true;/* By default, enable shaded state */
// bool        Info::Render::_vertical_line = Session::getSession().get_vertical_line_setting();
//true;/* By default, enable vertical line */

bool Info::Splitter::split = false;
bool Info::Splitter::load_splitted = false;
bool Info::Splitter::preview = false;
std::string Info::Splitter::path ;
std::string Info::Splitter::filename ;
std::string Info::Splitter::xml_filename ;
Element_pos Info::Splitter::_x_min = 0.0;
Element_pos Info::Splitter::_x_max = 0.0;


int Info::Trace::depth=0;

void Info::release_all(){

    Info::Container::x_min = 0;
    Info::Container::x_max = 0;
    Info::Container::y_min = 0;
    Info::Container::y_max = 0;

    Info::Entity::x_min = 0;
    Info::Entity::x_max = 0;
    Info::Entity::y_min = 0;
    Info::Entity::y_max = 0;

    Info::Render::_x_min_visible = 0.0;
    Info::Render::_x_max_visible = 0.0;
}
