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

#include <cstdio>
#include <iostream>
/* -- */
#include <locale.h>   // For dots or commas separator in double numbers
#include <time.h>
/* -- */
#include "../include/common/common.hpp"
#include "../include/common/Tools.hpp"

#ifdef WIN32
#define sscanf sscanf_s
#endif
/* -- */
using namespace std;

bool convert_to_double(const std::string &arg, double *val) {
    unsigned int nb_read;
    // Try to convert first in the current locale
    sscanf(arg.c_str(), "%lf%n", val, &nb_read);

    if(nb_read == arg.size()) {
        return true; // It is the good format
    }
    else{ // We change the locale to test
        bool is_english_system_needed = false; // We have dot as separator of decimal and integer.
        
        if(arg.find('.') != string::npos) {
            is_english_system_needed = true;
        }

        // We had dots initially, we need to have the english system
        if(is_english_system_needed) {
	  if((setlocale(LC_NUMERIC, "C")           == NULL) && 
	     (setlocale(LC_NUMERIC, "en_GB.UTF-8") == NULL)){
	      vite_warning("The locale en_GB.UTF-8 is unavailable so the decimal pointed will not be printed");
            }
        }
        else { // It is comma separated
            
	  if ((setlocale(LC_NUMERIC, "fr_FR.UTF-8") == NULL) && 
	      (setlocale(LC_NUMERIC, "French")      == NULL)){
	      vite_warning("The locale fr_FR.UTF-8 is unavailable so the decimal with comma will not be printed");
            }
	    
        }
        
        // Reads the value in the new locale
        sscanf(arg.c_str(), "%lf%n", val, &nb_read);
        return nb_read == arg.size();
    }
}

double clockGet (void)
{
#ifdef _MSC_VER
    // TODO
    return 0.;
#elif (defined X_ARCHalpha_compaq_osf1) || (defined X_ARCHi686_mac)
  struct rusage       data;
  getrusage (RUSAGE_SELF, &data);
  return (((double) data.ru_utime.tv_sec  + (double) data.ru_stime.tv_sec) +
          ((double) data.ru_utime.tv_usec + (double) data.ru_stime.tv_usec) * 
	  1.0e-6L);
#elif defined _POSIX_TIMERS && defined CLOCK_REALTIME
  struct timespec tp;
  
  clock_gettime (CLOCK_REALTIME, &tp);            /* Elapsed time */

  return ((double) tp.tv_sec + (double) tp.tv_nsec * (double)1.0e-9L);
#endif
}

