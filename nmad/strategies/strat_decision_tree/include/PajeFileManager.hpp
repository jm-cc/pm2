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
  
/**
 * \file    PajeFileManager.hpp
 *  Visual Trace Explorer 
 *
 *  Release Date: January, 2nd 2010
 *  ViTE is a software to vivisualize execution trace provided by
 *  a group of student from ENSEIRB and INRIA Bordeaux - Sud-Ouest
 *
 * @version 1.1
 * @author  Kevin Coulomb
 * @author  Mathieu Faverge
 * @author  Johnny Jazeix
 * @author  Olivier Lagrasse
 * @author  Jule Marcoueille
 * @author  Pascal Noisette
 * @author  Arthur Redondy
 * @author  Clément Vuchener 
 * @date    2010-01-02
 *
 **/
  
#ifndef FILE_HPP
#define FILE_HPP
 
#include <string.h>

#define _PAJE_NBMAXTKS  16

/**
 * \struct PajeLine 
 *
 * Brief structure to store informations read 
 * on each line
 *
 * \param _id Line number
 * \param _nbtks Number of tokens found on the line
 * \param _tokens Pointers on the found tokens
 * 
 */
typedef struct PajeLine {
    int    _id;
    int    _nbtks;
    char **_tokens;
    
    PajeLine() {}
} PajeLine_t;


/**
 *  \class PajeFileManager
 *  
 *  File manager to read files using Paje syntax.
 *  Each line is read one after one and stored in 
 *  the PajeLine structure associated to the class.
 * 
 * \sa Parser
 * \sa ParserPaje
 * \sa ParserVite
 */
class PajeFileManager : public std::ifstream {

private:
    std::string           _filename;
    long long                   _filesize;

    unsigned int          _lineid;
    int                   _nbtks;
    char**                _tokens;
    std::string           _line;

    PajeFileManager(const PajeFileManager &);
    
public:
    /*!
     *  \brief Constructor for the file
     */
    PajeFileManager();
    
    /*!
     *  \brief  Constructor for the file
     *  \param  filename : a filename
     */
    PajeFileManager(const char * filename, ios_base::openmode mode = ios_base::in );
      
    /*!
     *  \brief Destructor
     *  Destroy the file
     */
    ~PajeFileManager();
        
    /*!
     *  \fn open(const char * filename, ios_base::openmode mode)
     *  \brief Open the file
     *  \param filename the file name
     *  \param mode the opening mode
     */
    void open (const char * filename, ios_base::openmode mode = ios_base::in );

    /*!
     *  \fn close()
     *  \brief Close the file if already opened
     */
    void close();

    /*!
     *  \fn get_filesize() const;
     *  \return The length of the file opened
     */
    long long get_filesize() const;

    /*!
     *  \fn get_size_loaded();
     *  \return The size already loaded
     */
    long long get_size_loaded();
    
    /*!
     *  \fn get_percent_loaded()
     *  \return The percent of the file loaded (between 0 and 1).
     */
    float get_percent_loaded();

    /*!
     *  \fn get_line(PajeLine *lineptr)
     *  \brief Get the next line.
     *  \param lineptr the line filled in
     *  \return the next line
     */
    int get_line(PajeLine *lineptr);

    /*!
     *  \fn print_line()
     *  \brief Print the current line for debug
     */
    void print_line();
};

#endif // FILE_HPP
