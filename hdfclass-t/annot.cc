//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: annot.cc,v $ - input stream class for HDF annotations
// 
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <mfhdf.h>

#ifdef __POWERPC__
#undef isascii
#endif

#include <string>
#include <vector>

#include <hcerr.h>
#include <hcstream.h>
#include <hdfclass.h>


//
// protected member functions
//

// initialize hdfistream_annot members
void hdfistream_annot::_init(const string filename) {
    _an_id = _index = _tag = _ref = 0;
    _lab = _desc = true;
    _an_ids = vector<int32>();
    _filename = filename;
    return;
}

// initialize hdfistream_annot members, setting _tag, _ref to an object
void hdfistream_annot::_init(const string filename, int32 tag, int32 ref) {
    _init(filename);
    _tag = tag;
    _ref = ref;
    return;
}

// open a file using the annotation interface and set _filename, file_id, _an_id
void hdfistream_annot::_open(const char *filename) {
    if (_file_id != 0)
	close();
    if ( (_file_id = Hopen(filename, DFACC_READ, 0)) < 0)
	THROW(hcerr_openfile);
    if ( (_an_id = ANstart(_file_id)) < 0)
	THROW(hcerr_anninit);
    _filename = filename;
    return;
}

// retrieve information about annotations
void hdfistream_annot::_get_anninfo(void) {
    if (bos())
	_get_file_anninfo();
    else
	_get_obj_anninfo();
}


// retrieve information about the file annotations for current file
void hdfistream_annot::_get_file_anninfo(void) {
    int32 nlab, ndesc, junk, junk2;
    if (ANfileinfo(_an_id, &nlab, &ndesc, &junk, &junk2) == FAIL)
	THROW(hcerr_anninfo);

    int32 _ann_id;
    _an_ids = vector<int32>();
    int i;
    for (i=0; _lab && i<nlab; ++i) {
	if ( (_ann_id = ANselect(_an_id, i, AN_FILE_LABEL)) == FAIL)
	    THROW(hcerr_anninfo);
	_an_ids.push_back(_ann_id);
    }
    for (i=0; _desc && i<ndesc; ++i) {
	if ( (_ann_id = ANselect(_an_id, i, AN_FILE_DESC)) == FAIL)
	    THROW(hcerr_anninfo);
	_an_ids.push_back(_ann_id);
    }
    return;
}

// retrieve information about the annotations for currently pointed-at object
void hdfistream_annot::_get_obj_anninfo(void) {
    int nlab = 0, ndesc = 0;
    if (_desc  && 
	(ndesc = ANnumann(_an_id, AN_DATA_DESC, _tag, _ref)) == FAIL)
	THROW(hcerr_anninfo);
    if (_lab  &&
	(nlab = ANnumann(_an_id, AN_DATA_LABEL, _tag, _ref)) == FAIL)
	THROW(hcerr_anninfo);
    if (nlab + ndesc > 0) {
	int32 *annlist = new int32[nlab+ndesc];
	if (annlist == 0)
	    THROW(hcerr_annlist);
	if (_desc  &&  
	    ANannlist(_an_id, AN_DATA_DESC, _tag, _ref, annlist) == FAIL) {
	    delete []annlist; annlist = 0;
	    THROW(hcerr_annlist);
	}
	if (_lab  &&
	    ANannlist(_an_id, AN_DATA_LABEL, _tag, _ref, annlist+ndesc) == FAIL) {
	    delete []annlist; annlist = 0;
	    THROW(hcerr_annlist);
    	}
	
	// import into _an_ids vector
	// try {    // add this when STL supports exceptions
	_an_ids = vector<int32>(annlist[0],annlist[nlab+ndesc]);
	// }
	// catch(...) {
	// 		delete []annlist; annlist = 0;
	//		THROW(hcerr_annlist);
    	// }
	delete []annlist;
    }
    return;
}


//
// public member functions
//

hdfistream_annot::hdfistream_annot(const string filename) : 
  hdfistream_obj(filename) {
    _init(filename);
    if (_filename.length() != 0)
	open(_filename.c_str());
    return;
}

hdfistream_annot::hdfistream_annot(const string filename, int32 tag, int32 ref) : 
  hdfistream_obj(filename) {
    _init(filename);
    open(_filename.c_str(), tag, ref);
    return;
}

void hdfistream_annot::open(const char *filename) {
    _open(filename);		// get _an_id for open file
    _tag = _ref = 0;		// set tag, ref for file annotations
    _get_anninfo();		// retrieve annotation info
    return;
}

void hdfistream_annot::open(const char *filename, int32 tag, int32 ref) {
    _open(filename);		// get _an_id for open file
    _tag = tag;			// set tag, ref for object
    _ref = ref;
    _get_anninfo();		// retrieve annotation info
    return;
}
    
void hdfistream_annot::close(void) {
    if (_an_id > 0)
	(void)ANend(_an_id);
    if (_file_id > 0)
	(void)Hclose(_file_id);
    _init();
    return;
}

hdfistream_annot& hdfistream_annot::operator>>(string& an) {

    // delete any previous data in an
    an = string();

    if (_an_id == 0  ||  _index < 0)
	THROW(hcerr_invstream);
    if (eos())			// do nothing if positioned past end of stream
	return *this;
    
    int32 _ann_id = _an_ids[_index];
    char buf[hdfclass::MAXSTR];
    if (ANreadann(_ann_id, buf, hdfclass::MAXSTR-1) < 0)
	THROW(hcerr_annread);
    an = buf;
    seek_next();

    return *this;
}
    
hdfistream_annot& hdfistream_annot::operator>>(vector<string>& anv) {
    for (string an; !eos(); ) {
	*this>>an;
	anv.push_back(an);
    }
    return *this;
}
    
    
// $Log: annot.cc,v $
// Revision 1.6  2004/07/09 18:08:50  jimg
// Merged with release-3-4-3FCS.
//
// Revision 1.5.8.1.2.1  2004/02/23 02:08:03  rmorris
// There is some incompatibility between the use of isascii() in the hdf library
// and its use on OS X.  Here we force in the #undef of isascii in the osx case.
//
// Revision 1.5.8.1  2003/05/21 16:26:58  edavis
// Updated/corrected copyright statements.
//
// Revision 1.5  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.4  1999/05/06 03:23:33  jimg
// Merged changes from no-gnu branch
//
// Revision 1.3  1999/05/05 23:33:43  jimg
// String --> string conversion
//
// Revision 1.2.6.1  1999/05/06 00:35:44  jimg
// Jakes String --> string changes
//
// Revision 1.2  1998/09/10 21:57:10  jehamby
// Fix incorrect checking of HDF return values and other incorrect HDF calls.
//
// Revision 1.1  1996/10/31 18:42:55  jimg
// Added.
//
// Revision 1.3  1996/06/14  23:07:37  todd
// Fixed minor bug in operator>>(string)
//
// Revision 1.2  1996/05/23  18:15:08  todd
// Added copyright statement.
//
// Revision 1.1  1996/04/19  01:19:55  todd
// Initial revision
//