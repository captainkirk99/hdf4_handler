//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: hcerr.cc,v $ - implementation of hcerr class
// 
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#ifdef __POWERPC__
#undef isascii
#endif
#include <iostream.h>
#include <hdf.h>
#include <hcerr.h>

using namespace std;

//#ifdef NO_EXCEPTIONS
void fakethrow(const hcerr& e) {
    cerr << "A fatal exception has been thrown:"  << endl;
    cerr << e;
    exit(1);
}
//#endif

// print out value of exception
void hcerr::_print(ostream &out) const {
    out << "Exception:    " << _errmsg << endl 
	<< "Location: \"" << _file << "\", line " << _line << endl;
    out << "HDF Error stack:" << endl;
    for (int i=0; i<5; ++i)
	out << i << ") " << HEstring((hdf_err_code_t)HEvalue(i)) << endl;;
    return;
}

// stream the value of the exception to out
ostream& operator<<(ostream& out, const hcerr& x) {
    x._print(out);
    return out;
}

// $Log: hcerr.cc,v $
// Revision 1.4  2004/07/09 18:08:50  jimg
// Merged with release-3-4-3FCS.
//
// Revision 1.3  2004/02/06 00:36:28  jimg
// Switched from strstream to stringstream.
// Revision 1.2.8.1.2.1  2004/02/23 02:08:03  rmorris
// There is some incompatibility between the use of isascii() in the hdf library
// and its use on OS X.  Here we force in the #undef of isascii in the osx case.
//
//
// Revision 1.2  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.1  1996/10/31 18:42:59  jimg
// Added.
//
// Revision 1.3  1996/05/23  18:15:58  todd
// Added copyright statement.
//
// Revision 1.3  1996/05/23  18:15:58  todd
// Added copyright statement.
//
// Revision 1.2  1996/04/22  17:42:42  todd
// Corrected a minor bug in hcerr::_print(ostream &) const.
//
// Revision 1.1  1996/04/02  20:47:50  todd
// Initial revision
//
