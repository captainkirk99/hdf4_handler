//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (c) 1996, California Institute of Technology.
//                     U.S. Government Sponsorship under NASA Contract
//		       NAS7-1260 is acknowledged.
// 
// Author: Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: genvec.cc,v $ - implementation of HDF generic vector class
//
// $Log: genvec.cc,v $
// Revision 1.1  1996/10/31 18:42:56  jimg
// Added.
//
// Revision 1.12  1996/10/14 23:07:53  todd
// Added a new import function to hdf_genvec to allow import from a vector
// of String.
//
// Revision 1.11  1996/08/22 20:54:14  todd
// Rewrite of export function suite to correctly support casts.  Now all casts towards
// integers of greater size are supported.  (E.g., char8 -> int8 ->int16 -> int32, but
// disallow int16 -> uint32 or uint32 -> int32).
//
// Revision 1.10  1996/08/21  23:18:59  todd
// Added mfuncs to return the i'th element of a genvec.
//
// Revision 1.9  1996/07/22  17:12:20  todd
// Changed export_*() mfuncs so they return a C++ array.  Added exportv_*() mfuncs
// to return STL vectors.
//
// Revision 1.8  1996/06/18  21:49:21  todd
// Fixed pointer expressions so they would be in conformance with proper usage of void *
//
// Revision 1.7  1996/06/18  18:37:42  todd
// Added support for initialization from a subsampled array to the _init() and
// constructor mfuncs; also added support for print() to output a subsample of
// the hdf_genvec.
// Added copyright notice.
//
// Revision 1.6  1996/05/02  18:10:51  todd
// Fixed a bug in print() and in export_string().
//
// Revision 1.5  1996/04/23  21:11:50  todd
// Fixed declaration of print mfunc so it was const correct.
//
// Revision 1.4  1996/04/18  19:06:37  todd
// Added print() mfunc
//
// Revision 1.3  1996/04/04  01:11:30  todd
// Added support for empty vectors that have a number type.
// Added import() public member function.
//
// Revision 1.2  1996/04/03  00:18:18  todd
// Fixed a bug in _init(int32, void *, int)
//
// Revision 1.1  1996/04/02  20:36:43  todd
// Initial revision
//
//////////////////////////////////////////////////////////////////////////////

#include <strstream.h>
#include <mfhdf.h>
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif
#include <vector.h>
#include <hcerr.h>
#include <hdfclass.h>

// Convert an array of U with length nelts into an array of T by casting each
// element of array to a T.
template<class T, class U> 
void ConvertArrayByCast(U *array, int nelts, T **carray) {
    if (nelts == 0) {
	*carray = 0;
	return;
    }
    *carray = new T[nelts];
    if (*carray == 0)
	THROW(hcerr_nomemory);
    for (int i=0; i<nelts; ++i)
	*(*carray+i) = (T) *((U *)array+i);
}

//
// protected member functions
//

// Initialize an hdf_genvec from an input C array.  If data, begin, end, stride are
// zero, a zero-length hdf_genvec of the specified type will be initialized.
void hdf_genvec::_init(int32 nt, void *data, int begin, int end, int stride) {

    // input checking: nt must be a valid HDF number type;
    // data, nelts can optionally both together be 0.
    int32 eltsize;		// number of bytes per element
    if ( (eltsize = DFKNTsize(nt)) <= 0)
	THROW(hcerr_dftype);	// invalid number type
    bool zerovec = (data == 0  &&  begin == 0  &&  end == 0  &&  stride == 0);
    if (zerovec) {		// if this is a zero-length vector
	_nelts = 0;
	_data = 0;
    }
    else {
	if (begin < 0  || end < 0  ||  stride <= 0  ||  end < begin)
	    THROW(hcerr_range);	// invalid range given for subset of data
	if (data == 0)		
	    THROW(hcerr_invarr); // if specify a range, need a data array!

	// allocate memory for _data and assign _nt, _nelts
	int nelts = (int)((end-begin)/stride+1);
	_data = (void *)new char[nelts*eltsize];	// allocate memory
	if (_data == 0)
	    THROW(hcerr_nomemory);
	if (stride == 1)	// copy data directly
	    (void)memcpy(_data, (void *)((char *)data+begin), eltsize*nelts); 
	else {
	    for (int i=0,j=begin; i<nelts; ++i,j+=stride)	// subsample data
		memcpy((void *)((char *)_data+i*eltsize), 
		       (void *)((char *)data+j*eltsize), eltsize);
	}
	_nelts = nelts;			 // assign number of elements
    }
    _nt = nt;				 // assign HDF number type

    return;
}    

// initialize an empty hdf_genvec
void hdf_genvec::_init(void) {
    _data = (void *)0;
    _nelts = _nt = 0;
    return;
}

// initialize hdf_genvec from another hdf_genvec
void hdf_genvec::_init(const hdf_genvec& gv) {
    if (gv._nt == 0 && gv._nelts == 0 && gv._data == 0)
	_init();
    else if (gv._nelts == 0)
	_init(gv._nt, 0, 0, 0, 0);
    else
	_init(gv._nt, gv._data, 0, gv._nelts-1, 1);
    return;
}

// free up memory of hdf_genvec
void hdf_genvec::_del(void) {
    delete []_data;
    _nelts = _nt = 0;
    _data = (void *)0;
    return;
}



//
// public member functions
//

hdf_genvec::hdf_genvec(void) {
    _init();
    return;
}

hdf_genvec::hdf_genvec(int32 nt, void *data, int begin, int end, int stride) {
    _init(nt, data, begin, end, stride);
    return;
}

hdf_genvec::hdf_genvec(int32 nt, void *data, int nelts) {
    _init(nt, data, 0, nelts-1, 1);
    return;
}

hdf_genvec::hdf_genvec(const hdf_genvec& gv) {
    _init(gv);
    return;
}

hdf_genvec::~hdf_genvec(void) {
    _del();
    return;
}

hdf_genvec& hdf_genvec::operator=(const hdf_genvec& gv) {
    if (this == &gv)
	return *this;
    _del();
    _init(gv);
    return *this;
}

// import new data into hdf_genvec (old data is deleted)
void hdf_genvec::import(int32 nt, void *data, int begin, int end, int stride) {
    _del();
    if (nt == 0)
	_init();
    else
	_init(nt, data, begin, end, stride);
    return;
}

// import new data into hdf_genvec from a vector of Strings
void hdf_genvec::import(int32 nt, const vector<String>& sv) {
    static char strbuf[hdfclass::MAXSTR];

    int eltsize = DFKNTsize(nt);
    if (eltsize == 0)
	THROW(hcerr_invnt);
    if (sv.size() == 0) {
	this->import(nt);
	return;
    }

    void *obuf = new char[DFKNTsize(nt)*sv.size()];
    switch (nt) {
    case DFNT_FLOAT32: {
	float32 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((float32 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_FLOAT64: {
	float64 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((float64 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_INT8: {
	int8 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((int8 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_INT16: {
	int16 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((int16 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_INT32: {
	int32 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((int32 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_UINT8: {
	uint8 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((uint8 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_UINT16: {
	uint16 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((uint16 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_UINT32: {
	uint32 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((uint32 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_UCHAR8: {
	uchar8 val;
	for (int i=0; i<(int)sv.size(); ++i) {
	    strncpy(strbuf,sv[i].chars(),hdfclass::MAXSTR-1);
	    istrstream(strbuf,hdfclass::MAXSTR) >> val;
	    *((uchar8 *)obuf+i) = val;
	}
	break;
    }
    case DFNT_CHAR8: {
	strncpy((char *)obuf,sv[0].chars(),hdfclass::MAXSTR);
	break;
    }
    default:
	THROW(hcerr_invnt);
    }

    this->import(nt, obuf, (int)sv.size());
    return;
}

// export an hdf_genvec holding uint8 or uchar8 data to a uchar8 array
uchar8 *hdf_genvec::export_uchar8(void) const {
    uchar8 *rv = 0;
    if (_nt == DFNT_UINT8)
	ConvertArrayByCast((uint8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UCHAR8)
	ConvertArrayByCast((uchar8 *)_data, _nelts, &rv);
    else 
	THROW(hcerr_dataexport);
    return rv;
}    

// return the i'th element of the vector as a uchar8
uchar8 hdf_genvec::elt_uchar8(int i) const  {
    uchar8 rv;
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_UINT8)
	rv = (uchar8)*((uint8 *)_data+i);
    else if (_nt == DFNT_UCHAR8)
	rv = *((uchar8 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding uint8 or uchar8 data to a uchar8 vector
vector<uchar8> hdf_genvec::exportv_uchar8(void) const {
    vector<uchar8> rv = vector<uchar8>(0);
    uchar8 *dtmp = 0;
    if (_nt == DFNT_UINT8)  // cast to uchar8 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UCHAR8)
	dtmp = (uchar8 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<uchar8>(dtmp, dtmp+_nelts);
    if (dtmp != (uchar8 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding int8 or char8 data to a char8 array
char8 *hdf_genvec::export_char8(void) const {
    char8 *rv = 0;
    if (_nt == DFNT_INT8)
	ConvertArrayByCast((int8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_CHAR8)
	ConvertArrayByCast((char8 *)_data, _nelts, &rv);
    else 
	THROW(hcerr_dataexport);
    return rv;
}    

// return the i'th element of the vector as a char8
char8 hdf_genvec::elt_char8(int i) const  {
    char8 rv;
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_INT8)
	rv = (char8)*((int8 *)_data+i);
    else if (_nt == DFNT_CHAR8)
	rv = *((char8 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding int8 or char8 data to a char8 vector
vector<char8> hdf_genvec::exportv_char8(void) const {
    vector<char8> rv = vector<char8>(0);
    char8 *dtmp = 0;
    if (_nt == DFNT_INT8)  // cast to char8 array and export
	ConvertArrayByCast((int8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_CHAR8)
	dtmp = (char8 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<char8>(dtmp, dtmp+_nelts);
    if (dtmp != (char8 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8 or uint8 data to a uint8 array
uint8 *hdf_genvec::export_uint8(void) const {
    uint8 *rv = 0;
    if (_nt == DFNT_UCHAR8)
	ConvertArrayByCast((uchar8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT8)
	ConvertArrayByCast((uint8 *)_data, _nelts, &rv);
    else 
	THROW(hcerr_dataexport);
    return rv;
}    

// return the i'th element of the vector as a uint8
uint8 hdf_genvec::elt_uint8(int i) const  {
    uint8 rv;
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
	rv = (uint8)*((uchar8 *)_data+i);
    else if (_nt == DFNT_UINT8)
	rv = *((uint8 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding uchar8 or uint8 data to a uint8 vector
vector<uint8> hdf_genvec::exportv_uint8(void) const {
    vector<uint8> rv = vector<uint8>(0);
    uint8 *dtmp = 0;
    if (_nt == DFNT_UCHAR8)  // cast to uint8 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8)
	dtmp = (uint8 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<uint8>(dtmp, dtmp+_nelts);
    if (dtmp != (uint8 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding char8 or int8 data to a int8 array
int8 *hdf_genvec::export_int8(void) const {
    int8 *rv = 0;
    if (_nt == DFNT_CHAR8)
	ConvertArrayByCast((char8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_INT8)
	ConvertArrayByCast((int8 *)_data, _nelts, &rv);
    else 
	THROW(hcerr_dataexport);
    return rv;
}    

// return the i'th element of the vector as a int8
int8 hdf_genvec::elt_int8(int i) const  {
    int8 rv;
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_CHAR8)
	rv = (int8)*((char8 *)_data+i);
    else if (_nt == DFNT_INT8)
	rv = *((int8 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// export an hdf_genvec holding int8 data to a int8 vector
vector<int8> hdf_genvec::exportv_int8(void) const {
    vector<int8> rv = vector<int8>(0);
    int8 *dtmp = 0;
    if (_nt == DFNT_CHAR8)  // cast to int8 array and export
	ConvertArrayByCast((char8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_INT8)
	dtmp = (int8 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<int8>(dtmp, dtmp+_nelts);
    if (dtmp != (int8 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, uint8 or uint16 data to a uint16 array
uint16 *hdf_genvec::export_uint16(void) const {
    uint16 *rv = 0;
    if (_nt == DFNT_UCHAR8)  // cast to uint16 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT8)  // cast to uint16 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT16)
	ConvertArrayByCast((uint16 *)_data, _nelts, &rv);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a uint16
uint16 hdf_genvec::elt_uint16(int i) const  {
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
	return (uint16)*((uchar8 *)_data+i);
    else if (_nt == DFNT_UINT8)
	return (uint16)*((uint8 *)_data+i);
    else if (_nt == DFNT_UINT16)
	return *((uint16 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return 0;
}

// export an hdf_genvec holding uchar8, uint8 or uint16 data to a uint16 vector
vector<uint16> hdf_genvec::exportv_uint16(void) const {
    vector<uint16> rv = vector<uint16>(0);
    uint16 *dtmp = 0;
    if (_nt == DFNT_UCHAR8)  // cast to uint16 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8)  // cast to uint16 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT16)
	dtmp = (uint16 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<uint16>(dtmp, dtmp+_nelts);
    if (dtmp != (uint16 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, char8, uint8, int8 or int16 data to 
// an int16 array
int16 *hdf_genvec::export_int16(void) const {
    int16 *rv = 0;
    if (_nt == DFNT_UCHAR8) // cast to int16 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_CHAR8) // cast to int16 array and export
	ConvertArrayByCast((char8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT8) // cast to int16 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_INT8) // cast to int16 array and export
	ConvertArrayByCast((int8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_INT16)
	ConvertArrayByCast((int16 *)_data, _nelts, &rv);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a int16
int16 hdf_genvec::elt_int16(int i) const  {
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
	return (int16)(*((uchar8 *)_data+i));
    else if (_nt == DFNT_CHAR8)
	return (int16)(*((char8 *)_data+i));
    else if (_nt == DFNT_UINT8)
	return (int16)(*((uint8 *)_data+i));
    else if (_nt == DFNT_INT8)
	return (int16)(*((int8 *)_data+i));
    else if (_nt == DFNT_INT16)
	return *((int16 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return 0;
}

// export an hdf_genvec holding int8 or int16 data to an int16 vector
vector<int16> hdf_genvec::exportv_int16(void) const {
    vector<int16> rv = vector<int16>(0);
    int16 *dtmp = 0;
    if (_nt == DFNT_UCHAR8)  // cast to int16 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_CHAR8)  // cast to int16 array and export
	ConvertArrayByCast((char8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8)  // cast to int16 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_INT8)  // cast to int16 array and export
	ConvertArrayByCast((int8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_INT16)
	dtmp = (int16 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<int16>(dtmp, dtmp+_nelts);
    if (dtmp != (int16 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, uint8, uint16 or uint32 data to a 
// uint32 array
uint32 *hdf_genvec::export_uint32(void) const {
    uint32 *rv = 0;
    if (_nt == DFNT_UCHAR8)  // cast to uint32 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT8)   // cast to uint32 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT16)   // cast to uint32 array and export
	ConvertArrayByCast((uint16 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT32)
	ConvertArrayByCast((uint32 *)_data, _nelts, &rv);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a uint32
uint32 hdf_genvec::elt_uint32(int i) const  {
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
	return (uint32)(*((uchar8 *)_data+i));
    else if (_nt == DFNT_UINT8)
	return (uint32)(*((uint8 *)_data+i));
    else if (_nt == DFNT_UINT16)
	return (uint32)(*((uint8 *)_data+i));
    else if (_nt == DFNT_UINT32)
	return *((uint32 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return 0;
}

// export an hdf_genvec holding uchar8, uint8, uint16 or uint32 data to a 
// uint32 vector
vector<uint32> hdf_genvec::exportv_uint32(void) const {
    vector<uint32> rv = vector<uint32>(0);
    uint32 *dtmp = 0;
    if (_nt == DFNT_UCHAR8)  // cast to uint32 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8)   // cast to uint32 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT16)   // cast to uint32 array and export
	ConvertArrayByCast((uint16 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT32)
	dtmp = (uint32 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<uint32>(dtmp, dtmp+_nelts);
    if (dtmp != (uint32 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding uchar8, char8, uint8, int8, uint16, int16 or 
// int32 data to a int32 array
int32 *hdf_genvec::export_int32(void) const {
    int32 *rv = 0;
    if (_nt == DFNT_UCHAR8)  // cast to int32 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_CHAR8)  // cast to int32 array and export
	ConvertArrayByCast((char8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT8)  // cast to int32 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_INT8)  // cast to int32 array and export
	ConvertArrayByCast((int8 *)_data, _nelts, &rv);
    else if (_nt == DFNT_UINT16)
	ConvertArrayByCast((uint16 *)_data, _nelts, &rv);
    else if (_nt == DFNT_INT16)
	ConvertArrayByCast((int16 *)_data, _nelts, &rv);
    else if (_nt == DFNT_INT32)
	ConvertArrayByCast((int32 *)_data, _nelts, &rv);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a int32
int32 hdf_genvec::elt_int32(int i) const  {
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_UCHAR8)
	return (int32)(*((uchar8 *)_data+i));
    else if (_nt == DFNT_CHAR8)
	return (int32)(*((char8 *)_data+i));
    else if (_nt == DFNT_UINT8)
	return (int32)(*((uint8 *)_data+i));
    else if (_nt == DFNT_INT8)
	return (int32)(*((int8 *)_data+i));
    else if (_nt == DFNT_UINT16)
	return (int32)(*((uint16 *)_data+i));
    else if (_nt == DFNT_INT16)
	return (int32)(*((int16 *)_data+i));
    else if (_nt == DFNT_INT32)
	return *((int32 *)_data+i);
    else
	THROW(hcerr_dataexport);
    return 0;
}

// export an hdf_genvec holding uchar8, char8, uint8, int8, uint16, int16 or 
// int32 data to a int32 vector
vector<int32> hdf_genvec::exportv_int32(void) const {
    vector<int32> rv = vector<int32>(0);
    int32 *dtmp = 0;
    if (_nt == DFNT_UCHAR8)  // cast to int32 array and export
	ConvertArrayByCast((uchar8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_CHAR8)  // cast to int32 array and export
	ConvertArrayByCast((char8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT8)  // cast to int32 array and export
	ConvertArrayByCast((uint8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_INT8)  // cast to int32 array and export
	ConvertArrayByCast((int8 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_UINT16)  // cast to int32 array and export
	ConvertArrayByCast((uint16 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_INT16)  // cast to int32 array and export
	ConvertArrayByCast((int16 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_INT32)
	dtmp = (int32 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<int32>(dtmp, dtmp+_nelts);
    if (dtmp != (int32 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding float32 data to a float32 array
float32 *hdf_genvec::export_float32(void) const {
    float32 *rv = 0;
    if (_nt != DFNT_FLOAT32)
	THROW(hcerr_dataexport);
    else
	ConvertArrayByCast((float32 *)_data, _nelts, &rv);
    return rv;
}

// return the i'th element of the vector as a float32
float32 hdf_genvec::elt_float32(int i) const  {
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt != DFNT_FLOAT32)
	THROW(hcerr_dataexport);
    return *((float32 *)_data+i);
}

// export an hdf_genvec holding float32 data to a float32 vector
vector<float32> hdf_genvec::exportv_float32(void) const {
    if (_nt != DFNT_FLOAT32) {
	THROW(hcerr_dataexport);
	return vector<float32>(0);
    }
    else
	return vector<float32>((float32 *)_data,(float32 *)_data+_nelts);
}

// export an hdf_genvec holding float32 or float64 data to a float64 array
float64 *hdf_genvec::export_float64(void) const {
    float64 *rv = 0;
    if (_nt == DFNT_FLOAT64)
	ConvertArrayByCast((float64 *)_data, _nelts, &rv);
    else if (_nt == DFNT_FLOAT32)  // cast to float64 array and export
	ConvertArrayByCast((float32 *)_data, _nelts, &rv);
    else
	THROW(hcerr_dataexport);
    return rv;
}

// return the i'th element of the vector as a float64
float64 hdf_genvec::elt_float64(int i) const  {
    if (i < 0  || i > _nelts)
	THROW(hcerr_range);
    if (_nt == DFNT_FLOAT64)
	return *((float64 *)_data+i);
    else if (_nt == DFNT_FLOAT32)
	return (float64)(*((float32 *)_data+i));
    else
	THROW(hcerr_dataexport);
    return 0;
}

// export an hdf_genvec holding float32 or float64 data to a float64 vector
vector<float64> hdf_genvec::exportv_float64(void) const {
    vector<float64> rv = vector<float64>(0);
    float64 *dtmp = 0;
    if (_nt == DFNT_FLOAT32)  // cast to float64 array and export
	ConvertArrayByCast((float32 *)_data, _nelts, &dtmp);
    else if (_nt == DFNT_FLOAT64)
	dtmp = (float64 *)_data;
    else
	THROW(hcerr_dataexport);
    rv = vector<float64>(dtmp, dtmp+_nelts);
    if (dtmp != (float64 *)_data)
	delete []dtmp;
    return rv;
}

// export an hdf_genvec holding char data to a String
String hdf_genvec::export_string(void) const {
    if (_nt != DFNT_CHAR) {
	THROW(hcerr_dataexport);
	return String();
    }
    else {
	if (_data == 0)
	    return String();
	else
	    return String((char *)_data,_nelts);
    }
}

// print all of the elements of hdf_genvec to a vector of String
void hdf_genvec::print(vector<String>& sv) const {
    if (_nelts > 0)
	print(sv, 0, _nelts-1, 1);
    return;
}

// print the elements of hdf_genvec to a vector of String; start with initial
// element "begin", end with "end" and increment by "stride" elements. 
void hdf_genvec::print(vector<String>& sv, int begin, int end, int stride) const
{
    if (begin < 0 || begin > _nelts || stride < 1  ||  end < 0  ||  end < begin  || 
	stride <= 0  ||  end > _nelts-1)
	THROW(hcerr_range);
    if (_nt == DFNT_CHAR) {
	String sub;
	sub = String((char *)_data+begin,(end-begin+1));
	if (stride > 1) {
	    String x;
	    for (int i=0; i<(end-begin+1); i+=stride)
		x += sub[i];
	    sub = x;
	}
	sv.push_back(sub);
    }
    else {
	char buf[hdfclass::MAXSTR];
	int i;
	switch(_nt) {
	case DFNT_UCHAR8:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << hex << 
		    (int)*((uchar8 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_UINT8:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    (unsigned int)*((uint8 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_INT8:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    (int)*((int8 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_UINT16:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    *((uint16 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_INT16:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    *((int16 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_UINT32:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    *((uint32 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_INT32:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    *((int32 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_FLOAT32:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    *((float32 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	case DFNT_FLOAT64:
	    for (i=begin; i<=end; i+=stride) {
		ostrstream(buf,hdfclass::MAXSTR) << 
		    *((float64 *)_data+i) << ends;
		sv.push_back(String(buf));
	    }
	    break;
	}
    }
    return;
}
